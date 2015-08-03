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

package main

import (
    "log"
    "os"
    "os/signal"
    "syscall"
    "pay-service-2/service"
)



func main() {
    signals := make(chan os.Signal, 1)
    signal.Notify(signals, syscall.SIGINT, syscall.SIGTERM)

    service, err := service.New()
    if err != nil {
        log.Fatalf("Unable to create daemon: %s", err)
    }

    err = service.Run()
    if err != nil {
        log.Fatalf("Unable to run daemon: %s", err)
    }

    shutdown := func() {
        shutdownError := service.Shutdown()
        if shutdownError != nil {
            fmt.Errorf("Unable to shut down: %s", shutdownError)
            os.Exit(1)
        }

        os.Exit(0)
    }
    service.shutdownTimer = time.AfterFunc(shutdownTimeout, shutdown)

    <-signals // Block so the daemon can run
}
