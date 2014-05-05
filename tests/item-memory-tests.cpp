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

#include <gtest/gtest.h>
#include "service/item-memory.h"
#include "service/verification-null.h"

#include "verification-test.h"

struct MemoryItemTests : public ::testing::Test
{
	protected:
		virtual void SetUp() {
		}

		virtual void TearDown() {
		}
};

/* Test to make sure the basic stuff doesn't crash to ensure we can move forward */
TEST_F(MemoryItemTests, BasicCreate) {
	auto vfactory = std::make_shared<Verification::NullFactory>();
	ASSERT_NE(nullptr, vfactory);

	auto store = std::make_shared<Item::MemoryStore>(std::dynamic_pointer_cast<Verification::Factory, Verification::NullFactory>(vfactory));
	EXPECT_NE(nullptr, store);
	/* Force a destruction in the test */
	store.reset();
	EXPECT_EQ(nullptr, store);
}

/* Verify that the initial state is empty */
TEST_F(MemoryItemTests, InitialState) {
	auto vfactory = std::make_shared<Verification::NullFactory>();
	auto store = std::make_shared<Item::MemoryStore>(std::dynamic_pointer_cast<Verification::Factory, Verification::NullFactory>(vfactory));

	auto apps = store->listApplications();
	EXPECT_EQ(0, apps.size());

	std::string appname("my-application");
	auto items = store->getItems(appname);
	EXPECT_EQ(0, items->size());
}

/* Verify that the memory store saves things */
TEST_F(MemoryItemTests, StoreItems) {
	auto vfactory = std::make_shared<Verification::NullFactory>();
	ASSERT_NE(nullptr, vfactory);

	auto store = std::make_shared<Item::MemoryStore>(std::dynamic_pointer_cast<Verification::Factory, Verification::NullFactory>(vfactory));

	auto apps = store->listApplications();
	EXPECT_EQ(0, apps.size());

	std::string appname("my-application");
	auto items = store->getItems(appname);
	EXPECT_EQ(0, items->size());

	auto apps_after = store->listApplications();
	EXPECT_EQ(1, apps_after.size());

	std::string itemname("my-item");
	auto item = store->getItem(appname, itemname);
	ASSERT_NE(nullptr, item);
	EXPECT_EQ(itemname, item->getId());

	auto items_after = store->getItems(appname);
	EXPECT_EQ(1, items_after->size());
	EXPECT_EQ(items, items_after);

	auto item_later = store->getItem(appname, itemname);
	EXPECT_NE(nullptr, item_later);
	EXPECT_EQ(itemname, item_later->getId());
	EXPECT_EQ(item, item_later);
}

TEST_F(MemoryItemTests, VerifyItem) {
	auto vfactory = std::make_shared<Verification::TestFactory>();
	ASSERT_NE(nullptr, vfactory);

	auto store = std::make_shared<Item::MemoryStore>(std::dynamic_pointer_cast<Verification::Factory, Verification::TestFactory>(vfactory));

	std::string appname("my-application");
	std::string itemname("my-item");
	auto item = store->getItem(appname, itemname);

	ASSERT_NE(nullptr, item);
	EXPECT_EQ(Item::Item::Status::UNKNOWN, item->getStatus());

	/* It's not running yet */
	EXPECT_FALSE(item->verify());

	vfactory->test_setRunning(true);
	ASSERT_TRUE(item->verify());
	usleep(50 * 1000);

	EXPECT_EQ(Item::Item::Status::NOT_PURCHASED, item->getStatus());
}
