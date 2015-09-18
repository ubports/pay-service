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
    "io/ioutil"
    "fmt"
    "path"
    "path/filepath"

    "github.com/ziutek/glib"
    "launchpad.net/go-mir/mir"
    "launchpad.net/go-ual/ual"
)

const (
    mirTrustedSocketName = "mir_socket_trusted"
    serviceName = "pay-service"
    helperName = "pay-ui"
)

var (
    payUiGetPrimaryPidFunction = ual.GetPrimaryPid
)

// PayUiFeedback allows the LaunchPayUi caller to keep track of Pay UI's status.
type PayUiFeedback struct {
    Finished chan struct{}
    Error chan error
}

// LaunchPayUi launches Pay UI in a Mir trusted session, handing it the provided
// appId and purchase URL. This is done asynchronously; this function returns
// immediately, and provides a PayUiFeedback handle that can be used to track
// the status of the launched Pay UI instance.
func LaunchPayUi(appId string, purchaseUrl string) PayUiFeedback {
    feedback := PayUiFeedback{
        Finished: make(chan struct{}),
        Error: make(chan error, 1),
    }
    go launchPayUiAndWait(appId, purchaseUrl, feedback)

    return feedback
}

// launchPayUiAndWait is the synchronous worker function that actually
// launches Pay UI via Ubuntu App Launch (UAL) and provides the associated
// feedback.
func launchPayUiAndWait(appId string, purchaseUrl string, feedback PayUiFeedback) {
    // Fire up glib (it's how UAL handles events)
    glibMainLoop := glib.NewMainLoop(nil)
    glibDone := make(chan struct{})
    go func() {
        glibMainLoop.Run()
        close(glibDone)
    }()

    // When this function ends Pay UI will have been closed, so make sure the
    // glib mainloop quits and we send the correct feedback.
    defer func() {
        // Ask glib mainloop to quit and wait for it
        glibMainLoop.Quit()
        <-glibDone
        close(feedback.Error)
        close(feedback.Finished)
    }()

    // The Mir trusted socket is in the user runtime directory
    runtimeDirectory, err := getUserRuntimeDirectory()
    if err != nil {
        feedback.Error <- fmt.Errorf("Unable to get user runtime directory: %s",
                                     err)
        return
    }

    // Connect to Mir trusted session
    connection, err := mir.NewConnection(path.Join(runtimeDirectory,
                                                   mirTrustedSocketName),
                                         serviceName)
    if err != nil {
        feedback.Error <- fmt.Errorf("Unable to create Mir connection: %s", err)
        return
    }
    defer connection.Release()

    // Get app ID for Pay UI
    uiAppId, err := getPayUiAppId()
    if err != nil {
        feedback.Error <- fmt.Errorf("Unable to obtain Pay UI app ID: %s", err)
        return
    }

    // The Mir prompt session will be displayed over a specific process, so
    // we need to get the PID of the app that wants to launch Pay UI.
    pidToOverlay, err := getAppPid(appId)
    if err != nil {
        feedback.Error <- fmt.Errorf(`Unable to get PID for "%s": %s`, appId,
                                     err)
        return
    }

    // Now start the Mir prompt session, asking to overlay on that specific PID.
    session, err := mir.NewPromptSession(connection, pidToOverlay)
    if err != nil {
        feedback.Error <- fmt.Errorf(
            `Unable launch prompt session for "%s": %s`, appId, err)
        return
    }
    defer session.Release()

    // Add an observer to UAL that will be run when Pay UI stops
    helperDone := make(chan struct{})
    var instanceId string
    observerId, err := ual.ObserverAddHelperStop(helperName,
        func(stoppedAppId, stoppedInstanceId, stoppedHelperType string) {
            // Make sure the helper that stopped was the one we started. If it
            // wasn't, we'll continue waiting.
            if (stoppedAppId == uiAppId) && (stoppedInstanceId == instanceId) {
                close(helperDone)
            }
        })
    if err != nil {
        feedback.Error <- fmt.Errorf("Unable to add UAL observer: %s", err)
        return
    }
    defer ual.ObserverDeleteHelperStop(observerId)

    // Finally, start the helper with that purchase URL
    instanceId = ual.StartSessionHelper(helperName, session, uiAppId,
                                        []string{purchaseUrl})
    if instanceId == "" {
        feedback.Error <- fmt.Errorf(`Failed to start helper "%s"`, uiAppId)
        return
    }

    <-helperDone // Wait for helper to stop
}

// getUserRuntimeDirectory uses the $XDG_RUNTIME_DIR environment variable to
// return the user runtime directory.
func getUserRuntimeDirectory() (string, error) {
    runtimeDirectory := os.Getenv("XDG_RUNTIME_DIR")
    if runtimeDirectory == "" {
        return "", fmt.Errorf("XDG_RUNTIME_DIR environment variable not defined")
    }

    return runtimeDirectory, nil
}

// getUserCacheDirectory tries to find the user cache directory by first
// checking the $XDG_CACHE_HOME environment variable, and then trying
// $HOME/.cache.
func getUserCacheDirectory() (string, error) {
    cacheDirectory := os.Getenv("XDG_CACHE_HOME")
    if cacheDirectory == "" {
        currentUser, err := user.Current()
        if err != nil {
            return "", fmt.Errorf("Unable to get current user: %s", err)
        }

        if currentUser.HomeDir == "" {
            return "", fmt.Errorf("Unable to find user's home directory")
        }

        cacheDirectory = path.Join(currentUser.HomeDir, ".cache")
    }

    return cacheDirectory, nil
}

// getPayUiAppId looks through a directory to find the first entry that is a
// .desktop file and uses that as our AppID. We don't support more than one
// entry being in a directory.
func getPayUiAppId() (string, error) {
    directory := os.Getenv("PAY_SERVICE_CLICK_DIR")
    if directory == "" {
        var err error
        directory, err = getUserCacheDirectory()
        if err != nil {
         return "", fmt.Errorf("Unable to obtain user cache directory: %s", err)
        }

        directory = path.Join(directory, serviceName, helperName)
    }

    files, _ := ioutil.ReadDir(directory)
    for _, file := range files {
        fileName := file.Name()
        extension := filepath.Ext(fileName)
        if extension == ".desktop" {
            return fileName[0:len(fileName)-len(extension)], nil
        }
    }

    return "", fmt.Errorf(`Failed to find .desktop file in "%s"`, directory)
}

// getAppPid attempts to obtain the PID of the given appId. Note that the only
// scope that is supported is the click scope.
func getAppPid(appId string) (uint32, error) {
    // This has the same FIXME as pay-service: Before other scopes can use pay,
    // we'll need to figure out how to detect that they're actually scopes.
    // For now we'll only support the click-scope.
    if appId == "click-scope" {
        pids, err := Pidof("unity8-dash")
        if err != nil {
            return 0, fmt.Errorf(`Unable to get PID of "unity8-dash": %s`, err)
        }

        if len(pids) == 0 {
            return 0, fmt.Errorf(`Expected at least 1 PID for "unity8-dash"`)
        }

        // Simply take the first one, as pay-service does (there shouldn't be
        // more than one)
        return pids[0], nil
    } else {
        pid := payUiGetPrimaryPidFunction(appId)
        if pid == 0 {
            return 0, fmt.Errorf(
                `Unable to get PID of "%s": No such application is running`,
                appId)
        }

        return pid, nil
    }
}
