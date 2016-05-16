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

#ifndef NETWORK_H
#define NETWORK_H

#include <QDateTime>
#include <QObject>
#include <QtNetwork/QNetworkReply>
#include <QStringList>
#include <QVariantList>
#include <QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <token.h>
#include "credentials_service.h"

#include "certificateadapter.h"
#include "pay_info.h"

using namespace UbuntuOne;

namespace UbuntuPurchase {

constexpr static const char* PAY_BASE_URL_ENVVAR{"PAY_BASE_URL"};
constexpr static const char* PAY_BASE_URL{"https://myapps.developer.ubuntu.com"};
constexpr static const char* PAY_API_ROOT{"/api/2.0/click"};
constexpr static const char* IAP_API_ROOT{"/inventory/api/v1"};
constexpr static const char* CURRENCY_ENVVAR {"U1_SEARCH_CURRENCY"};
constexpr static const char* SEARCH_BASE_URL_ENVVAR{"U1_SEARCH_BASE_URL"};
constexpr static const char* SEARCH_BASE_URL{"https://search.apps.ubuntu.com"};
constexpr static const char* SEARCH_API_ROOT{"/api/v1"};
constexpr static const char* FALLBACK_ICON_URL{"image://theme/placeholder-app-icon"};


class RequestObject : public QObject
{
    Q_OBJECT
public:
    explicit RequestObject(QString oper, QObject *parent = 0) :
        QObject(parent)
    {
        operation = oper;
    }

    QString operation;
};

class Network : public QObject
{
    Q_OBJECT
public:
    explicit Network(QObject *parent = 0);

    void requestPaymentTypes(const QString& currency);
    void buyItem(const QString& email, const QString& password,
                 const QString& otp, const QString& appid,
                 const QString& itemid, const QString& currency, const QString& paymentId,
                 const QString& backendId, bool recentLogin);
    void buyItemWithPreferredPaymentType(const QString& email, const QString& password,
                                         const QString& otp,
                                         const QString& appid, const QString& itemid, const QString& currency,
                                         bool recentLogin);
    void getItemInfo(const QString& packagename, const QString& sku);
    void checkPassword(const QString& email, const QString& password,
                       const QString& otp, bool purchasing=false);
    void getCredentials();
    void setCredentials(Token token);
    QString getAddPaymentUrl(const QString& currency);
    QDateTime getTokenUpdated();
    void checkItemPurchased(const QString& appid, const QString& sku);
    static QString getSymbolForCurrency(const QString& currency_code);
    static bool isSupportedCurrency(const QString& currency_code);
    static QString sanitizeUrl(const QUrl& url);
    static QString encodeQuerySlashes(const QString& query);
    virtual QString getEnvironmentValue(const QString& key,
                                        const QString& defaultValue);
    virtual QString getPayApiUrl(const QString& path);
    virtual QString getSearchApiUrl(const QString& path);

Q_SIGNALS:
    void itemDetailsObtained(QString title, QString publisher, QString currency, QString formatted_price, QString icon);
    void paymentTypesObtained(QVariantList payments);
    void buyItemSucceeded();
    void buyItemFailed();
    void buyInteractionRequired(QString url);
    void error(QString message);
    void authenticationError();
    void credentialsNotFound();
    void credentialsFound();
    void passwordValid();
    void noPreferredPaymentMethod();
    void loginError(const QString& message);
    void twoFactorAuthRequired();
    void itemNotPurchased();
    void certificateFound(QObject* cert);

private Q_SLOTS:
    void onReply(QNetworkReply*);
    void handleCredentialsFound(Token token);
    void handleCredentialsStored();
    void purchaseProcess();

private:
    QNetworkAccessManager m_nam;
    QNetworkRequest m_request;
    CredentialsService m_service;
    Token m_token;
    PayInfo* m_preferred;
    QString m_selectedPaymentId;
    QString m_selectedBackendId;
    QString m_selectedAppId;
    QString m_selectedItemId;
    QString m_currency;
    bool m_startPurchase = false;

    void signRequestUrl(QNetworkRequest& request, QString url, QString method="GET");
    QString getDeviceId();
    QByteArray getPartnerId();
};

}

#endif // NETWORK_H
