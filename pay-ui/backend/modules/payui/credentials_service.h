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

#ifndef CREDENTIALS_SERVICE_H
#define CREDENTIALS_SERVICE_H

#include <QObject>
#include <ssoservice.h>
#include <token.h>

using namespace UbuntuOne;

namespace UbuntuPurchase {

class CredentialsService : public SSOService
{
    Q_OBJECT

public:
    explicit CredentialsService(QObject *parent = 0);

    void getCredentials();
    void setCredentials(Token token);
    void login(const QString email, const QString password,
               const QString otp = QString());

Q_SIGNALS:
    void loginError(const QString& message);

private Q_SLOTS:
    void handleRequestFailed(const ErrorResponse& error);

private:
    Token m_token;
};
}

#endif // CREDENTIALS_SERVICE_H
