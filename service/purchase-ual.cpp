/*
 * Copyright © 2014 Canonical Ltd.
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

#include "purchase-ual.h"
#include <thread>
#include <future>
#include <system_error>
#include <ubuntu-app-launch.h>
#include <gio/gio.h>
#include <mir_toolkit/mir_connection.h>
#include <mir_toolkit/mir_prompt_session.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

typedef struct sockaddr_un addrunstruct;
typedef struct sockaddr addrstruct;
typedef struct msghdr msgstruct;
typedef struct iovec iovecstruct;
typedef struct
{
    struct cmsghdr chdr;
    int fd;
} fdcmsghdr;

namespace Purchase
{

class UalItem : public Item
{
public:
    typedef std::shared_ptr<Item> Ptr;

    UalItem (std::string& in_appid, std::string& in_itemid, std::shared_ptr<MirConnection> mir) :
        appid(in_appid),
        itemid(in_itemid),
        status(Item::ERROR),
        connection(mir)
    {
        stopThread = std::shared_ptr<GCancellable>(g_cancellable_new(), [](GCancellable * cancel)
        {
            if (cancel != nullptr)
            {
                g_cancellable_cancel(cancel);
                g_object_unref(cancel);
            }
        });

        /* TODO: ui_appid needs to be grabbed from the click hook */
        gchar* appidc = ubuntu_app_launch_triplet_to_app_id("com.canonical.payui",
                                                            nullptr,
                                                            nullptr);
        if (appidc != nullptr)
        {
            ui_appid = appidc;
            g_free(appidc);
        }
    }

    ~UalItem ()
    {
        cleanupThread();
    }

    static void stateChanged (MirPromptSession* session, MirPromptSessionState state, void* user_data)
    {
        g_debug("Mir Prompt session is in stated: %d", state);
    }

    /* Goes through the basis phases of building up the environment for the
       UI to run in. Ensures we've got an AppID, builds the session, sets up
       the socket to pass the session. And then starts the UI. */
    virtual bool run (void)
    {
        if (ui_appid.empty())
        {
            return false;
        }

        auto session = setupSession();
        if (session == nullptr)
        {
            return false;
        }

        std::string socketname = setupSocket(session);
        if (socketname.empty())
        {
            return false;
        }

        return appThreadCreate(socketname);
    }

    /* Creates a Mir Prompt Session by finding the overlay pid and making it. */
    std::shared_ptr<MirPromptSession> setupSession (void)
    {
        pid_t overlaypid = appid2pid(appid);
        if (overlaypid == 0)
        {
            /* We can't overlay nothin' */
            return nullptr;
        }

        /* Setup the trusted prompt session */
        auto session = std::shared_ptr<MirPromptSession>(
                           mir_connection_create_prompt_session_sync(connection.get(), overlaypid, stateChanged, this),
                           [](MirPromptSession * session)
        {
            if (session != nullptr)
            {
                mir_prompt_session_release_sync(session);
            }
        });

        if (session == nullptr)
        {
            g_critical("Unable to create a trusted prompt session");
        }

        return session;
    }

    /* Creates the abstract socket for communicating the file handle to the
       Pay UI and builds a thread to service it */
    std::string setupSocket (std::shared_ptr<MirPromptSession>& session)
    {
        auto socketPromise = std::make_shared<std::promise<std::string>>();
        auto socketFuture = socketPromise->get_future();

        std::thread([socketPromise, session]()
        {
            std::string socketName;

            int sock = socket(AF_UNIX, SOCK_STREAM, 0);
            if (sock == 0)
            {
                g_critical("Unable to create socket");
                socketPromise->set_value(socketName);
                return;
            }

            /* Create a Unique-ish Name */
            int bindtry = 0;
            do
            {
                bindtry++;
                char templateName[32] = {"/tmp/pay-service-XXXXXX"};
                mktemp(templateName);
                std::string tempSock(templateName);

                g_debug("Socket name attempt: %s", templateName);

                addrunstruct addr = {0};
                addr.sun_family = AF_UNIX;
                memcpy(addr.sun_path + 1, tempSock.c_str(), tempSock.size() + 1);
                int bindret = bind(sock, reinterpret_cast<addrstruct*>(&addr), sizeof(addrunstruct));

                if (bindret == 0)
                {
                    g_debug("Bound to socket: %s", templateName);
                    socketName = std::string(templateName);
                }
                else
                {
                    perror("Unable to bind to socket");
                }
            }
            while (socketName.empty() && bindtry < 5);

            /* At this point we're kicking off starting up the process, so we're
               already bound, which is good. But let's remember what's happening here. */
            socketPromise->set_value(socketName);

            /* If we didn't get a socket name, we should just exit. And
               make sure to clean up the socket. */
            if (socketName.empty())
            {
                g_critical("Unable to bind to any name");
                close(sock);
                return;
            }

            int fdlist[1] = {0};
            auto mirwait = mir_prompt_session_new_fds_for_prompt_providers(session.get(),
                                                                           1,
                                                                           [](MirPromptSession * session, size_t count, int const * fdsin, void * context) -> void
            {
                g_debug("FDs %d Returned from Mir", count);
                if (count != 1) return;
                int* fdout = reinterpret_cast<int*>(context);
                fdout[0] = fdsin[0];
            },
            fdlist);

            mir_wait_for(mirwait);

            if (fdlist[0] == 0)
            {
                g_debug("FD from Mir was a 0");
                close(sock);
                return;
            }

            listen(sock, 1);

            g_debug("Waiting for connection…");
            addrstruct accepted = {0};
            socklen_t length = sizeof(addrstruct);
            int acceptsock = accept(sock, &accepted, &length);
            if (acceptsock <= 0)
            {
                perror("No acceptance");
                close(sock);
                return;
            }

            iovecstruct iovec = {0};
            int dummydata = 0;
            iovec.iov_base = &dummydata;
            iovec.iov_len = sizeof(dummydata);

            fdcmsghdr cmessage = {0};
            cmessage.chdr.cmsg_len = CMSG_LEN(sizeof(int));
            cmessage.chdr.cmsg_level = SOL_SOCKET;
            cmessage.chdr.cmsg_type = SCM_RIGHTS;
            cmessage.fd = fdlist[0];

            msgstruct message = {0};
            message.msg_control = &cmessage;
            message.msg_controllen = 1;
            message.msg_iov = &iovec;
            message.msg_iovlen = 1;

            g_debug("Sending FD via socket…");
            /* This will block until someone picks up the message */
            int sendcnt = sendmsg(acceptsock, &message, 0);
            if (sendcnt < 0)
            {
                perror("Send message error");
            }

            /* If it's sent, we're done */
            close(acceptsock);
            close(sock);
            g_debug("Shutting down this side of the socket.");
        }).detach(); /* TODO: We should track this so we can clean it up if we don't use it for some reason */

        socketFuture.wait();
        return socketFuture.get();
    }

    void cleanupThread (void)
    {
        if (t.joinable())
        {
            g_cancellable_cancel(stopThread.get());
            if (loop != nullptr)
            {
                g_main_loop_quit(loop.get());
            }

            t.join();
        }
    }

    /* Creates the thread to manage the execution of the Pay UI */
    bool appThreadCreate (std::string socketname)
    {
        cleanupThread();
        g_cancellable_reset(stopThread.get());

        t = std::thread([this, socketname]()
        {
            /* Build up the context and loop for the async events and a place
               for GDBus to send its events back to */
            context = std::shared_ptr<GMainContext>(g_main_context_new(), [](GMainContext * context)
            {
                if (context != nullptr)
                {
                    g_main_context_unref(context);
                }
            });
            loop = std::shared_ptr<GMainLoop>(g_main_loop_new(context.get(), FALSE), [](GMainLoop * loop)
            {
                if (loop != nullptr)
                {
                    g_main_loop_unref(loop);
                }
            });

            g_main_context_push_thread_default(context.get());

            /* We're grabbing the bus to ensure we can get it, but also
               to keep it connected for the lifecycle of this thread */
            bus = std::shared_ptr<GDBusConnection>(g_bus_get_sync(G_BUS_TYPE_SESSION, stopThread.get(),
                                                                  NULL), [](GDBusConnection * bus)
            {
                if (bus != nullptr)
                {
                    g_object_unref(bus);
                }
            });
            if (bus == nullptr)
            {
                purchaseComplete(Item::ERROR);
                return;
            }

            ubuntu_app_launch_observer_add_helper_stop(helper_stop_static_helper, HELPER_TYPE.c_str(), this);
            /* TODO: Add failed when in UAL */

            /* Building a URL to pass info to the Pay UI */
            std::string purchase_url = buildPurchaseUrl();
            const gchar* urls[3] = {0};
            urls[0] = socketname.c_str();
            urls[1] = purchase_url.c_str();

            gchar* helperid = ubuntu_app_launch_start_multiple_helper(HELPER_TYPE.c_str(), ui_appid.c_str(), urls);
            if (helperid != nullptr && !g_cancellable_is_cancelled(stopThread.get()))
            {
                g_main_loop_run(loop.get());
            }

            /* Clean up */
            ubuntu_app_launch_observer_delete_helper_stop(helper_stop_static_helper, HELPER_TYPE.c_str(), this);

            /* If we've been cancelled we need to clean up the sub process too */
            if (helperid != nullptr && g_cancellable_is_cancelled(stopThread.get()))
            {
                ubuntu_app_launch_stop_multiple_helper(HELPER_TYPE.c_str(), ui_appid.c_str(), helperid);
            }

            bus.reset();
            loop.reset();
            context.reset();
            g_free(helperid);

            /* Signal where we end up */
            purchaseComplete(status);
        });

        return true;
    }

    /* Build up the URL that we use to pass the purchase information on
       to the Pay UI */
    std::string buildPurchaseUrl (void)
    {
        std::string purchase_url = "purchase://";

        if (appid != "click-scope")
        {
            purchase_url += appid;
            purchase_url += "/";
        }

        purchase_url += itemid;

        return purchase_url;
    }

private:
    /* Set at init */
    std::string appid;
    std::string itemid;
    std::string ui_appid;

    /* Given to us by our parents */
    std::shared_ptr<MirConnection> connection;

    /* Created by run, destroyed with the object */
    std::thread t;
    std::shared_ptr<GCancellable> stopThread;

    /* Lifecycle should generally match thread t */
    std::shared_ptr<GMainContext> context;
    std::shared_ptr<GMainLoop> loop;
    std::shared_ptr<GDBusConnection> bus;

    /* For the callbacks */
    Item::Status status;

    /* Const */
    const std::string HELPER_TYPE
    {"pay-ui"
    };

    /* Figures out the PID that we should be overlaying with the PayUI */
    static pid_t appid2pid (std::string& appid)
    {
        if (appid == "click-scope")
        {
            /* TODO: For the click-scope we're using the dash's pid */
            return 0;
        }
        else
        {
            return ubuntu_app_launch_get_primary_pid(appid.c_str());
        }
    }

    static void helper_stop_static_helper (const gchar* appid, const gchar* instanceid, const gchar* helpertype,
                                           gpointer user_data)
    {
        UalItem* notthis = static_cast<UalItem*>(user_data);
        notthis->helperStop(std::string(appid));
    }

    void helperStop (std::string stop_appid)
    {
        if (stop_appid != ui_appid)
        {
            return;
        }

        status = Item::PURCHASED;
        g_main_loop_quit(loop.get());
    }
};

class UalFactory::Impl
{
    std::shared_ptr<MirConnection> connection;

public:
    Impl(void)
    {
        gchar* mirpath = g_build_filename(g_get_user_runtime_dir(), "mir_socket_trusted", NULL);

        connection = std::shared_ptr<MirConnection>(mir_connect_sync(mirpath, "pay-service"),
                                                    [](MirConnection * connection)
        {
            if (connection != nullptr)
            {
                mir_connection_release(connection);
            }
        });

        g_free(mirpath);

        if (connection == nullptr)
        {
            throw std::runtime_error("Unable to connect to Mir Trusted Session");
        }
    }

    Item::Ptr purchaseItem (std::string& appid, std::string& itemid)
    {
        return std::make_shared<UalItem>(appid, itemid, connection);
    }
};

Item::Ptr
UalFactory::purchaseItem (std::string& appid, std::string& itemid)
{
    return impl->purchaseItem(appid, itemid);
}

UalFactory::UalFactory ()
{
    impl = std::make_shared<Impl>();
    if (impl == nullptr)
    {
        throw std::runtime_error("Unable to build implementation of UAL Factory");
    }
}

} // ns Purchase

