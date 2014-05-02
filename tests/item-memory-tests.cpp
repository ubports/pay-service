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
#include "service/item-memory.hpp"
#include "service/verification-null.hpp"

#include "verification-test.hpp"

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
	Verification::IFactory::Ptr vfactory(dynamic_cast<Verification::IFactory *>(new Verification::NullFactory()));
	Item::MemoryStore * store = new Item::MemoryStore(vfactory);
	EXPECT_NE(nullptr, store);
	delete store;
}

/* Verify that the initial state is empty */
TEST_F(MemoryItemTests, InitialState) {
	Verification::IFactory::Ptr vfactory(dynamic_cast<Verification::IFactory *>(new Verification::NullFactory()));
	Item::IStore::Ptr store(new Item::MemoryStore(vfactory));

	auto apps = store->listApplications();
	EXPECT_EQ(0, apps.size());

	std::string appname("my-application");
	auto items = store->getItems(appname);
	EXPECT_EQ(0, items->size());
}

/* Verify that the memory store saves things */
TEST_F(MemoryItemTests, StoreItems) {
	Verification::IFactory::Ptr vfactory(dynamic_cast<Verification::IFactory *>(new Verification::NullFactory()));
	Item::IStore::Ptr store(new Item::MemoryStore(vfactory));

	auto apps = store->listApplications();
	EXPECT_EQ(0, apps.size());

	std::string appname("my-application");
	auto items = store->getItems(appname);
	EXPECT_EQ(0, items->size());

	auto apps_after = store->listApplications();
	EXPECT_EQ(1, apps_after.size());

	std::string itemname("my-item");
	auto item = store->getItem(appname, itemname);
	EXPECT_NE(nullptr, item);
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
	Verification::TestFactory::Ptr vfactory(new Verification::TestFactory());
	Item::IStore::Ptr store(new Item::MemoryStore(std::dynamic_pointer_cast<Verification::IFactory, Verification::TestFactory>(vfactory)));

	std::string appname("my-application");
	std::string itemname("my-item");
	auto item = store->getItem(appname, itemname);

	EXPECT_EQ(Item::IItem::Status::UNKNOWN, item->getStatus());

	item->verify();
	sleep(20);

	EXPECT_EQ(Item::IItem::Status::NOT_PURCHASED, item->getStatus());
}
