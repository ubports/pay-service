#include <gtest/gtest.h>
#include "service/item-memory.hpp"

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
	Item::MemoryStore * store = new Item::MemoryStore();
	EXPECT_NE(nullptr, store);
	delete store;
}

/* Verify that the initial state is empty */
TEST_F(MemoryItemTests, InitialState) {
	Item::IStore::Ptr store(new Item::MemoryStore());

	auto apps = store->listApplications();
	EXPECT_EQ(0, apps.size());

	std::string appname("my-application");
	auto items = store->getItems(appname);
	EXPECT_EQ(0, items->size());
}

/* Verify that the memory store saves things */
TEST_F(MemoryItemTests, StoreItems) {
	Item::IStore::Ptr store(new Item::MemoryStore());

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
}
