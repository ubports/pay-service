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

#ifndef GO_TRUST_STORE_REQUEST_PARAMETERS_SHIM_H
#define GO_TRUST_STORE_REQUEST_PARAMETERS_SHIM_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// Represents an application that is requesting access to a feature via the
// trust store.
typedef struct
{
	int uid;
	int pid;
	char *id;
} Application;

// Contains the information necessary to prompt the user for permission via the
// trust store.
typedef struct
{
	Application application;
	uint64_t feature;
	char *description;
} RequestParameters;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GO_TRUST_STORE_REQUEST_PARAMETERS_SHIM_H
