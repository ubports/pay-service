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
    "testing"
)

// Test that Pidof works for as expected.
func TestPidof(t *testing.T) {
    pids, err := Pidof("init")
    if err != nil {
        t.Errorf(`Unexpected error getting PID of "init": %s`, err)
    }

    if len(pids) != 1 {
        t.Fatalf("Got %d PIDs, expected only 1", len(pids))
    }

    if pids[0] != 1 {
        t.Errorf("Got PID %d, expected 1", pids[0])
    }
}

// Test that Pidof fails when given a non-running process name
func TestPidof_invalidName(t *testing.T) {
    _, err := Pidof("non-existent-process-name")
    if err == nil {
        t.Error("Expected an error due to no such process existing")
    }
}
