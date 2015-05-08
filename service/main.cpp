/*
 * Copyright © 2014-2015 Canonical Ltd.
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

#include "dbus-interface.h"
#include "item-memory.h"
#include "verification-http.h"
#include "webclient-curl.h"
#include "purchase-ual.h"
#include "qtbridge.h"
#include "token-grabber-u1.h"

int
main (int argv, char* argc[])
{
    TokenGrabber::Ptr token;
    Web::Factory::Ptr wfactory;
    Web::ClickPurchasesApi::Ptr cpa;
    Verification::Factory::Ptr vfactory;
    Purchase::Factory::Ptr pfactory;
    Item::Store::Ptr items;
    DBusInterface::Ptr dbus;

    qt::core::world::build_and_run(argv, argc, [&token, &wfactory, &cpa, &vfactory, &pfactory, &items, &dbus]()
    {
        /* Initialize the other object after Qt is built */
        token = std::make_shared<TokenGrabberU1>();
        wfactory = std::make_shared<Web::CurlFactory>(token);
        cpa = std::make_shared<Web::ClickPurchasesApi>(wfactory);
        vfactory = std::make_shared<Verification::HttpFactory>(cpa);
        pfactory = std::make_shared<Purchase::UalFactory>();
        items = std::make_shared<Item::MemoryStore>(vfactory, pfactory);
        dbus = std::make_shared<DBusInterface>(items);
    });

    qt::core::world::destroy();

    return EXIT_SUCCESS;
}

