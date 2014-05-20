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

#include <gio/gio.h>
#include <gtest/gtest.h>
#include <libdbustest/dbus-test.h>

struct LibPayTests : public ::testing::Test
{
	protected:
		DbusTestService * service = NULL;
		DbusTestDbusMock * mock = NULL;
		DbusTestDbusMockObject * obj = NULL;
		DbusTestDbusMockObject * pkgobj = NULL;
		GDBusConnection * bus = NULL;

		virtual void SetUp() {
			service = dbus_test_service_new(NULL);

			mock = dbus_test_dbus_mock_new("com.canonical.pay");

			obj = dbus_test_dbus_mock_get_object(mock, "/com/canonical/pay", "com.canonical.pay", NULL);

			dbus_test_dbus_mock_object_add_method(mock, obj,
				"ListPackages",
				nullptr,
				G_VARIANT_TYPE("ao"), /* out */
				"ret = [ dbus.ObjectPath('/com/canonical/pay/package') ]", /* python */
				NULL); /* error */

			pkgobj = dbus_test_dbus_mock_get_object(mock, "/com/canonical/pay/package", "com.canonical.pay.package", NULL);

			dbus_test_dbus_mock_object_add_method(mock, pkgobj,
				"VerifyItem",
				G_VARIANT_TYPE_STRING,
				NULL, /* out */
				"", /* python */
				NULL); /* error */

			dbus_test_dbus_mock_object_add_method(mock, pkgobj,
				"PurchaseItem",
				G_VARIANT_TYPE_STRING,
				NULL, /* out */
				"", /* python */
				NULL); /* error */

			dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));

			dbus_test_service_start_tasks(service);

			bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
			g_dbus_connection_set_exit_on_close(bus, FALSE);
			g_object_add_weak_pointer(G_OBJECT(bus), (gpointer *)&bus);
		}

		virtual void TearDown() {
			g_clear_object(&mock);
			g_clear_object(&service);

			g_object_unref(bus);

			unsigned int cleartry = 0;
			while (bus != NULL && cleartry < 100) {
				g_usleep(100000);
				while (g_main_pending())
					g_main_iteration(TRUE);
				cleartry++;
			}
		}
};

TEST_F(LibPayTests, InitTest) {

}

