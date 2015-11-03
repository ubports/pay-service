/* -*- mode: go; tab-width: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

package service

import (
    "fmt"
    "strings"
    "strconv"
    "os/exec"
)

type Outputer interface {
    Output() ([]byte, error)
}

const (
    pidofExecutable = "pidof"
)

var pidofCommandRunner = pidofRunCommand

func pidofRunCommand(executable string, args ...string) ([]byte, error) {
    return exec.Command(executable, args...).Output()
}

// Pidof does the same as directly calling `pidof`, and returns a list of the
// PIDs with a given process name.
func Pidof(processName string) ([]uint32, error) {
    output, err := pidofCommandRunner(pidofExecutable, processName)
    if err != nil {
        return nil, fmt.Errorf(`Unable to get PID for process "%s": %s`,
                               processName, err)
    }

    // Clear newlines from the output
    stringOutput := strings.Replace(string(output), "\n", "", -1)

    // Split multiple PIDs by spaces
    pidStrings := strings.Split(stringOutput, " ")

    // Convert the string-form PIDs into ints
    pids := make([]uint32, len(pidStrings))
    for index, pid := range pidStrings {
        pidInt, err := strconv.ParseInt(pid, 10, 32)
        if err != nil {
            return nil, fmt.Errorf(`Unable to convert "%s" into a valid PID`,
                                   pid)
        }

        pids[index] = uint32(pidInt)
    }

    return pids, nil
}
