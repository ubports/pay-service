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

#ifndef GO_TRUST_STORE_AGENT_REQUEST_SHIM_H
#define GO_TRUST_STORE_AGENT_REQUEST_SHIM_H

#include <inttypes.h>

#include "answer_shim.h"

#ifdef __cplusplus
extern "C" {
#endif

// Not using timespec so we're not limited to 32 bits for the seconds.
typedef struct
{
	int64_t seconds;
	int32_t nanoseconds;
} Timestamp;

// A shim request.
typedef struct
{
	Answer answer;
	char *from;
	uint64_t feature;
	Timestamp when;
} Request;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GO_TRUST_STORE_AGENT_REQUEST_SHIM_H
