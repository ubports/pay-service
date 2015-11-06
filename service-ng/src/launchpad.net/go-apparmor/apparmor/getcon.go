/* Copyright (C) 2015 Canonical Ltd.
 *
 * This file is part of go-apparmor.
 *
 * go-apparmor is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * go-apparmor is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with go-apparmor. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kyle Fazzari <kyle@canonical.com>
 */

package apparmor

// #cgo pkg-config: libapparmor
// #include <stdlib.h>
// #include <sys/apparmor.h>
import "C"

import (
	"os"
	"unsafe"
)

type Pid uint32

// GetCon gets the current AppArmor confinement context for the current task.
// The confinement context is usually just the name of the AppArmor profile
// restricting the task, but it may include the profile namespace or in some
// cases a set of profile names (known as a stack of profiles).
func GetCon() (con string, mode string, err error) {
	var conCstring *C.char
	var modeCstring *C.char

	_, err = C.aa_getcon(&conCstring, &modeCstring)
	if err != nil {
		return
	}

	// According to man aa_getcon, caller must free `con`, but not `mode`.
	defer C.free(unsafe.Pointer(conCstring))

	con = C.GoString(conCstring)
	mode = C.GoString(modeCstring)
	return
}

// GetTaskCon is similar to GetCon() except it will work for any arbitrary task
// in the system.
func GetTaskCon(taskPid Pid) (con string, mode string, err error) {
	var conCstring *C.char
	var modeCstring *C.char

	_, err = C.aa_gettaskcon(C.pid_t(taskPid), &conCstring, &modeCstring)
	if err != nil {
		return
	}

	// According to man aa_gettaskcon, caller must free `con`, but not `mode`.
	defer C.free(unsafe.Pointer(conCstring))

	con = C.GoString(conCstring)
	mode = C.GoString(modeCstring)
	return
}

// GetPeerCon is similar to GetTaskCon() except that it returns the confinement
// information for the task on the other end of a socket connection.
func GetPeerCon(file *os.File) (con string, mode string, err error) {
	var conCstring *C.char
	var modeCstring *C.char

	_, err = C.aa_getpeercon(C.int(file.Fd()), &conCstring, &modeCstring)
	if err != nil {
		return
	}

	// According to man aa_getpeercon, caller must free `con`, but not `mode`.
	defer C.free(unsafe.Pointer(conCstring))

	con = C.GoString(conCstring)
	mode = C.GoString(modeCstring)
	return
}
