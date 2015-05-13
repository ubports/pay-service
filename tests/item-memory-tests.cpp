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
#include "service/refund-null.h"
#include "service/purchase-null.h"

#include "verification-test.h"
#include "purchase-test.h"

struct MemoryItemTests : public ::testing::Test
{
	protected:
		virtual void SetUp() {
		}

		virtual void TearDown() {
		}

		std::shared_ptr<Item::MemoryStore> create_null_store() {
			auto v = std::make_shared<Verification::NullFactory>();
			auto r = std::make_shared<Refund::NullFactory>();
			auto p = std::make_shared<Purchase::NullFactory>();
			EXPECT_NE(nullptr, v);
			EXPECT_NE(nullptr, r);
			EXPECT_NE(nullptr, p);
			return std::make_shared<Item::MemoryStore>(v, r, p);
		}
};

/* Test to make sure the basic stuff doesn't crash to ensure we can move forward */
TEST_F(MemoryItemTests, BasicCreate) {
	auto store = create_null_store();
	EXPECT_NE(nullptr, store);
	/* Force a destruction in the test */
	store.reset();
	EXPECT_EQ(nullptr, store);
}

/* Verify that the initial state is empty */
TEST_F(MemoryItemTests, InitialState) {
	auto store = create_null_store();

	auto apps = store->listApplications();
	EXPECT_EQ(0, apps.size());

	std::string appname("my-application");
	auto items = store->getItems(appname);
	EXPECT_EQ(0, items->size());
}

/* Verify that the memory store saves things */
TEST_F(MemoryItemTests, StoreItems) {
	auto store = create_null_store();

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
	auto rfactory = std::make_shared<Refund::NullFactory>();
	auto pfactory = std::make_shared<Purchase::NullFactory>();
	ASSERT_NE(nullptr, vfactory);
	ASSERT_NE(nullptr, rfactory);
	ASSERT_NE(nullptr, pfactory);

	auto store = std::make_shared<Item::MemoryStore>(vfactory, rfactory, pfactory);

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

	std::string pitemname("purchased-item");
	vfactory->test_setPurchase(appname, pitemname, true);
	auto pitem = store->getItem(appname, pitemname);

	ASSERT_TRUE(pitem->verify());
	usleep(50 * 1000);

	EXPECT_EQ(Item::Item::Status::PURCHASED, pitem->getStatus());
}

TEST_F(MemoryItemTests, PurchaseItemNull) {
	auto vfactory = std::make_shared<Verification::TestFactory>();
	auto rfactory = std::make_shared<Refund::NullFactory>();
	auto pfactory = std::make_shared<Purchase::NullFactory>();
	ASSERT_NE(nullptr, vfactory);
	ASSERT_NE(nullptr, pfactory);

	vfactory->test_setRunning(true);

	auto store = std::make_shared<Item::MemoryStore>(vfactory, rfactory, pfactory);

	std::string appname("my-application");
	std::string itemname("my-item");
	auto item = store->getItem(appname, itemname);

	ASSERT_NE(nullptr, item);
	ASSERT_TRUE(item->verify());
	usleep(50 * 1000);

	EXPECT_FALSE(item->purchase());
}

TEST_F(MemoryItemTests, PurchaseItem) {
	auto vfactory = std::make_shared<Verification::TestFactory>();
	auto rfactory = std::make_shared<Refund::NullFactory>();
	auto pfactory = std::make_shared<Purchase::TestFactory>();
	ASSERT_NE(nullptr, vfactory);
	ASSERT_NE(nullptr, pfactory);

	vfactory->test_setRunning(true);

	auto store = std::make_shared<Item::MemoryStore>(vfactory, rfactory, pfactory);

	std::string appname("my-application");
	std::string itemname("my-item");
	auto item = store->getItem(appname, itemname);

	ASSERT_NE(nullptr, item);
	ASSERT_TRUE(item->verify());
	usleep(50 * 1000);
	/* Make sure we're testing what we think we're going to test */
	ASSERT_EQ(Item::Item::Status::NOT_PURCHASED, item->getStatus());

	EXPECT_TRUE(item->purchase());
	usleep(50 * 1000);
	EXPECT_EQ(Item::Item::Status::NOT_PURCHASED, item->getStatus());

	/* Assume the purchase UI is a liar */
	std::string fitemname("falsely-purchased-item");
	pfactory->test_setPurchase(appname, fitemname, true);
	auto fitem = store->getItem(appname, fitemname);

	ASSERT_TRUE(fitem->verify());
	usleep(50 * 1000);
	ASSERT_TRUE(fitem->purchase());
	usleep(50 * 1000);

	EXPECT_EQ(Item::Item::Status::NOT_PURCHASED, fitem->getStatus());

	/* Legit purchase with verification */
	std::string pitemname("purchased-item");
	pfactory->test_setPurchase(appname, pitemname, true);
	vfactory->test_setPurchase(appname, pitemname, true);

	auto pitem = store->getItem(appname, pitemname);
	ASSERT_TRUE(pitem->purchase());
	usleep(50 * 1000);

	EXPECT_EQ(Item::Item::Status::PURCHASED, pitem->getStatus());
}
