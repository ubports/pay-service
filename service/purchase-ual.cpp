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
#include <ubuntu-app-launch.h>
#include <gio/gio.h>
#include <mir_toolkit/mir_connection.h>

namespace Purchase
{

class UalItem : public Item
{
public:
    typedef std::shared_ptr<Item> Ptr;

    UalItem (std::string& in_appid, std::string& in_itemid) :
        appid(in_appid),
        itemid(in_itemid),
        loop(nullptr),
        status(Item::ERROR)
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

    virtual bool run (void)
    {
        if (ui_appid.empty())
        {
            return false;
        }
        t = std::thread([this]()
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

            ubuntu_app_launch_observer_add_app_stop(app_stop_static_helper, this);
            ubuntu_app_launch_observer_add_app_failed(app_failed_static_helper, this);

            /* Building a URL so that we can pass this information today without
               using trusted helpers and setting environment vars */
            /* TODO: Use trusted helpers */
            std::string purchase_url = "purchase://";

            if (appid != "click-scope")
            {
                purchase_url += appid;
                purchase_url += "/";
            }

            purchase_url += itemid;
            const gchar* urls[2] = {0};
            urls[0] = purchase_url.c_str();

            if (ubuntu_app_launch_start_application(ui_appid.c_str(), urls))
            {
                g_main_loop_run(loop);
            }

            /* Clean up */
            ubuntu_app_launch_observer_delete_app_stop(app_stop_static_helper, this);
            ubuntu_app_launch_observer_delete_app_failed(app_failed_static_helper, this);

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

    /* Created by run, destroyed with the object */
    std::thread t;

    /* Only used in thread t */
    GMainLoop* loop;
    Item::Status status;

    static void app_stop_static_helper (const gchar* appid, gpointer user_data)
    {
        UalItem* notthis = static_cast<UalItem*>(user_data);
        notthis->appStop(std::string(appid));
    }

    static void app_failed_static_helper (const gchar* appid, UbuntuAppLaunchAppFailed failure_type, gpointer user_data)
    {
        /* we're not actually using the failure type, we don't care why */
        UalItem* notthis = static_cast<UalItem*>(user_data);
        notthis->appFailed(std::string(appid));
    }

    void appStop (std::string stop_appid)
    {
        if (stop_appid != ui_appid)
        {
            return;
        }

        status = Item::PURCHASED;
        g_main_loop_quit(loop);
    }

    void appFailed (std::string failed_appid)
    {
        if (failed_appid != ui_appid)
        {
            return;
        }

        status = Item::NOT_PURCHASED;
        g_main_loop_quit(loop);
    }
};

class UalFactory::Impl
{
    std::unique_ptr<MirConnection, void(*)(MirConnection*)> connection;

    Impl(void) :
        connection(mir_connect_sync(nullptr, "pay-service"), mir_connection_release)
    {
    }

    /* Figures out the PID that we should be overlaying with the PayUI */
    pid_t appid2pid (std::string& appid)
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

public:
    Item::Ptr purchaseItem (std::string& appid, std::string& itemid)
    {
        pid_t overlaypid = appid2pid(appid);
        if (overlaypid == 0)
        {
            /* We can't overlay nothin' */
            Item::Ptr empty;
            return empty;
        }

        return std::make_shared<UalItem>(appid, itemid);
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

