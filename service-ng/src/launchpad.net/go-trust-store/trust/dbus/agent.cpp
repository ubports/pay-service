/* Copyright (C) 2015 Canonical Ltd.
 *
 * This file is part of go-trust-store.
 *
 * go-trust-store is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the Free Software Foundation.
 *
 * go-trust-store is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with go-trust-store. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kyle Fazzari <kyle@canonical.com>
 */

#include <core/dbus/dbus.h>
#include <core/dbus/asio/executor.h>

#include "agent.h"

Agent::Agent(const std::shared_ptr<core::dbus::Bus> &bus,
             const std::shared_ptr<core::trust::Agent> &agent) :
	m_bus(bus),
	m_agent(agent)
{
	m_bus->install_executor(core::dbus::asio::make_executor(m_bus));
	m_dbusThread = std::thread(std::bind(&core::dbus::Bus::run, m_bus));
}

Agent::~Agent()
{
	if (m_bus)
	{
		m_bus->stop();
		if (m_dbusThread.joinable())
		{
			m_dbusThread.join();
		}
	}
}

std::shared_ptr<core::trust::Agent> Agent::agent() const
{
	return m_agent;
}
