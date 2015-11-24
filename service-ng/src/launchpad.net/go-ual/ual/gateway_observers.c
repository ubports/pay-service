/* Copyright (C) 2015 Canonical Ltd.
 *
 * This file is part of go-ual.
 *
 * go-ual is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3, as published by the
 * Free Software Foundation.
 *
 * go-ual is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with go-ual. If not, see <http://www.gnu.org/licenses/>.
 */

#include "_cgo_export.h"
#include "gateway_observers.h"

void gatewayHelperStopObserver(const gchar *appId, const gchar *instanceId,
                               const gchar *helperType, gpointer userData)
{
    // Call the helper-stop observer dispatcher in Go
    callHelperStopObserver((char*)appId, (char*)instanceId,
                           (char*)helperType, userData);
}
