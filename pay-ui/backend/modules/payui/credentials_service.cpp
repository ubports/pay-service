/*
 * Copyright 2014 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "credentials_service.h"

#include <errormessages.h>
#include <token.h>
#include <QProcessEnvironment>


namespace UbuntuPurchase {

bool useFakeCredentials()
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    QString getcreds = environment.value("GET_CREDENTIALS", "1");
    return getcreds != "1";
}

CredentialsService::CredentialsService(QObject *parent) :
    SSOService(parent)
{
    connect(this, &SSOService::requestFailed,
            this, &CredentialsService::handleRequestFailed);
}

void CredentialsService::getCredentials()
{
    if (useFakeCredentials()) {
        Token token("tokenkey", "tokensecret", "consumerkey", "consumersecret");
        Q_EMIT credentialsFound(token);
    } else {
        if (!m_token.isValid()) {
            SSOService::getCredentials();
        } else {
            Q_EMIT credentialsFound(m_token);
        }
    }
}

void CredentialsService::setCredentials(Token token)
{
    m_token = token;
}

void CredentialsService::login(const QString email,
                               const QString password,
                               const QString otp)
{
    if (m_token.isValid() || useFakeCredentials()) {
        Q_EMIT SSOService::credentialsStored();
    } else {
        SSOService::login(email, password, otp);
    }
}

void CredentialsService::handleRequestFailed(const ErrorResponse& error)
{
    if (error.httpStatus() == 0 || error.httpReason() == NO_HTTP_REASON) {
        emit loginError("Network error, please try again.");
    } else {
        emit loginError(error.message());
    }
}

}
