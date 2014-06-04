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

#include <core/posix/signal.h>

#include "dbus-interface.h"
#include "item-memory.h"
#include "verification-curl.h"
#include "purchase-ual.h"
#include "qtbridge.h"

int
main (int argv, char* argc[])
{
    qt::core::world::build_and_run(argv, argc, []() {});

    auto trap = core::posix::trap_signals_for_all_subsequent_threads(
    {
        core::posix::Signal::sig_int,
        core::posix::Signal::sig_term
    });

    trap->signal_raised().connect([trap](core::posix::Signal)
    {
        trap->stop();
    });

    auto vfactory = std::make_shared<Verification::CurlFactory>();
    auto pfactory = std::make_shared<Purchase::UalFactory>();
    auto items = std::make_shared<Item::MemoryStore>(vfactory, pfactory);
    auto dbus = std::make_shared<DBusInterface>(items);

    trap->run();

    return EXIT_SUCCESS;
}
