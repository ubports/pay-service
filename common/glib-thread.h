/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Ted Gould <ted.gould@canonical.com>
 */

#include <thread>
#include <future>

#include <gio/gio.h>

namespace GLib
{

class ContextThread
{
    std::thread _thread;
    std::shared_ptr<GMainContext> _context;
    std::shared_ptr<GMainLoop> _loop;
    std::shared_ptr<GCancellable> _cancel;

public:
    ContextThread (std::function<void(void)> beforeLoop = [] {}, std::function<void(void)> afterLoop = [] {})
        : _context(nullptr)
        , _loop(nullptr)
    {
        _cancel = std::shared_ptr<GCancellable>(g_cancellable_new(), [](GCancellable * cancel)
        {
            if (cancel != nullptr)
            {
                g_cancellable_cancel(cancel);
                g_object_unref(cancel);
            }
        });
        std::promise<std::pair<std::shared_ptr<GMainContext>, std::shared_ptr<GMainLoop>>> context_promise;

        /* NOTE: We copy afterLoop but reference beforeLoop. We're blocking so we
           know that beforeLoop will stay valid long enough, but we can't say the
           same for afterLoop */
        _thread = std::thread([&context_promise, &beforeLoop, afterLoop, this](void) -> void
        {
            /* Build up the context and loop for the async events and a place
               for GDBus to send its events back to */
            auto context = std::shared_ptr<GMainContext>(g_main_context_new(), [](GMainContext * context)
            {
                if (context != nullptr)
                {
                    g_main_context_unref(context);
                }
            });
            auto loop = std::shared_ptr<GMainLoop>(g_main_loop_new(context.get(), FALSE), [](GMainLoop * loop)
            {
                if (loop != nullptr)
                {
                    g_main_loop_unref(loop);
                }
            });

            g_main_context_push_thread_default(context.get());

            beforeLoop();

            /* Free's the constructor to continue */
            auto pair = std::pair<std::shared_ptr<GMainContext>, std::shared_ptr<GMainLoop>>(context, loop);
            context_promise.set_value(pair);

            if (!g_cancellable_is_cancelled(_cancel.get()))
            {
                g_main_loop_run(loop.get());
            }

            afterLoop();
        });

        /* We need to have the context and the mainloop ready before
           other functions on this object can work properly. So we wait
           for them and set them on this thread. */
        auto context_future = context_promise.get_future();
        context_future.wait();
        auto context_value = context_future.get();

        _context = context_value.first;
        _loop = context_value.second;

        if (_context == nullptr || _loop == nullptr)
        {
            throw std::runtime_error("Unable to create GLib Thread");
        }
    }

    ~ContextThread (void)
    {
        quit();

        if (_thread.joinable())
        {
            _thread.join();
        }
    }

    void quit (void)
    {
        g_cancellable_cancel(_cancel.get()); /* Force the cancellation on ongoing tasks */
        if (_loop != nullptr)
        {
            g_main_loop_quit(_loop.get());    /* Quit the loop */
        }
    }

    bool isCancelled (void)
    {
        return g_cancellable_is_cancelled(_cancel.get()) == TRUE;
    }

    std::shared_ptr<GCancellable> getCancellable (void)
    {
        return _cancel;
    }

    template<typename T> auto executeOnThread (std::function<T(void)> work) -> T
    {
        if (isCancelled())
        {
            throw std::runtime_error("Trying to execute work on a GLib thread that is shutting down.");
        }

        std::promise<T> promise;
        std::function<gboolean(void)> magicFunc = [&promise, &work] (void) -> gboolean {
            promise.set_value(work());
            return G_SOURCE_REMOVE;
        };

        auto source = g_idle_source_new();
        g_source_set_callback(source,
        [](gpointer data) -> gboolean {
            std::function<gboolean(void)>* magic = reinterpret_cast<std::function<gboolean(void)> *>(data);
            return (*magic)();
        }, &magicFunc, nullptr);

        g_source_attach(source, _context.get());
        g_source_unref(source);

        auto future = promise.get_future();
        future.wait();
        return future.get();
    }

private:
    void simpleSource (std::function<std::shared_ptr<GSource>(void)> srcBuilder, std::function<void(void)> work)
    {
        if (isCancelled())
        {
            throw std::runtime_error("Trying to execute work on a GLib thread that is shutting down.");
        }

        /* Copy the work so that we can reuse it */
        /* Lifecycle is handled with the source pointer when we attach
           it to the context. */
        auto heapWork = new std::function<void(void)>(work);

        auto source = srcBuilder();
        g_source_set_callback(source.get(),
                              [](gpointer data) -> gboolean
        {
            std::function<void(void)>* heapWork = reinterpret_cast<std::function<void(void)> *>(data);
            (*heapWork)();
            return G_SOURCE_REMOVE;
        }, heapWork,
        [](gpointer data)
        {
            std::function<void(void)>* heapWork = reinterpret_cast<std::function<void(void)> *>(data);
            delete heapWork;
        });

        g_source_attach(source.get(), _context.get());
    }

public:
    void executeOnThread (std::function<void(void)> work)
    {
        simpleSource([]() -> std::shared_ptr<GSource>
        {
            return std::shared_ptr<GSource>(g_idle_source_new(), [](GSource * src)
            {
                if (src != nullptr)
                {
                    g_source_unref(src);
                }
            });
        }, work);
    }

    void timeout (std::chrono::milliseconds length, std::function<void(void)> work)
    {
        simpleSource([length]() -> std::shared_ptr<GSource>
        {
            return std::shared_ptr<GSource>(g_timeout_source_new(length.count()), [](GSource * src)
            {
                if (src != nullptr)
                {
                    g_source_unref(src);
                }
            });
        }, work);
    }

    void timeoutSeconds (std::chrono::seconds length, std::function<void(void)> work)
    {
        simpleSource([length]() -> std::shared_ptr<GSource>
        {
            return std::shared_ptr<GSource>(g_timeout_source_new_seconds(length.count()), [](GSource * src)
            {
                if (src != nullptr)
                {
                    g_source_unref(src);
                }
            });
        }, work);
    }
};
}
