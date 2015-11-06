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
    "errors"
    "testing"
)

func pidofTest_fakeCommandRunner_singleResult(executable string, args ...string) ([]byte, error) {
    return []byte("42"), nil
}

func pidofTest_fakeCommandRunner_multipleResults(executable string, args ...string) ([]byte, error) {
    return []byte("42 43"), nil
}

func pidofTest_fakeCommandRunner_invalidResults(executable string, args ...string) ([]byte, error) {
    return []byte("hello"), nil // Cannot be converted to int
}

func pidofTest_fakeCommandRunner_error(executable string, args ...string) ([]byte, error) {
    return nil, errors.New("Failed at user request")
}

// Test that Pidof works for as expected for single results
func TestPidof_singleResult(t *testing.T) {
    pidofCommandRunner = pidofTest_fakeCommandRunner_singleResult

    pids, err := Pidof("foo")
    if err != nil {
        t.Errorf(`Unexpected error getting PID of "foobar": %s`, err)
    }

    if len(pids) != 1 {
        t.Fatalf("Got %d PIDs, expected only 1", len(pids))
    }

    if pids[0] != 42 {
        t.Errorf("Got PID %d, expected 42", pids[0])
    }
}

// Test that Pidof works for as expected for multiple results
func TestPidof_multipleResults(t *testing.T) {
    pidofCommandRunner = pidofTest_fakeCommandRunner_multipleResults

    pids, err := Pidof("foo")
    if err != nil {
        t.Errorf(`Unexpected error getting PID of "foobar": %s`, err)
    }

    if len(pids) != 2 {
        t.Fatalf("Got %d PIDs, expected 2", len(pids))
    }

    if pids[0] != 42 {
        t.Errorf("Got PID %d, expected 42", pids[0])
    }

    if pids[1] != 43 {
        t.Errorf("Got PID %d, expected 43", pids[1])
    }
}

// Test that Pidof works for as expected for invalid results
func TestPidof_invalidResults(t *testing.T) {
    pidofCommandRunner = pidofTest_fakeCommandRunner_invalidResults

    _, err := Pidof("foo")
    if err == nil {
        t.Error("Expected an error due to invalid PID")
    }
}

// Test that Pidof fails when pidof doesn't find any PIDs (i.e. process isn't
// running)
func TestPidof_invalidName(t *testing.T) {
    pidofCommandRunner = pidofTest_fakeCommandRunner_error

    _, err := Pidof("foo")
    if err == nil {
        t.Error("Expected an error due to pidof failure")
    }
}
