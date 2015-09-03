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
    "time"
)


type FakeTimer struct {
    duration time.Duration
    resetCalled bool
    stopCalled bool
}

func (timer *FakeTimer) Reset(duration time.Duration) bool {
    timer.resetCalled = true
    retval := true

    if timer.duration == 0 {
        retval = false
    }
    timer.duration = duration
    return retval
}

func (timer *FakeTimer) Stop() bool {
    timer.stopCalled = true

    if timer.duration == 0 {
        return false
    }
    timer.duration = 0
    return true
}
