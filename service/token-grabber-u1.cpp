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

#include <ssoservice.h>
#include <token.h>
#include "qtbridge.h"

class TokenGrabberQt: public QObject
{
    Q_OBJECT

public:
    explicit TokenGrabberQt (QObject* parent = 0);
    void run (void);
    std::string signUrl(std::string url, std::string type);

private Q_SLOTS:
    void handleCredentialsFound(const UbuntuOne::Token& token);
    void handleCredentialsNotFound();

private:
    UbuntuOne::Token token;
    UbuntuOne::SSOService service;
};

TokenGrabberQt::TokenGrabberQt (QObject* parent) :
    QObject(parent)
{
    std::cout << "Token grabber built" << std::endl;
}

void TokenGrabberQt::run (void)
{
    std::cout << "Token grabber running" << std::endl;
    qt::core::world::enter_with_task([this] ()
    {
        QObject::connect(&service,
                         &UbuntuOne::SSOService::credentialsFound,
                         this,
                         &TokenGrabberQt::handleCredentialsFound);
        QObject::connect(&service,
                         &UbuntuOne::SSOService::credentialsNotFound,
                         this,
                         &TokenGrabberQt::handleCredentialsNotFound);

        service.getCredentials();
    });
}

void TokenGrabberQt::handleCredentialsFound(const UbuntuOne::Token& in_token)
{
    token = in_token;
    std::cout << "Got a Token" << std::endl;
}

void TokenGrabberQt::handleCredentialsNotFound()
{
    std::cout << "No Token :-(" << std::endl;
}

std::string TokenGrabberQt::signUrl (std::string url, std::string type)
{
    std::string retval;

}

TokenGrabberU1 (void) {
	qt = std::make_shared<TokenGrabberQt>();
	qt->run();
}

~TokenGrabberU1 (void) {
}

std::string TokenGrabberU1::signUrl (std::string url, std::string type) { return qt->signUrl(url, type);}

#include "token-grabber-u1.moc"
