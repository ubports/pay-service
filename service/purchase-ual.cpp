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
#include <gio/gunixfdlist.h>
#include <mir_toolkit/mir_connection.h>
#include <mir_toolkit/mir_prompt_session.h>

#include "glib-thread.h"

static const char* HELPER_TYPE = "pay-ui";

namespace Purchase
{

class UalItem : public Item
{
public:
    typedef std::shared_ptr<Item> Ptr;

    UalItem (const std::string& in_appid, const std::string& in_itemid, const std::shared_ptr<MirConnection>& mir) :
        status(Item::ERROR),
        appid(in_appid),
        itemid(in_itemid),
        connection(mir)
    {

    }

    ~UalItem ()
    {
    }

    /* Goes through the basis phases of building up the environment for the
       UI to run in. Ensures we've got an AppID, builds the session, sets up
       the socket to pass the session. And then starts the UI. */
    virtual bool run (void)
    {
        ui_appid = discoverUiAppid();
        if (ui_appid.empty())
        {
            g_warning("Empty UI App ID for PayUI");
            return false;
        }

        auto purchase_url = buildPurchaseUrl();
        if (purchase_url.empty())
        {
            g_warning("Unable to determine purchase URL");
            return false;
        }

        thread = std::make_shared<GLib::ContextThread>([]() {}, [this]()
        {
            if (!instanceid.empty())
            {
                ubuntu_app_launch_stop_multiple_helper(HELPER_TYPE, ui_appid.c_str(), instanceid.c_str());
                instanceid.clear();
            }

            if (session)
            {
                session.reset();
            }

            purchaseComplete(status);
            ubuntu_app_launch_observer_delete_helper_stop(helper_stop_static_helper, HELPER_TYPE, this);
        });
        instanceid = thread->executeOnThread<std::string>([this, purchase_url]()
        {
            std::string instid;

            pid_t overlaypid = appid2pid(appid);
            if (overlaypid == 0)
            {
                return instid;
            }

            session = std::shared_ptr<MirPromptSession>(
                          mir_connection_create_prompt_session_sync(connection.get(), overlaypid, stateChanged, this),
                          [](MirPromptSession * session)
            {
                if (session != nullptr)
                {
                    mir_prompt_session_release_sync(session);
                }
            });

            if (!session)
            {
                return instid;
            }

            ubuntu_app_launch_observer_add_helper_stop(helper_stop_static_helper, HELPER_TYPE, this);

            std::array<const gchar*, 2>urls {purchase_url.c_str(), nullptr};
            auto instance_c = ubuntu_app_launch_start_session_helper(HELPER_TYPE,
                                                                     session.get(),
                                                                     ui_appid.c_str(),
                                                                     urls.data());
            instid = std::string(instance_c);
            g_free(instance_c);
            return instid;
        });

        return !instanceid.empty();
    }

private:
    /* Set at init */
    std::string appid;
    std::string ui_appid;
    std::string itemid;
    std::string instanceid;
    std::shared_ptr<GLib::ContextThread> thread;
    std::shared_ptr<MirPromptSession> session;
    Item::Status status;

    /* Given to us by our parents */
    std::shared_ptr<MirConnection> connection;

    /* Looks through a directory to find the first entry that is a .desktop file
       and uses that as our AppID. We don't support more than one entry being in
       a directory */
    std::string discoverUiAppid (void)
    {
        std::string appid;
        GDir* dir;
        const gchar* clickhookdir = g_getenv("PAY_SERVICE_CLICK_DIR");
        if (clickhookdir == nullptr)
        {
            gchar* cacheclickdir = g_build_filename(g_get_user_cache_dir(), "pay-service", HELPER_TYPE, nullptr);
            g_debug("Looking for Pay UI in: %s", cacheclickdir);
            dir = g_dir_open(cacheclickdir, 0, nullptr);
            g_free(cacheclickdir);
        }
        else
        {
            g_debug("Looking for Pay UI in: %s", clickhookdir);
            dir = g_dir_open(clickhookdir, 0, nullptr);
        }

        if (dir != nullptr)
        {
            const gchar* name = nullptr;

            do
            {
                name = g_dir_read_name(dir);
                g_debug("Looking at file: %s", name);
            }
            while (name != nullptr && !g_str_has_suffix(name, ".desktop"));

            g_debug("Chose file: %s", name);

            gchar* desktopsuffix = nullptr;
            if (name != nullptr)
            {
                desktopsuffix = g_strstr_len(name, -1, ".desktop");
            }

            if (desktopsuffix != nullptr)
            {
                appid.assign(name, desktopsuffix - name);
            }

            g_dir_close(dir);
        }

        return appid;
    }

    /* Build up the URL that we use to pass the purchase information on
       to the Pay UI */
    std::string buildPurchaseUrl (void)
    {
        std::string purchase_url = "purchase://";

        if (appid != "click-scope")
        {
            purchase_url += appid;
            purchase_url += '/';
        }

        purchase_url += itemid;

        return purchase_url;
    }

    static pid_t pidForUpstartJob (const gchar* jobname)
    {
        GError* error = nullptr;

        if (jobname == nullptr)
        {
            return 0;
        }

        auto bus = std::shared_ptr<GDBusConnection>(g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr,
                                                                   NULL), [](GDBusConnection * bus)
        {
            g_clear_object(&bus);
        });
        if (!bus)
        {
            g_critical("Unable to get session bus");
            return 0;
        }

        GVariant* retval = nullptr;
        retval = g_dbus_connection_call_sync(
                     bus.get(),
                     "com.ubuntu.Upstart",
                     "/com/ubuntu/Upstart",
                     "com.ubuntu.Upstart0_6",
                     "GetJobByName",
                     g_variant_new("(s)", jobname),
                     G_VARIANT_TYPE("(o)"),
                     G_DBUS_CALL_FLAGS_NO_AUTO_START,
                     -1, /* timeout */
                     NULL, /* cancel */
                     &error);

        if (error != NULL)
        {
            g_warning("Unable to get path for job '%s': %s", jobname, error->message);
            g_error_free(error);
            return 0;
        }

        gchar* path = nullptr;
        g_variant_get(retval, "(o)", &path);
        g_variant_unref(retval);

        retval = g_dbus_connection_call_sync(
                     bus.get(),
                     "com.ubuntu.Upstart",
                     path,
                     "com.ubuntu.Upstart0_6.Job",
                     "GetInstanceByName",
                     g_variant_new("(s)", ""),
                     G_VARIANT_TYPE("(o)"),
                     G_DBUS_CALL_FLAGS_NO_AUTO_START,
                     -1, /* timeout */
                     NULL, /* cancel */
                     &error);

        g_free(path);

        if (error != NULL)
        {
            g_warning("Unable to get instance for job '%s': %s", jobname, error->message);
            g_error_free(error);
            return 0;
        }

        g_variant_get(retval, "(o)", &path);
        g_variant_unref(retval);

        retval = g_dbus_connection_call_sync(
                     bus.get(),
                     "com.ubuntu.Upstart",
                     path,
                     "org.freedesktop.DBus.Properties",
                     "Get",
                     g_variant_new("(ss)", "com.ubuntu.Upstart0_6.Instance", "processes"),
                     G_VARIANT_TYPE("(v)"),
                     G_DBUS_CALL_FLAGS_NO_AUTO_START,
                     -1, /* timeout */
                     NULL, /* cancel */
                     &error);

        g_free(path);

        if (error != NULL)
        {
            g_warning("Unable to get processes for job '%s': %s", jobname, error->message);
            g_error_free(error);
            return 0;
        }

        GPid pid = 0;
        GVariant* variant = g_variant_get_child_value(retval, 0);
        GVariant* array = g_variant_get_variant(variant);
        if (g_variant_n_children(array) > 0)
        {
            /* (si) */
            GVariant* firstitem = g_variant_get_child_value(array, 0);
            GVariant* vpid = g_variant_get_child_value(firstitem, 1);
            pid = g_variant_get_int32(vpid);
            g_variant_unref(vpid);
            g_variant_unref(firstitem);
        }
        g_variant_unref(variant);
        g_variant_unref(array);
        g_variant_unref(retval);

        return pid;
    }

    /* Figures out the PID that we should be overlaying with the PayUI */
    static pid_t appid2pid (std::string& appid)
    {
        if (appid == "click-scope")
        {
            /* FIXME: Before other scopes can use pay, we'll need to figure out
               how to detect if they're scopes or not. But for now we'll only just
               look for 'click-scope' as it's our primary use-case */
            return pidForUpstartJob("unity8-dash");
        }
        else
        {
            return ubuntu_app_launch_get_primary_pid(appid.c_str());
        }
    }

    static void stateChanged (MirPromptSession* session, MirPromptSessionState state, void* user_data)
    {
        g_debug("Mir Prompt session is in state: %d", state);
    }

    static void helper_stop_static_helper (const gchar* appid,
                                           const gchar* instanceid,
                                           const gchar* helpertype,
                                           gpointer user_data)
    {
        UalItem* notthis = static_cast<UalItem*>(user_data);
        g_debug("UAL Stop callback, appid: '%s', instance: '%s', helper: '%s'", appid, instanceid, helpertype);

        if (instanceid == nullptr) /* Causes std::string to hate us, and we don't care about it if not set */
        {
            return;
        }

        notthis->helperStop(std::string(appid), std::string(instanceid));
    }

    void helperStop (std::string stop_appid, std::string stop_instanceid)
    {
        if (stop_appid != ui_appid)
        {
            return;
        }

        if (instanceid.empty() || instanceid != stop_instanceid)
        {
            return;
        }

        status = Item::PURCHASED;
        instanceid.clear();
        thread->quit();
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

        if (!connection)
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
    if (!impl)
    {
        throw std::runtime_error("Unable to build implementation of UAL Factory");
    }
}

} // ns Purchase

