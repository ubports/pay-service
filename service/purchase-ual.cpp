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
#include <ubuntu-app-launch.h>
#include <gio/gio.h>
#include <mir_toolkit/mir_connection.h>
#include <mir_toolkit/mir_prompt_session.h>

namespace Purchase
{

class UalItem : public Item
{
public:
    typedef std::shared_ptr<Item> Ptr;

    UalItem (std::string& in_appid, std::string& in_itemid, std::shared_ptr<MirConnection> mir) :
        appid(in_appid),
        itemid(in_itemid),
        loop(nullptr),
        status(Item::ERROR),
        connection(mir)
    {
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
        if (loop != nullptr)
        {
            g_main_loop_quit(loop);
        }
    }

    static void stateChanged (MirPromptSession* session, MirPromptSessionState state, void* user_data)
    {
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
            std::string socketName("test");
            socketPromise->set_value(socketName);
            /* TODO */
        }).detach(); /* TODO: We should track this so we can clean it up if we don't use it for some reason */

        socketFuture.wait();
        return socketFuture.get();
    }

    /* Creates the thread to manage the execution of the Pay UI */
    bool appThreadCreate (std::string socketname)
    {
        t = std::thread([this, socketname]()
        {
            /* Build up the context and loop for the async events and a place
               for GDBus to send its events back to */
            GMainContext* context = g_main_context_new();
            loop = g_main_loop_new(context, FALSE);

            g_main_context_push_thread_default(context);

            /* We're grabbing the bus to ensure we can get it, but also
               to keep it connected for the lifecycle of this thread */
            GDBusConnection* bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
            if (bus == NULL)
            {
                purchaseComplete(Item::ERROR);
                return;
            }

            ubuntu_app_launch_observer_add_helper_stop(helper_stop_static_helper, HELPER_TYPE.c_str(), this);
            /* TODO: Add failed when in UAL */

            /* Building a URL to pass info to the Pay UI */
            std::string purchase_url = "purchase://";

            if (appid != "click-scope")
            {
                purchase_url += appid;
                purchase_url += "/";
            }

            purchase_url += itemid;
            const gchar* urls[2] = {0};
            urls[0] = purchase_url.c_str();

            gchar* helperid = ubuntu_app_launch_start_multiple_helper(HELPER_TYPE.c_str(), ui_appid.c_str(), urls);
            if (helperid != nullptr)
            {
                g_main_loop_run(loop);
                g_free(helperid);
            }

            /* Clean up */
            ubuntu_app_launch_observer_delete_helper_stop(helper_stop_static_helper, HELPER_TYPE.c_str(), this);

            g_clear_object(&bus);
            g_clear_pointer(&loop, g_main_loop_unref);
            g_clear_pointer(&context, g_main_context_unref);

            /* Signal where we end up */
            purchaseComplete(status);
        });

        t.detach();
        return true;
    }

private:
    /* Set at init */
    std::string appid;
    std::string itemid;
    std::string ui_appid;
    std::shared_ptr<MirConnection> connection;

    /* Created by run, destroyed with the object */
    std::thread t;

    /* Only used in thread t */
    GMainLoop* loop;
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
        g_main_loop_quit(loop);
    }
};

class UalFactory::Impl
{
    std::shared_ptr<MirConnection> connection;

    Impl(void)
    {
        connection = std::shared_ptr<MirConnection>(mir_connect_sync(nullptr, "pay-service"),
                                                    [](MirConnection * connection)
        {
            if (connection != nullptr)
            {
                mir_connection_release(connection);
            }
        });
    }

public:
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

UalFactory::UalFactory () : impl()
{
}

} // ns Purchase

