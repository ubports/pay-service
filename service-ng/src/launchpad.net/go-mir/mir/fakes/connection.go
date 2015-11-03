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

package fakes

// #cgo pkg-config: mirclient
// #include <stdlib.h>
// #include <mir_toolkit/mir_connection.h>
import "C"

import (
	"unsafe"
)

type fakeConnection struct {
	ReleaseCalled         bool
	ToMirConnectionCalled bool
}

func NewConnection() *fakeConnection {
	return &fakeConnection{}
}

func (fake *fakeConnection) Release() {
	fake.ReleaseCalled = true
}

func (fake *fakeConnection) ToMirConnection() unsafe.Pointer {
	fake.ToMirConnectionCalled = true
	return nil
}
