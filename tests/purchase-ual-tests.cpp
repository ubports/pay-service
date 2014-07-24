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

#include "mir-mock.h"

struct PurchaseUALTests : public ::testing::Test
{
	protected:
		DbusTestService * service = NULL;
		DbusTestDbusMock * mock = NULL;
		DbusTestDbusMockObject * obj = NULL;
		DbusTestDbusMockObject * jobobj = NULL;
		DbusTestDbusMockObject * instobj = NULL;
		GDBusConnection * bus = NULL;

		virtual void SetUp() {
			g_setenv("PAY_SERVICE_CLICK_DIR", CMAKE_SOURCE_DIR "/click-hook-data", TRUE);

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
			dbus_test_dbus_mock_object_add_method(mock, jobobj,
				"GetAllInstances",
				NULL,
				G_VARIANT_TYPE("ao"), /* out */
				"ret = [dbus.ObjectPath('/instance')]", /* python */
				NULL); /* error */
			dbus_test_dbus_mock_object_add_method(mock, jobobj,
				"GetInstanceByName",
				G_VARIANT_TYPE_STRING,
				G_VARIANT_TYPE_OBJECT_PATH, /* out */
				"ret = dbus.ObjectPath('/instance')", /* python */
				NULL); /* error */

			instobj = dbus_test_dbus_mock_get_object(mock, "/instance", "com.ubuntu.Upstart0_6.Instance", NULL);

			dbus_test_dbus_mock_object_add_property(mock, instobj,
				"processes",
				G_VARIANT_TYPE("a(si)"),
				g_variant_new_parsed("[('main', 1234)]"),
				NULL);

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

TEST_F(PurchaseUALTests, InitTest) {
	auto purchase = std::make_shared<Purchase::UalFactory>();
	EXPECT_NE(nullptr, purchase);
	purchase.reset();
	EXPECT_EQ(nullptr, purchase);
}

static gchar *
find_env (GVariant * env, const gchar * varname)
{
	GVariantIter iter;
	g_variant_iter_init(&iter, env);
	const gchar * entry = NULL;

	while (g_variant_iter_loop(&iter, "&s", &entry)) {
		if (g_str_has_prefix(entry, varname)) {
			const gchar * value = entry + strlen(varname) + 1;
			return g_strdup(value);
		}
	}

	return NULL;
}

TEST_F(PurchaseUALTests, PurchaseTest) {
	auto purchase = std::make_shared<Purchase::UalFactory>();
	ASSERT_NE(nullptr, purchase);

	/*** Purchase an item ***/
	std::string appname("click-scope");
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

	guint callcount = 0;
	auto calls = dbus_test_dbus_mock_object_get_method_calls(mock, jobobj,
		"Start", &callcount, NULL);
	ASSERT_EQ(1, callcount);

	GVariant * env = g_variant_get_child_value(calls->params, 0);

	gchar * helpertype = find_env(env, "HELPER_TYPE");
	EXPECT_STREQ("pay-ui", helpertype);
	g_free(helpertype);

	gchar * untrustedappid = find_env(env, "APP_ID");
	EXPECT_STREQ("payuihelper", untrustedappid);
	g_free(untrustedappid);

	gchar * instanceid = find_env(env, "INSTANCE_ID");
	dbus_test_dbus_mock_object_emit_signal(mock, obj, "EventEmitted", G_VARIANT_TYPE("(sas)"), g_variant_new_parsed("('stopped', ['JOB=untrusted-helper', 'INSTANCE=pay-ui:%1:payuihelper'])", instanceid), NULL);
	g_free(instanceid);

	usleep(20 * 1000);

	EXPECT_EQ(Purchase::Item::Status::PURCHASED, status);
}
