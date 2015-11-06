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
 *
 * Authors:
 *   Ted Gould <ted.gould@canonical.com>
 */

#ifndef DBUS_FIXTURE_H
#define DBUS_FIXTURE_H

#include <gtest/gtest.h>

#include <gio/gio.h>

struct DBusFixture : public ::testing::Test
{
protected:
    GDBusConnection* m_bus = nullptr;

    virtual void SetUp()
    {
        BeforeBusSetUp();

        m_bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
        g_dbus_connection_set_exit_on_close(m_bus, FALSE);
        g_object_add_weak_pointer(G_OBJECT(m_bus), (gpointer*)&m_bus);
    }

    virtual void TearDown()
    {
        BeforeBusTearDown();

        g_object_unref(m_bus);

        unsigned int cleartry = 0;
        while (m_bus != nullptr && cleartry < 100)
        {
            g_usleep(100000);
            while (g_main_pending())
            {
                g_main_iteration(TRUE);
            }
            cleartry++;
        }

        ASSERT_LT(cleartry, 100);
    }

    virtual void BeforeBusSetUp() {}
    virtual void BeforeBusTearDown() {}
};

#endif // DBUS_FIXTURE_H
