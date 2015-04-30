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

#include "service/verification-factory.h"

#include <thread>
#include <chrono>

#ifndef VERIFICATION_TEST_HPP__
#define VERIFICATION_TEST_HPP__ 1

namespace Verification {

class TestItem : public Item {
public:
	TestItem (bool purchased) : m_purchased(purchased) {
	}

	~TestItem (void) {
		if (t.joinable())
			t.join();
	}

	virtual bool run (void) {
		if (t.joinable())
			t.join();

		t = std::thread([this]() {
			/* Fastest website in the world */
			usleep(10 * 1000);
			verificationComplete(m_purchased ? Status::PURCHASED : Status::NOT_PURCHASED);
		});
		return true;
	}
private:
	bool m_purchased;
	std::thread t;
};

class TestFactory : public Factory {
public:
	TestFactory() : m_running(false) {
	}

	virtual bool running () {
		return m_running;
	}

	virtual Item::Ptr verifyItem (const std::string& appid, const std::string& itemid) {
		bool purchased = false;
		try {
			std::pair<std::string, std::string> key(appid, itemid);
			purchased = itemStatus.at(key);
		} catch (std::out_of_range) {
			/* If it's not there, we'll assume it's not purchased */
		}

		return std::make_shared<TestItem>(purchased);
	}

	void test_setRunning (bool running) {
		m_running = running;
	}

	void test_setPurchase (const std::string& appid, const std::string& itemid, bool purchased) {
		std::pair<std::string, std::string> key(appid, itemid);
		itemStatus[key] = purchased;
	}

	typedef std::shared_ptr<TestFactory> Ptr;
private:
	bool m_running;
	std::map<std::pair<std::string, std::string>, bool> itemStatus;
};

} // ns Verification

#endif /* VERIFICATION_TEST_HPP__ */
