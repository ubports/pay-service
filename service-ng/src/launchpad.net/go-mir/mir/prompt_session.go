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
// #include <mir_toolkit/mir_prompt_session.h>
import "C"

import (
	"fmt"
)

// PromptSession represents a Mir prompt session.
type PromptSession struct {
	promptSession *C.MirPromptSession
}

// NewPromptSession creates and starts a new prompt session on the given
// connection, started by the given application PID.  Note that it's up to the
// caller to explicitly Release() this session.
func NewPromptSession(connection *Connection, applicationPid uint32) (*PromptSession, error) {
	pid := C.pid_t(applicationPid)

	session := &PromptSession{}
	session.promptSession =
		C.mir_connection_create_prompt_session_sync(connection.connection,
			pid, nil, nil)

	if session.promptSession == nil {
		return nil, fmt.Errorf("Failed to create Mir prompt session")
	}

	return session, nil
}

// Release stops and releases the prompt session.
func (session *PromptSession) Release() {
	C.mir_prompt_session_release_sync(session.promptSession)
}

// ToMirPromptSession returns the raw MirPromptSession (in case other bindings
// require it).
func (session *PromptSession) ToMirPromptSession() *C.MirPromptSession {
	return session.promptSession
}
