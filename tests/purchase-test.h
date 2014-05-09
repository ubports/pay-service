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

#include "service/purchase-factory.h"

#include <thread>
#include <chrono>

#ifndef PURCHASE_TEST_HPP__
#define PURCHASE_TEST_HPP__ 1

namespace Purchase {

class TestItem : public Item {
public:
	TestItem (bool purchased) : m_purchased(purchased) {
	}

	~TestItem (void) {
		if (t.joinable())
			t.join();
	}

	virtual bool run (void) {
		t = std::thread([this]() {
			/* Fastest website in the world */
			usleep(10 * 1000);
			purchaseComplete(m_purchased ? Status::PURCHASED : Status::NOT_PURCHASED);
		});
		return true;
	}
private:
	bool m_purchased;
	std::thread t;
};

class TestFactory : public Factory {
public:
	virtual Item::Ptr purchaseItem (std::string& appid, std::string& itemid) {
		bool purchased = false;
		try {
			std::pair<std::string, std::string> key(appid, itemid);
			purchased = itemStatus.at(key);
		} catch (std::out_of_range) {
			/* If it's not there, we'll assume it's not purchased */
		}

		return std::make_shared<TestItem>(purchased);
	}

	void test_setPurchase (std::string& appid, std::string& itemid, bool purchased) {
		std::pair<std::string, std::string> key(appid, itemid);
		itemStatus[key] = purchased;
	}

	typedef std::shared_ptr<TestFactory> Ptr;
private:
	std::map<std::pair<std::string, std::string>, bool> itemStatus;
};

} // ns Purchase

#endif /* PURCHASE_TEST_HPP__ */
