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

#include "service/verification-factory.hpp"

#include <thread>
#include <chrono>

#ifndef VERIFICATION_NULL_HPP__
#define VERIFICATION_NULL_HPP__ 1

namespace Verification {

class TestItem : public IItem {
public:
	TestItem (bool purchased) : m_purchased(purchased) {
	}

	~TestItem (void) {
	}

	virtual bool run (void) {
		t = std::thread([this]() {
			/* Fastest website in the world */
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			verificationComplete(purchased ? Status::PURCHASED : Status::NOT_PURCHASED);
		});
		return true;
	}
private:
	bool m_purchased;
	std::thread t;
};

class TestFactory {
public:
	TestFactory : m_running(false) {
	}

	virtual bool running () {
		return m_running;
	}

	virtual IItem::Ptr verifyItem (std::string& appid, std::string& itemid) {
		bool purchased = false;
		try {
			std::pair<std::string, std::string> key(appid, itemid);
			purchased = itemStatus.at(key);
		} catch (std::out_of_range) {
			/* If it's not there, we'll assume it's not purchased */
		}

		return IItem::Ptr(new TestItem(purchased));
	}

	void test_setRunning (bool running) {
		m_running = running;
	}

	void test_setPurchase (std::string& appid, std::string& itemid, bool purchased) {
		std::pair<std::string, std::string> key(appid, itemid);
		itemStatus[key] = purchased;
	}

private:
	bool m_running;
	std::map<std::pair<std::string, std::string>, bool> itemStatus;
};

} // ns Verification

#endif /* VERIFICATION_NULL_HPP__ */
