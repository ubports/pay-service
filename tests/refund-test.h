/*
 * Copyright © 2015 Canonical Ltd.
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
 */

#include "service/refund-factory.h"

#include <thread>
#include <chrono>

#ifndef REFUND_TEST_HPP__
#define REFUND_TEST_HPP__ 1

namespace Refund {

class TestItem : public Item {
public:
    TestItem (bool refunded):
        m_refunded(refunded) {
    }

    ~TestItem () {
        if (t.joinable())
            t.join();
    }

    bool run (void) override {
        if (t.joinable())
            t.join();

        t = std::thread([this]() {
            usleep(10 * 1000);
            finished(m_refunded);
        });
        return true;
    }
private:
    const bool m_refunded;
    std::thread t;
};

class TestFactory : public Factory {
public:
    TestFactory() =default;

    bool running () override {
        return m_running;
    }

    Item::Ptr refund (const std::string& appid, const std::string& itemid) override {

        bool success = false;
        auto it = itemStatus.find(std::make_pair(appid, itemid));
        if (it != itemStatus.end())
            success = it->second;

        return std::make_shared<TestItem>(success);
    }

    void test_setRunning (bool running) {
        m_running = running;
    }

    void test_setRefunded (const std::string& appid, const std::string& itemid, bool refunded) {
        auto key = std::make_pair(appid, itemid);
        itemStatus[key] = refunded;
    }

    typedef std::shared_ptr<TestFactory> Ptr;
private:

    bool m_running = false;
    std::map<std::pair<std::string, std::string>, bool> itemStatus;
};

} // ns Refund

#endif /* REFUND_TEST_HPP__ */
