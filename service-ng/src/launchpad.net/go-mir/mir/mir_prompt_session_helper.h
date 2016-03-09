/* Copyright (C) 2016 Canonical Ltd.
 *
 * This file is part of go-mir.
 *
 * go-mir is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3, as published by the
 * Free Software Foundation.
 *
 * go-mir is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with go-mir. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GATEWAY_OBSERVERS_H
#define GATEWAY_OBSERVERS_H

#include <glib.h>
#include <mir_toolkit/mir_prompt_session.h>

/* Get an fd to use for the MIR_SOCKET for a trusted prompt session.
 */
int mir_prompt_session_get_fd(MirPromptSession * session);

#endif
