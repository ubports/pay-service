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

    "github.com/godbus/dbus"
    "launchpad.net/go-mir/mir"
    "launchpad.net/go-mir/mir/fakes"
)

func setupPayUiAppIdDesktopFiles(t *testing.T) string {
    // Create temporary directory for this test.
    directory, err := ioutil.TempDir("", serviceName)
    if err != nil {
        t.Fatalf("Unexpected error when creating temporary directory: %s", err)
    }

    // Place a few non-.desktop files and a few .desktop files in the directory
    fileNames := []string{"foo_bar_0.1", "baz_qux_1.2.3",
                          "foo_bar_0.1.desktop", "baz_qux_1.2.3.desktop"}
    for _, fileName := range fileNames {
        file, _ := os.Create(path.Join(directory, fileName))
        file.Close()
    }

    return directory
}

func setupUserRuntimeDirectory(t *testing.T, directory string) {
    err := os.Setenv("XDG_RUNTIME_DIR", directory)
    if err != nil {
        t.Errorf("Unexpected error while setting $XDG_RUNTIME_DIR: %s", err)
    }
}

func setupUserCacheDirectory(t *testing.T, directory string) {
    err := os.Setenv("XDG_CACHE_HOME", directory)
    if err != nil {
        t.Errorf("Unexpected error while setting $XDG_CACHE_HOME: %s", err)
    }
}

func setupClickDirectory(t *testing.T, directory string) {
    err := os.Setenv("PAY_SERVICE_CLICK_DIR", directory)
    if err != nil {
        t.Errorf("Unexpected error while setting $PAY_SERVICE_CLICK_DIR: %s",
                 err)
    }
}

func launchFakePayUi(
    t *testing.T,
    newMirConnectionFunction func(string, string) (mir.Connection, error),
    newMirPromptSessionFunction func(mir.Connection, uint32) (mir.PromptSession, error),
    startSessionHelperFunction func(string, mir.PromptSession, string, []string) string,
    getPrimaryPidFunction func(string) uint32, failureExpectedMessage string) {
    // Redirect necessary functions to their fake versions
    payUiNewMirConnectionFunction = newMirConnectionFunction
    payUiNewMirPromptSessionFunction = newMirPromptSessionFunction
    payUiGetPrimaryPidFunction = getPrimaryPidFunction

    started := make(chan struct{})
    payUiStartSessionHelperFunction = func(a string, b mir.PromptSession,
                                           c string, d []string) string {
        close(started)
        return startSessionHelperFunction(a, b, c, d)
    }

    setupUserRuntimeDirectory(t, "/foo")

    // Prepare the user cache directory
    setupUserCacheDirectory(t, "/bar")

    // Create temporary directory to obtain Pay UI appid (results in a Pay UI
    // appid of "baz_qux_1.2.3")
    directory := setupPayUiAppIdDesktopFiles(t)
    defer func() {
        err := os.RemoveAll(directory)
        if err != nil {
            t.Errorf("Unexpected error removing temporary directory: %s", err)
        }
    }()

    setupClickDirectory(t, directory)

    // Finally, launch the fake Pay UI
    feedback := LaunchPayUi("foo", "purchase://foo/bar")

    if feedback.Finished == nil {
        t.Fatal("Feedback Finished channel is nil")
    }

    if feedback.Error == nil {
        t.Fatal("Feedback Error channel is nil")
    }

    failureExpected := (failureExpectedMessage != "")
    if !failureExpected {
        // Connect to the DBus session bus to emit a signal
        dbusConnection, err := dbus.SessionBus()
	    if err != nil {
		    t.Fatalf("Unable to connect to DBus session bus: %s", err)
	    }

	    // Wait for the helper to be started before continuing
	    <-started

	    // Spoof an Upstart EventEmitted so UAL thinks pay-ui has stopped
	    err = dbusConnection.Emit("/com/ubuntu/Upstart",
		    "com.ubuntu.Upstart0_6.EventEmitted",
		    "stopped", []string{"JOB=untrusted-helper", fmt.Sprintf("INSTANCE=%s:12345:baz_qux_1.2.3", helperName)})
	    if err != nil {
		    t.Errorf("Unexpected error emitting signal: %s", err)
	    }
	}

	// Wait until the launcher is finished
	<-feedback.Finished

	// Check for error
	err := <-feedback.Error
	if err != nil && !failureExpected {
	    t.Errorf("Unexpected error launching Pay UI: %s", err)
	} else if err == nil && failureExpected {
	    t.Error(failureExpectedMessage)
	}
}

// Test typical LaunchPayUi usage.
func TestLaunchPayUi(t *testing.T) {
    fakeConnection := fakes.NewConnection()
    fakePromptSession := fakes.NewPromptSession()

    // Redirect necessary functions to their fake versions
    newMirConnection := func(serverFilePath string,
                            clientName string) (mir.Connection, error) {
        if clientName != "pay-service" {
            t.Errorf(`Client name was "%s", expected "pay-service"`, clientName)
        }

        return fakeConnection, nil
    }

    newMirPromptSession := func(connection mir.Connection,
                               applicationPid uint32) (mir.PromptSession, error) {
        if applicationPid != 42 {
            t.Errorf("Application PID was %d, expected 42", applicationPid)
        }

        return fakePromptSession, nil
    }

    startSessionHelper := func(string, mir.PromptSession,
                              string, []string) string {
        return "12345"
    }

    getPrimaryPid := func(appId string) uint32 {
        return 42
    }

    launchFakePayUi(t, newMirConnection, newMirPromptSession,
                    startSessionHelper, getPrimaryPid, "")
}

// Test that LaunchPayUi handles a Mir connection failure as expected.
func TestLaunchPayUi_mirConnectionFailure(t *testing.T) {
    fakePromptSession := fakes.NewPromptSession()

    // Redirect necessary functions to their fake versions
    newMirConnection := func(serverFilePath string,
                            clientName string) (mir.Connection, error) {
        return nil, fmt.Errorf("Failed at user request")
    }

    newMirPromptSession := func(connection mir.Connection,
                               applicationPid uint32) (mir.PromptSession, error) {
        return fakePromptSession, nil
    }

    startSessionHelper := func(string, mir.PromptSession,
                              string, []string) string {
        return "12345"
    }

    getPrimaryPid := func(appId string) uint32 {
        return 42
    }

    launchFakePayUi(t, newMirConnection, newMirPromptSession,
                    startSessionHelper, getPrimaryPid,
                    "Expected an error due to Mir connection failure")
}

// Test that LaunchPayUi handles a Mir prompt session creation failure as
// expected.
func TestLaunchPayUi_promptSessionFailure(t *testing.T) {
    fakeConnection := fakes.NewConnection()

    // Redirect necessary functions to their fake versions
    newMirConnection := func(serverFilePath string,
                            clientName string) (mir.Connection, error) {
        return fakeConnection, nil
    }

    newMirPromptSession := func(connection mir.Connection,
                               applicationPid uint32) (mir.PromptSession, error) {
        return nil, fmt.Errorf("Failed at user request")
    }

    startSessionHelper := func(string, mir.PromptSession,
                              string, []string) string {
        return "12345"
    }

    getPrimaryPid := func(appId string) uint32 {
        return 42
    }

    launchFakePayUi(t, newMirConnection, newMirPromptSession,
                    startSessionHelper, getPrimaryPid,
                    "Expected an error due to Mir prompt session failure")
}

// Test that LaunchPayUi handles an failure to start the session helper as
// expected.
func TestLaunchPayUi_sessionHelperFailure(t *testing.T) {
    fakeConnection := fakes.NewConnection()
    fakePromptSession := fakes.NewPromptSession()

    // Redirect necessary functions to their fake versions
    newMirConnection := func(serverFilePath string,
                            clientName string) (mir.Connection, error) {
        return fakeConnection, nil
    }

    newMirPromptSession := func(connection mir.Connection,
                               applicationPid uint32) (mir.PromptSession, error) {
        return fakePromptSession, nil
    }

    startSessionHelper := func(string, mir.PromptSession,
                              string, []string) string {
        return "" // This is an error
    }

    getPrimaryPid := func(appId string) uint32 {
        return 42
    }

    launchFakePayUi(t, newMirConnection, newMirPromptSession,
                    startSessionHelper, getPrimaryPid,
                    "Expected an error due to failure to start session helper")
}

// Test that LaunchPayUi handles an failure to get the primary PID as expected.
func TestLaunchPayUi_pidError(t *testing.T) {
    fakeConnection := fakes.NewConnection()
    fakePromptSession := fakes.NewPromptSession()

    // Redirect necessary functions to their fake versions
    newMirConnection := func(serverFilePath string,
                            clientName string) (mir.Connection, error) {
        return fakeConnection, nil
    }

    newMirPromptSession := func(connection mir.Connection,
                               applicationPid uint32) (mir.PromptSession, error) {
        return fakePromptSession, nil
    }

    startSessionHelper := func(string, mir.PromptSession,
                              string, []string) string {
        return "12345"
    }

    getPrimaryPid := func(appId string) uint32 {
        return 0 // This is an error
    }

    launchFakePayUi(t, newMirConnection, newMirPromptSession,
                    startSessionHelper, getPrimaryPid,
                    "Expected an error due to failure to find primary PID")
}

// Test that getUserRuntimeDirectory works with XDG environment variables
func TestGetUserRuntimeDirectory(t *testing.T) {
    expectedRuntimeDirectory := "/foo/bar"

    setupUserRuntimeDirectory(t, expectedRuntimeDirectory)

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
    setupUserRuntimeDirectory(t, "")

    _, err := getUserRuntimeDirectory()
    if err == nil {
        t.Error("Expected an error due to invalid XDG environment")
    }
}

// Test that getUserCacheDirectory works with XDG environment variables
func TestGetUserCacheDirectory(t *testing.T) {
    expectedCacheDirectory := "/foo/bar"

    setupUserCacheDirectory(t, expectedCacheDirectory)

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
    setupUserCacheDirectory(t, "")

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
    directory := setupPayUiAppIdDesktopFiles(t)
    defer func() {
        err := os.RemoveAll(directory)
        if err != nil {
            t.Errorf("Unexpected error removing temporary directory: %s", err)
        }
    }()

    setupClickDirectory(t, directory)

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
