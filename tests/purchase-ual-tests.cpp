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

#include "service/purchase-ual.h"

struct VerificationCurlTests : public ::testing::Test
{
	protected:
		DbusTestService * service = NULL;
		DbusTestDbusMock * mock = NULL;
		DbusTestDbusMockObject * obj = NULL;
		DbusTestDbusMockObject * jobobj = NULL;
		GDBusConnection * bus = NULL;

		virtual void SetUp() {
			service = dbus_test_service_new(NULL);

			mock = dbus_test_dbus_mock_new("com.ubuntu.Upstart");
			obj = dbus_test_dbus_mock_get_object(mock, "/com/ubuntu/Upstart", "com.ubuntu.Upstart0_6", NULL);

			dbus_test_dbus_mock_object_add_method(mock, obj,
				"GetJobByName",
				G_VARIANT_TYPE_STRING,
				G_VARIANT_TYPE_OBJECT_PATH, /* out */
				"ret = dbus.ObjectPath('/job')", /* python */
				NULL); /* error */

			jobobj = dbus_test_dbus_mock_get_object(mock, "/job", "com.ubuntu.Upstart0_6.Job", NULL);

			dbus_test_dbus_mock_object_add_method(mock, jobobj,
				"Start",
				G_VARIANT_TYPE("(asb)"),
				G_VARIANT_TYPE_OBJECT_PATH, /* out */
				"ret = dbus.ObjectPath('/instance')", /* python */
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

TEST_F(VerificationCurlTests, InitTest) {
	auto purchase = std::make_shared<Purchase::UalFactory>();
	EXPECT_NE(nullptr, purchase);
	purchase.reset();
	EXPECT_EQ(nullptr, purchase);
}

TEST_F(VerificationCurlTests, PurchaseTest) {
	auto purchase = std::make_shared<Purchase::UalFactory>();
	ASSERT_NE(nullptr, purchase);

	std::string appname("application");
	std::string itemname("item");
	auto item = purchase->purchaseItem(appname, itemname);

	ASSERT_NE(nullptr, item);

	Purchase::Item::Status status = Purchase::Item::Status::ERROR;
	item->purchaseComplete.connect([&status](Purchase::Item::Status in_status) {
		std::cout << "Purchase Status Callback: " << in_status << std::endl;
		status = in_status;
	});

	EXPECT_TRUE(item->run());
	usleep(20 * 1000);

	dbus_test_dbus_mock_object_emit_signal(mock, obj, "EventEmitted", G_VARIANT_TYPE("(sas)"), g_variant_new_parsed("('stopped', ['JOB=application-legacy', 'INSTANCE=gedit-'])"), NULL);
	usleep(20 * 1000);

	EXPECT_EQ(Purchase::Item::Status::PURCHASED, status);
}
