/* Copyright (C) 2015 Canonical Ltd.
 *
 * This file is part of go-trust-store.
 *
 * go-trust-store is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the Free Software Foundation.
 *
 * go-trust-store is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with go-trust-store. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kyle Fazzari <kyle@canonical.com>
 */

package trust

// #include <stdlib.h>
// #include "answer_shim.h"
import "C"

// Answer represents a response from the user: yes or no, granted or denied.
type Answer int

const (
	AnswerDenied Answer = iota
	AnswerGranted
)

type shimMapping struct {
	shim C.Answer
	name string
}

// answerShimMappings maps the answers to their string and shim representations.
var answerShimMappings = map[Answer]shimMapping{
	AnswerDenied: shimMapping{
		shim: C.DENIED,
		name: "DENIED",
	},
	AnswerGranted: shimMapping{
		shim: C.GRANTED,
		name: "GRANTED",
	},
}

// AnswerFromShim converts a shim answer to an Answer.
func AnswerFromShim(shimInt int) Answer {
	shim := (C.Answer)(shimInt)

	for key, value := range answerShimMappings {
		if value.shim == shim {
			return key
		}
	}

	return AnswerDenied
}

// ToShim converts an Answer to a shim answer.
func (answer Answer) ToShim() int {
	return int(answerShimMappings[answer].shim)
}

// String returns the string representation of the answer.
func (answer Answer) String() string {
	return answerShimMappings[answer].name
}
