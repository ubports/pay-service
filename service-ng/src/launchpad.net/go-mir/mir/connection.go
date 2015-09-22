/* Copyright (C) 2015 Canonical Ltd.
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

package mir

// #cgo pkg-config: mirclient
// #include <stdlib.h>
// #include <mir_toolkit/mir_connection.h>
import "C"

import (
	"fmt"
	"unsafe"
)

// Connection represents a connection to the Mir server.
type Connection interface {
	Release()
	ToMirConnection() unsafe.Pointer
}

// connection satisfies the Connection interface as a connection to the Mir
// server.
type connection struct {
	mirConnection *C.MirConnection
}

// NewConnection request (and waits for) a connection to the Mir server. Note
// that it's up to the caller to explicitly Release() this connection.
func NewConnection(serverFilePath string, clientName string) (Connection, error) {
	serverFilePathCstring := C.CString(serverFilePath)
	defer C.free(unsafe.Pointer(serverFilePathCstring))

	clientNameCstring := C.CString(clientName)
	defer C.free(unsafe.Pointer(clientNameCstring))

	newConnection := &connection{}
	newConnection.mirConnection = C.mir_connect_sync(serverFilePathCstring,
		clientNameCstring)

	if newConnection.mirConnection == nil {
		return nil, fmt.Errorf("Failed to connect to Mir Trusted Session")
	}

	return newConnection, nil
}

// Release releases the connection to the Mir server.
func (thisConnection *connection) Release() {
	C.mir_connection_release(thisConnection.mirConnection)
}

// ToMirConnection returns the raw MirConnection (in case other bindings require
// it).
func (thisConnection *connection) ToMirConnection() unsafe.Pointer {
	return unsafe.Pointer(thisConnection.mirConnection)
}
