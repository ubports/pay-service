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

package fakes

import "launchpad.net/go-trust-store/trust"

// Agent is a fake that satisfies the trust.Agent interface.
type Agent struct {
	AuthenticateRequestWithParametersCalled bool
	DestroyCalled                           bool

	AuthenticationRequestParameters trust.RequestParameters

	GrantAuthentication bool
}

// AuthenticateRequestWithParameters will grant authentication depending on the
// value of agent.GrantAuthentication. It also saves the parameters in
// agent.AuthenticationRequestParameters for verification purposes.
func (agent *Agent) AuthenticateRequestWithParameters(parameters *trust.RequestParameters) trust.Answer {
	agent.AuthenticateRequestWithParametersCalled = true
	agent.AuthenticationRequestParameters = *parameters

	if agent.GrantAuthentication {
		return trust.AnswerGranted
	}

	return trust.AnswerDenied
}

// Destroy doesn't do anything in this case.
func (agent *Agent) Destroy() {
	agent.DestroyCalled = true
}
