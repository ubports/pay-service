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
    "os"
    "os/user"
    "fmt"
    "path"
    "io/ioutil"
    "testing"
)

// Test that getUserRuntimeDirectory works with XDG environment variables
func TestGetUserRuntimeDirectory(t *testing.T) {
    expectedRuntimeDirectory := "/foo/bar"

    err := os.Setenv("XDG_RUNTIME_DIR", expectedRuntimeDirectory)
    if err != nil {
        t.Errorf("Unexpected error while setting $XDG_RUNTIME_DIR: %s", err)
    }

    directory, err := getUserRuntimeDirectory()
    if err != nil {
        t.Errorf("Unexpected error getting user runtime directory: %s", err)
    }

    if directory != expectedRuntimeDirectory {
        t.Errorf(`User runtime directory was "%s", expected "%s"`, directory,
                 expectedRuntimeDirectory)
    }
}

// Test that getUserRuntimeDirectory fails without XDG environment variables
func TestGetUserRuntimeDirectory_withoutXDG(t *testing.T) {
    // Make sure XDG environment variable isn't set
    err := os.Setenv("XDG_RUNTIME_DIR", "")
    if err != nil {
        t.Errorf("Unexpected error while setting $XDG_RUNTIME_DIR: %s", err)
    }

    _, err = getUserRuntimeDirectory()
    if err == nil {
        t.Error("Expected an error due to invalid XDG environment")
    }
}

// Test that getUserCacheDirectory works with XDG environment variables
func TestGetUserCacheDirectory(t *testing.T) {
    expectedCacheDirectory := "/foo/bar"

    err := os.Setenv("XDG_CACHE_HOME", expectedCacheDirectory)
    if err != nil {
        t.Errorf("Unexpected error while setting $XDG_CACHE_HOME: %s", err)
    }

    directory, err := getUserCacheDirectory()
    if err != nil {
        t.Errorf("Unexpected error getting user cache directory: %s", err)
    }

    if directory != expectedCacheDirectory {
        t.Errorf(`User cache directory was "%s", expected "%s"`, directory,
                 expectedCacheDirectory)
    }
}

// Test that getUserCacheDirectory uses fallback without XDG environment
// variables
func TestGetUserCacheDirectory_withoutXDG(t *testing.T) {
    currentUser, err := user.Current()
    if err != nil {
        t.Errorf("Unable to get current user: %s", err)
    }

    expectedCacheDirectory := path.Join(currentUser.HomeDir, ".cache")

    // Make sure XDG environment variable isn't set
    err = os.Setenv("XDG_CACHE_HOME", "")
    if err != nil {
        t.Errorf("Unexpected error while setting $XDG_CACHE_HOME: %s", err)
    }

    directory, err := getUserCacheDirectory()
    if err != nil {
        t.Errorf("Unexpected error getting user cache directory: %s", err)
    }

    if directory != expectedCacheDirectory {
        t.Errorf(`User cache directory was "%s", expected "%s"`, directory,
                 expectedCacheDirectory)
    }
}

// Test that getPayUiAppId finds the first .desktop file in the given directory
func TestGetPayUiAppId(t *testing.T) {
    // Create temporary directory for this test.
    directory, err := ioutil.TempDir("", serviceName)
    if err != nil {
        t.Fatalf("Unexpected error when creating temporary directory: %s", err)
    }
    defer func() {
        err = os.RemoveAll(directory)
        if err != nil {
            t.Errorf("Unexpected error removing temporary directory: %s", err)
        }
    }()

    // Place a few non-.desktop files and a few .desktop files in the directory
    fileNames := []string{"foo_bar_0.1", "baz_qux_1.2.3", "foo_bar_0.1.desktop",
                          "baz_qux_1.2.3.desktop"}
    for _, fileName := range fileNames {
        file, _ := os.Create(path.Join(directory, fileName))
        file.Close()
    }

    err = os.Setenv("PAY_SERVICE_CLICK_DIR", directory)
    appId, err := getPayUiAppId()
    if err != nil {
        t.Errorf("Unexpected error getting pay UI app ID: %s", err)
    }

    if appId != "baz_qux_1.2.3" {
        t.Errorf(`appId was "%s", expected "baz_qux_1.2.3"`, appId)
    }
}

// Test typical getAppPid usage.
func TestGetAppPid(t *testing.T) {
    called := false
    payUiGetPrimaryPidFunction = func(appId string) uint32 {
        called = true
        if appId != "foo" {
            t.Errorf(`appId was "%s", expected "foo"`, appId)
        }

        return 42
    }

    _, err := getAppPid("foo")
    if err != nil {
        t.Errorf("Unexpected error getting PID: %s", err)
    }

    if !called {
        t.Error("Expected GetPrimaryPid to be called")
    }
}

// Test that getAppPid returns the PID of unity8-dash for the click scope
func TestGetAppPid_clickScope(t *testing.T) {
    called := false
    pidofCommandRunner = func(executable string, args ...string) ([]byte, error) {
        called = true
        if len(args) != 1 {
            t.Fatalf("Got %d args, expected 1", len(args))
        }

        if args[0] != "unity8-dash" {
            t.Errorf(`Arg was "%s", expected "unity8-dash"`, args[0])
        }

        return []byte("42"), nil
    }

    _, err := getAppPid("click-scope")
    if err != nil {
        t.Errorf("Unexpected error getting PID: %s", err)
    }

    if !called {
        t.Error("Expected Pidof to be called")
    }
}

// Test getAppPid when UAL can't find the requested application.
func TestGetAppPid_ualNotFound(t *testing.T) {
    called := false
    payUiGetPrimaryPidFunction = func(appId string) uint32 {
        called = true
        return 0
    }

    _, err := getAppPid("foo")
    if err == nil {
        t.Error("Expected an error getting PID due to PID of 0 from UAL")
    }

    if !called {
        t.Error("Expected GetPrimaryPid to be called")
    }
}

// Test getAppPid when Pidof can't find the PID of unity8-dash.
func TestGetAppPid_pidofNotFound(t *testing.T) {
    called := false
    pidofCommandRunner = func(executable string, args ...string) ([]byte, error) {
        called = true
        return nil, fmt.Errorf("Failed at user request")
    }

    _, err := getAppPid("click-scope")
    if err == nil {
        t.Error("Expected an error due to Pidof failure")
    }

    if !called {
        t.Error("Expected Pidof to be called")
    }
}

// Test getAppPid when Pidof simply returns no results for unity8-dash.
func TestGetAppPid_pidofNoResults(t *testing.T) {
    called := false
    pidofCommandRunner = func(executable string, args ...string) ([]byte, error) {
        called = true
        return make([]byte, 0), nil
    }

    _, err := getAppPid("click-scope")
    if err == nil {
        t.Error("Expected an error due to Pidof returning no results")
    }

    if !called {
        t.Error("Expected Pidof to be called")
    }
}
