/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authors:
 *   Ted Gould <ted.gould@canonical.com>
 */

#include "token-grabber-u1.h"
#include "qtbridge.h"

#include <ssoservice.h>
#include <token.h>

class TokenGrabberU1Qt: public QObject
{
    Q_OBJECT

public:
    explicit TokenGrabberU1Qt (QObject* parent = 0);
    void run (void);
    std::string signUrl(std::string url, std::string type);

private Q_SLOTS:
    void handleCredentialsFound(const UbuntuOne::Token& token);
    void handleCredentialsNotFound();
    void handleCredentialsStored();

private:
    UbuntuOne::Token token;
    UbuntuOne::SSOService service;
};

TokenGrabberU1Qt::TokenGrabberU1Qt (QObject* parent) :
    QObject(parent)
{
    std::cout << "Token grabber built" << std::endl;
}

void TokenGrabberU1Qt::run (void)
{
    std::cout << "Token grabber running" << std::endl;

    QObject::connect(&service,
                     &UbuntuOne::SSOService::credentialsFound,
                     this,
                     &TokenGrabberU1Qt::handleCredentialsFound);
    QObject::connect(&service,
                     &UbuntuOne::SSOService::credentialsNotFound,
                     this,
                     &TokenGrabberU1Qt::handleCredentialsNotFound);
    QObject::connect(&service,
                     &UbuntuOne::SSOService::credentialsStored,
                     this,
                     &TokenGrabberU1Qt::handleCredentialsStored);

    service.getCredentials();
}

void TokenGrabberU1Qt::handleCredentialsFound(const UbuntuOne::Token& in_token)
{
    token = in_token;
    std::cout << "Got a Token" << std::endl;
}

void TokenGrabberU1Qt::handleCredentialsNotFound()
{
    std::cout << "No Token :-(" << std::endl;
}

void TokenGrabberU1Qt::handleCredentialsStored()
{
    std::cout << "New Credentials Stored" << std::endl;
    service.getCredentials();
}

std::string TokenGrabberU1Qt::signUrl (std::string url, std::string type)
{
    std::string retval;

    auto qretval = token.signUrl(url.c_str(), type.c_str());
    retval = std::string(qretval.toUtf8());

    return retval;
}

TokenGrabberU1::TokenGrabberU1 (void) :
    grabber(nullptr)
{
    //qt = std::make_shared<TokenGrabberU1Qt>();
    qtfuture = qt::core::world::enter_with_task_and_expect_result<std::shared_ptr<TokenGrabberU1Qt>>([]()
    {
        auto qtgrabber = std::make_shared<TokenGrabberU1Qt>();
        qtgrabber->run();
        return qtgrabber;
    });
}

TokenGrabberU1::~TokenGrabberU1 (void)
{
}

std::string TokenGrabberU1::signUrl (std::string url, std::string type)
{
    if (qtfuture.valid())
    {
        grabber = qtfuture.get();
    }

    if (grabber != nullptr)
    {
        return grabber->signUrl(url, type);
    }

    std::string retval;
    return retval;
}

#include "token-grabber-u1.moc"
