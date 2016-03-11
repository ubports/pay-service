/* Copyright (C) 2015-2016 Canonical Ltd.
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
// #include "mir_prompt_session_helper.h"
// #include <stdlib.h>
// #include <mir_toolkit/mir_prompt_session.h>
import "C"

import (
	"fmt"
	"unsafe"
)

// PromptSession represents a Mir prompt session.
type PromptSession interface {
	Release()
	ToMirPromptSession() unsafe.Pointer
	GetSocketURI() (string, error)
}

// promptSession satisfies the PromptSession interface as a Mir prompt session.
type promptSession struct {
	mirPromptSession *C.MirPromptSession
}

// NewPromptSession creates and starts a new prompt session on the given
// connection, started by the given application PID.  Note that it's up to the
// caller to explicitly Release() this session.
func NewPromptSession(connection Connection, applicationPid uint32) (PromptSession, error) {
	pid := C.pid_t(applicationPid)

	session := &promptSession{}
	session.mirPromptSession =
		C.mir_connection_create_prompt_session_sync(
			(*C.MirConnection)(connection.ToMirConnection()),
			pid, nil, nil)

	if session.mirPromptSession == nil {
		return nil, fmt.Errorf("Failed to create Mir prompt session")
	}

	return session, nil
}

// Release stops and releases the prompt session.
func (session *promptSession) Release() {
	C.mir_prompt_session_release_sync(session.mirPromptSession)
}

// ToMirPromptSession returns the raw MirPromptSession (in case other bindings
// require it).
func (session *promptSession) ToMirPromptSession() unsafe.Pointer {
	return unsafe.Pointer(session.mirPromptSession)
}

// GetSocketURI returns the MIR_SOCKET value to use for the prompt session.
func (session *promptSession) GetSocketURI() (string, error) {
	fd := C.mir_prompt_session_get_fd((*C.MirPromptSession)(session.ToMirPromptSession()))
	if fd == 0 {
		return "", fmt.Errorf("Unable to get FD for prompt session.")
	}
	sockPath := fmt.Sprintf("fd://%d", fd)

	return sockPath, nil
}
