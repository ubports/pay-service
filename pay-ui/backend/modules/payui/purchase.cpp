/*
 * Copyright 2014-2015 Canonical Ltd.
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

#include "purchase.h"
#include <QUrl>
#include <QUrlQuery>
#include <QCoreApplication>
#include <QDateTime>
#include <QtDebug>

#include <logging.h>

namespace UbuntuPurchase {

Purchase::Purchase(QObject *parent) :
    QObject(parent),
    m_network(this)
{
    connect(&m_network, &Network::itemDetailsObtained,
                     this, &Purchase::itemDetailsObtained);
    connect(&m_network, &Network::paymentTypesObtained,
                     this, &Purchase::paymentTypesObtained);
    connect(&m_network, &Network::buyItemSucceeded,
                     this, &Purchase::buyItemSucceeded);
    connect(&m_network, &Network::buyItemFailed,
                     this, &Purchase::buyItemFailed);
    connect(&m_network, &Network::buyInteractionRequired,
                     this, &Purchase::buyInterationRequired);
    connect(&m_network, &Network::error,
                     this, &Purchase::error);
    connect(&m_network, &Network::authenticationError,
                     this, &Purchase::authenticationError);
    connect(&m_network, &Network::credentialsNotFound,
                     this, &Purchase::credentialsNotFound);
    connect(&m_network, &Network::credentialsFound,
                     this, &Purchase::credentialsFound);
    connect(&m_network, &Network::passwordValid,
                     this, &Purchase::passwordValid);
    connect(&m_network, &Network::noPreferredPaymentMethod,
                     this, &Purchase::noPreferredPaymentMethod);
    connect(&m_network, &Network::loginError,
            this, &Purchase::loginError);
    connect(&m_network, &Network::twoFactorAuthRequired,
            this, &Purchase::twoFactorAuthRequired);
    connect(&m_network, &Network::itemNotPurchased,
                     this, &Purchase::itemNotPurchased);
    connect(&m_network, &Network::certificateFound,
                     this, &Purchase::certificateFound);

    QCoreApplication* instance = QCoreApplication::instance();

    // Set up logging
    UbuntuOne::AuthLogger::setupLogging();
    const char* u1_debug = getenv("U1_DEBUG");
    if (u1_debug != NULL && strcmp(u1_debug, "") != 0) {
        UbuntuOne::AuthLogger::setLogLevel(QtDebugMsg);
    }

    // Handle the purchase: URI
    for (int i = 0; i < instance->arguments().size(); i++) {
        QString argument = instance->arguments().at(i);
        if (argument.startsWith("purchase://")) {
            QUrl data(argument);
            m_appid = data.host();
            m_itemid = data.fileName();
            qDebug() << "Purchase requested for" << m_itemid << "by app" << m_appid;
            break;
        }
    }
}

Purchase::~Purchase()
{
    UbuntuOne::AuthLogger::stopLogging();
}

void Purchase::quitCancel()
{
    qDebug() << "Purchase Canceled: exit code 1";
    QCoreApplication::exit(1);
}

void Purchase::quitSuccess()
{
    qDebug() << "Purchase Succeeded: exit code 0";
    QCoreApplication::exit(0);
}

void Purchase::checkCredentials()
{
    m_network.getCredentials();
}

QString Purchase::getAddPaymentUrl(QString currency)
{
    return m_network.getAddPaymentUrl(currency);
}

void Purchase::getItemDetails()
{
    if (m_appid.isEmpty() && m_itemid.isEmpty()) {
        qCritical() << "AppId or ItemId not provided";
        quitCancel();
    }
    qDebug() << "Getting Item Details";
    m_network.getItemInfo(m_appid, m_itemid);
}

void Purchase::getPaymentTypes(const QString& currency)
{
    m_network.requestPaymentTypes(currency);
}

void Purchase::checkWallet(QString email, QString password, QString otp)
{
    m_network.checkPassword(email, password, otp);
}

void Purchase::buyItemWithPreferredPayment(QString email, QString password, QString otp, QString currency, bool recentLogin)
{
    m_network.buyItemWithPreferredPaymentType(email, password, otp, m_appid, m_itemid, currency, recentLogin);
}

void Purchase::buyItem(QString email, QString password, QString otp, QString currency, QString paymentId, QString backendId, bool recentLogin)
{
    m_network.buyItem(email, password, otp, m_appid, m_itemid, currency, paymentId, backendId, recentLogin);
}

QDateTime Purchase::getTokenUpdated()
{
    return m_network.getTokenUpdated();
}

void Purchase::checkItemPurchased()
{
    m_network.checkItemPurchased(m_appid, m_itemid);
}

}
