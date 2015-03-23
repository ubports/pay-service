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
#include "proxy-payui.h"

#include <thread>
#include <future>
#include <system_error>
#include <ubuntu-app-launch.h>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <mir_toolkit/mir_connection.h>
#include <mir_toolkit/mir_prompt_session.h>

#include <glib-thread.h>

static const char* HELPER_TYPE = "pay-ui";

namespace Purchase
{

class UalItem : public Item
{
private:
    class HelperThread
    {
    private:
        /* Members Only */
        std::string _socketname;
        GLib::ContextThread _thread;
        std::string _helperid;
        std::string _appid;
        std::string _purchaseUrl;
        Item::Status _status = Item::ERROR;

        /* Const */
        static void helper_stop_static_helper (const gchar* appid, const gchar* instanceid, const gchar* helpertype,
                                               gpointer user_data)
        {
            auto notthis = static_cast<HelperThread*>(user_data);
            notthis->helperStop(std::string(appid));
        }

        void helperStop (std::string stop_appid)
        {
            if (stop_appid != _appid)
            {
                return;
            }

            _status = Item::PURCHASED;
            _thread.quit();
        }

    public:
        core::Signal<Item::Status> helperFinished;

        HelperThread (const std::string& socketname, const std::string& appid, const std::string& purchaseUrl)
            : _socketname(socketname)
            , _appid(appid)
            , _purchaseUrl(purchaseUrl)
            , _thread([this]
        {
            if (!ubuntu_app_launch_observer_add_helper_stop(helper_stop_static_helper, HELPER_TYPE, this))
                throw std::runtime_error("Unable to register Stop Helper");
            /* TODO: Add failed when in UAL */
        },
        [this]
        {
            /* Clean up */
            ubuntu_app_launch_observer_delete_helper_stop(helper_stop_static_helper, HELPER_TYPE, this);

            /* If we've been cancelled we need to clean up the sub process too */
            if (!_helperid.empty() && _thread.isCancelled())
            {
                ubuntu_app_launch_stop_multiple_helper(HELPER_TYPE, _appid.c_str(), _helperid.c_str());
            }

            helperFinished(_status);
        })

        {
            _helperid = _thread.executeOnThread<std::string>([this]() -> std::string
            {
                /* Building a URL to pass info to the Pay UI */
                const gchar* urls[3] = {0};
                urls[0] = _socketname.c_str();
                urls[1] = _purchaseUrl.c_str();

                gchar* helperid = ubuntu_app_launch_start_multiple_helper(HELPER_TYPE, _appid.c_str(), urls);
                std::string retval(helperid);
                g_free(helperid);
                return retval;
            });
        }

        ~HelperThread (void)
        {
            /* Quit before other variables are free'd */
            _thread.quit();
        }
    };

    class MirSocketThread
    {
        /* From Init */
        std::shared_ptr<MirConnection> connection;
        std::string appid;
        std::string ui_appid;

        /* Built at Init */
        std::shared_ptr<MirPromptSession> session;
        std::string socketName;

        /* On thread */
        GLib::ContextThread thread;
        std::shared_ptr<proxyPayPayui> payuiobj;
        std::shared_ptr<GDBusConnection> bus;
        int fdlist[1] = {0};

        /* Creates a Mir Prompt Session by finding the overlay pid and making it. */
        std::shared_ptr<MirPromptSession> setupSession (void)
        {
            pid_t overlaypid = appid2pid(appid);
            if (overlaypid == 0)
            {
                /* We can't overlay nothin' */
                g_debug("Unable to find PID for: %s", appid.c_str());
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
        std::string setupSocket ()
        {
            return thread.executeOnThread<std::string>([this]()
            {
                GError* error = nullptr;
                std::string socketName;

                bus = std::shared_ptr<GDBusConnection>(g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr,
                                                                      nullptr), [](GDBusConnection * bus)
                {
                    if (bus != nullptr)
                    {
                        g_object_unref(bus);
                    }
                });
                if (bus == nullptr)
                {
                    g_critical("Unable to get session bus");
                    return std::string();
                }

                /* Export an Object on DBus */
                payuiobj = std::shared_ptr<proxyPayPayui>(
                               proxy_pay_payui_skeleton_new(),
                               [](proxyPayPayui * payui)
                {
                    if (payui != nullptr)
                    {
                        g_dbus_interface_skeleton_unexport(G_DBUS_INTERFACE_SKELETON(payui));
                        g_object_unref(payui);
                    }
                });
                g_signal_connect(payuiobj.get(), "handle-get-mir-socket", G_CALLBACK(mirHandle), fdlist);

                auto encodedAppId = encodePath(ui_appid);
                /* Loop until we fine an object path that isn't taken (probably only once) */
                while (socketName.empty())
                {
                    gchar* tryname = g_strdup_printf("/com/canonical/pay/%s/%X", encodedAppId.c_str(), g_random_int());
                    g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(payuiobj.get()),
                                                     bus.get(),
                                                     tryname,
                                                     &error);

                    if (error == NULL)
                    {
                        socketName = std::string(tryname);
                    }
                    else
                    {
                        /* Always print the error, but if the object path is in use let's
                           not exit the loop. Let's just try again. */
                        bool exitnow = (error->domain != G_DBUS_ERROR || error->code != G_DBUS_ERROR_OBJECT_PATH_IN_USE);
                        g_critical("Unable to export payui object: %s", error->message);
                        g_error_free(error);
                        if (exitnow)
                        {
                            g_free(tryname);
                            break;
                        }
                    }

                    g_free(tryname);
                }

                /* If we didn't get a socket name, we should just exit. And
                   make sure to clean up the socket. */
                if (socketName.empty())
                {
                    g_critical("Unable to export object to any name");
                    return std::string();
                }

                auto mirwait = mir_prompt_session_new_fds_for_prompt_providers(session.get(),
                                                                               1,
                                                                               [](MirPromptSession * session, size_t count, int const * fdsin, void* context) -> void
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
                    g_critical("FD from Mir was a 0");
                    return std::string();
                }

                return socketName;
            });
        }

        static bool mirHandle (GObject* obj, GDBusMethodInvocation* invocation, gpointer user_data)
        {
            int* fds = reinterpret_cast<int*>(user_data);

            if (fds[0] == 0)
            {
                g_critical("No FDs to give!");
                return false;
            }

            /* Index into fds */
            GVariant* handle = g_variant_new_handle(0);
            GVariant* tuple = g_variant_new_tuple(&handle, 1);

            GError* error = nullptr;
            GUnixFDList* list = g_unix_fd_list_new();
            g_unix_fd_list_append(list, fds[0], &error);

            if (error == nullptr)
            {
                g_dbus_method_invocation_return_value_with_unix_fd_list(invocation, tuple, list);
            }

            g_object_unref(list);

            if (error != nullptr)
            {
                g_critical("Unable to pass FD %d: %s", fds[0], error->message);
                g_error_free(error);
                return false;
            }

            return true;
        }

        static std::string encodePath (const std::string& input)
        {
            std::string output = "";
            bool first = true;

            for (unsigned char c : input)
            {
                std::string retval;

                if ((c >= 'a' && c <= 'z') ||
                        (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9' && !first))
                {
                    retval = std::string((char*)&c, 1);
                }
                else
                {
                    char buffer[5] = {0};
                    std::snprintf(buffer, sizeof(buffer), "_%2X", c);
                    retval = std::string(buffer);
                }

                output += retval;
                first = false;
            }

            return output;
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
                if (bus != nullptr)
                {
                    g_object_unref(bus);
                }
            });
            if (bus == nullptr)
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

    public:
        MirSocketThread (std::shared_ptr<MirConnection> in_connection, const std::string& in_appid,
                         const std::string& in_ui_appid)
            : connection(in_connection)
            , appid(in_appid)
            , ui_appid(in_ui_appid)
            , thread([]() {}, [this]()
        {
            payuiobj.reset();
            bus.reset();
        })
        {
            session = setupSession();
            socketName = setupSocket();
        }

        std::string getSocketName (void)
        {
            return socketName;
        }
    };

public:
    typedef std::shared_ptr<Item> Ptr;

    UalItem (const std::string& in_appid, const std::string& in_itemid, const std::shared_ptr<MirConnection>& mir) :
        appid(in_appid),
        itemid(in_itemid),
        connection(mir)
    {

    }

    ~UalItem ()
    {
        helperFinishedConnection->disconnect();
        helperFinishedConnection.reset();
        helperThread.reset();
        socketThread.reset();
    }

    /* Goes through the basis phases of building up the environment for the
       UI to run in. Ensures we've got an AppID, builds the session, sets up
       the socket to pass the session. And then starts the UI. */
    virtual bool run (void)
    {
        helperThread.reset();
        socketThread.reset();

        auto ui_appid = discoverUiAppid();

        if (ui_appid.empty())
        {
            g_debug("Empty UI App ID");
            return false;
        }

        socketThread = std::make_shared<MirSocketThread>(connection, appid, ui_appid);
        helperThread = std::make_shared<HelperThread>(socketThread->getSocketName(), ui_appid, buildPurchaseUrl());

        helperFinishedConnection = std::make_shared<core::Connection>(helperThread->helperFinished.connect([this](
                                                                                                               Item::Status status)
        {
            purchaseComplete(status);
        }));

        return true;
    }

private:
    /* Set at init */
    std::string appid;
    std::string itemid;

    /* Given to us by our parents */
    std::shared_ptr<MirConnection> connection;

    /* Created by run, destroyed with the object */
    std::shared_ptr<HelperThread> helperThread;
    std::shared_ptr<MirSocketThread> socketThread;
    std::shared_ptr<core::Connection> helperFinishedConnection;

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
            gchar* cacheclickdir = g_build_filename(g_get_user_cache_dir(), "pay-service", "pay-ui", nullptr);
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
            purchase_url += "/";
        }

        purchase_url += itemid;

        return purchase_url;
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

