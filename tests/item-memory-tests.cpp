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
	EXPECT_EQ(0, items.size());
}
