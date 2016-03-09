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
    "path"
    "os"
    "os/exec"
    "os/user"

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
    payUiGetAppIdFunction = ual.TripletToAppId
    payUiNewMirConnectionFunction = mir.NewConnection
    payUiNewMirPromptSessionFunction = mir.NewPromptSession
    payUiExecFunction = execPayUiCommand
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

// execPayUiCommand is a replaceable call to do the actual running of pay-ui
func execPayUiCommand(purchaseUrl string) {
    exec.Command("/usr/lib/payui/pay-ui", purchaseUrl).Run()
}

// launchPayUiAndWait is the synchronous worker function that actually
// launches Pay UI via Ubuntu App Launch (UAL) and provides the associated
// feedback.
func launchPayUiAndWait(appId string, purchaseUrl string, feedback PayUiFeedback) {
    // Fire up glib (it's how UAL handles events)
    glibStop := make(chan struct{})
    glibFinished := make(chan struct{})
    go runGlib(glibStop, glibFinished)

    // When this function ends Pay UI will have been closed, so make sure the
    // glib mainloop quits and we send the correct feedback.
    defer func() {
        // Ask glib to quit and wait for it
        close(glibStop)
        <-glibFinished

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
    connection, err :=
        payUiNewMirConnectionFunction(path.Join(runtimeDirectory,
                                                mirTrustedSocketName),
                                      serviceName)
    if err != nil {
        feedback.Error <- fmt.Errorf("Unable to create Mir connection: %s", err)
        return
    }
    defer connection.Release()

    // The Mir prompt session will be displayed over a specific process, so
    // we need to get the PID of the app that wants to launch Pay UI.
    pidToOverlay, err := getAppPid(appId)
    if err != nil {
        feedback.Error <- fmt.Errorf(`Unable to get PID for "%s": %s`, appId,
                                     err)
        return
    }

    // Now start the Mir prompt session, asking to overlay on that specific PID.
    session, err := payUiNewMirPromptSessionFunction(connection, pidToOverlay)
    if err != nil {
        feedback.Error <- fmt.Errorf(
            `Unable launch prompt session for "%s": %s`, appId, err)
        return
    }
    defer session.Release()

    // Get the socket path and set the MIR_SOCKET variable
    sockPath, sockErr := session.GetSocketURI()
    if sockErr != nil {
        feedback.Error <- sockErr
        return
    }
    os.Setenv("MIR_SOCKET", sockPath)

    // Finally, start the helper with that purchase URL
    payUiExecFunction(purchaseUrl)
}

func runGlib(stop chan struct{}, finished chan struct{}) {
    context := glib.DefaultMainContext()

    for {
        select {
            case <-stop:
                close(finished)
                return
            default:
                context.Iteration(false)
        }
    }
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

// getAppPid attempts to obtain the PID of the given appId. Note that the only
// scope that is supported is the click scope.
func getAppPid(packageName string) (uint32, error) {
    // This has the same FIXME as pay-service: Before other scopes can use pay,
    // we'll need to figure out how to detect that they're actually scopes.
    // For now we'll only support the click-scope.
    if packageName == "click-scope" {
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
        appId := payUiGetAppIdFunction(packageName, "", "")
        pid := payUiGetPrimaryPidFunction(appId)
        if pid == 0 {
            return 0, fmt.Errorf(
                `Unable to get PID of "%s": No such application is running`,
                appId)
        }

        return pid, nil
    }
}
