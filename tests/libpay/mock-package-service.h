/*
 * Copyright © 2014-2015 Canonical Ltd.
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

#ifndef MOCK_PACKAGE_SERVICE_H
#define MOCK_PACKAGE_SERVICE_H

#include <libdbustest/dbus-test.h>

#include <gio/gio.h>

#include <string>

class MockPackageService
{
    DbusTestService* m_service = nullptr;
    DbusTestDbusMock* m_mock = nullptr;
    DbusTestDbusMockObject* m_obj = nullptr;
    DbusTestDbusMockObject* m_pkgobj = nullptr;

public:

    MockPackageService()
    {
        m_service = dbus_test_service_new(nullptr);

        m_mock = dbus_test_dbus_mock_new("com.canonical.pay");

        m_obj = dbus_test_dbus_mock_get_object(m_mock, "/com/canonical/pay", "com.canonical.pay", nullptr);

        dbus_test_dbus_mock_object_add_method(m_mock, m_obj,
                                              "ListPackages",
                                              nullptr,
                                              G_VARIANT_TYPE("ao"), /* out */
                                              "ret = [ dbus.ObjectPath('/com/canonical/pay/package_2Dname') ]", /* python */
                                              nullptr); /* error */

        m_pkgobj = dbus_test_dbus_mock_get_object(m_mock,
                                                  "/com/canonical/pay/package_2Dname",
                                                  "com.canonical.pay.package",
                                                  nullptr);

        dbus_test_dbus_mock_object_add_method(m_mock, m_pkgobj,
                                              "VerifyItem",
                                              G_VARIANT_TYPE_STRING,
                                              nullptr, /* out */
                                              "", /* python */
                                              nullptr); /* error */

        dbus_test_dbus_mock_object_add_method(m_mock, m_pkgobj,
                                              "PurchaseItem",
                                              G_VARIANT_TYPE_STRING,
                                              nullptr, /* out */
                                              "", /* python */
                                              nullptr); /* error */

        dbus_test_dbus_mock_object_add_method(m_mock, m_pkgobj,
                                              "RefundItem",
                                              G_VARIANT_TYPE_STRING,
                                              nullptr, /* out */
                                              "", /* python */
                                              nullptr); /* error */

        dbus_test_service_add_task(m_service, DBUS_TEST_TASK(m_mock));
        dbus_test_service_start_tasks(m_service);
    }

    ~MockPackageService()
    {
        g_clear_object(&m_mock);
        g_clear_object(&m_service);
    }

    void emit_item_status_changed(const std::string& item, const std::string& status, time_t refund, GError** error)
    {
        dbus_test_dbus_mock_object_emit_signal(m_mock, m_pkgobj,
                                               "ItemStatusChanged",
                                               G_VARIANT_TYPE("(sst)"),
                                               g_variant_new("(sst)", item.c_str(), status.c_str(), (guint64)refund),
                                               error);
    }

    const DbusTestDbusMockCall* get_method_calls(const std::string& method, guint* len, GError** error)
    {
        return dbus_test_dbus_mock_object_get_method_calls(m_mock,
                                                           m_pkgobj,
                                                           method.c_str(),
                                                           len,
                                                           error);
    }
};

#endif // MOCK_PACKAGE_SERVICE_H
