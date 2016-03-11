/*
 * Copyright 2014-2016 Canonical Ltd.
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

#include "network.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QByteArray>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QFile>
#include <QProcessEnvironment>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusConnection>

#include "certificateadapter.h"

#define PAY_PURCHASES_PATH "/purchases"
#define PAY_PAYMENTMETHODS_PATH "/paymentmethods"
#define PAY_PAYMENTMETHODS_ADD_PATH PAY_PAYMENTMETHODS_PATH + "/add"
#define SUGGESTED_CURRENCY_HEADER_NAME "X-Suggested-Currency"

#define DEVICE_ID_HEADER "X-Device-Id"

#define PARTNER_ID_HEADER "X-Partner-ID"
#define PARTNER_ID_FILE "/custom/partner-id"

#define PREFERED_PAYMENT_TYPE "0"
#define PAYMENT_TYPES "1"
#define BUY_ITEM "2"
#define ITEM_INFO "3"
#define UPDATE_CREDENTIALS "4"
#define CHECK_PASSWORD "5"
#define CHECK_PURCHASED "6"

#define BUY_COMPLETE "Complete"
#define BUY_IN_PROGRESS "InProgress"

namespace UbuntuPurchase {

Network::Network(QObject *parent) :
    QObject(parent),
    m_nam(this),
    m_service(this),
    m_preferred(nullptr)
{
    connect(&m_nam, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(onReply(QNetworkReply*)));
    // SSO SERVICE
    connect(&m_service, &CredentialsService::credentialsFound,
                     this, &Network::handleCredentialsFound);
    connect(&m_service, &CredentialsService::credentialsNotFound,
                     this, &Network::credentialsNotFound);
    connect(&m_service, &SSOService::credentialsStored,
                     this, &Network::handleCredentialsStored);
    connect(&m_service, &CredentialsService::loginError,
            this, &Network::loginError);
    connect(&m_service, &SSOService::twoFactorAuthRequired,
            this, &Network::twoFactorAuthRequired);
}

void Network::getCredentials()
{
    qDebug() << "getting credentials";
    m_service.getCredentials();
}

void Network::setCredentials(Token token)
{
    m_service.setCredentials(token);
}

QMap<QString, QString> buildCurrencyMap()
{
    /* NOTE: The list of currencies we need to handle mapping symbols of.
     * Please keep this list in A-Z order.
     */
    QMap<QString, QString> currencies_map {
        { "CNY", "RMB"},
        { "EUR", "€"},
        { "GBP", "₤"},
        { "HKD", "HK$"},
        { "TWD", "TW$"},
        { "USD", "US$"},
    };
    return currencies_map;
}

const QString DEFAULT_CURRENCY {"USD"};

QString Network::getSymbolForCurrency(const QString &currency_code)
{
    static QMap<QString, QString> currency_map = buildCurrencyMap();

    if (currency_map.contains(currency_code)) {
        return currency_map[currency_code];
    } else{
        return currency_code;
    }
}

bool Network::isSupportedCurrency(const QString& currency_code)
{
    static QMap<QString, QString> currency_map = buildCurrencyMap();

    return currency_map.contains(currency_code);
}

QString Network::sanitizeUrl(const QUrl& url)
{
    static QRegExp regexp("\\b\\/\\/+");
    return url.toString().replace(regexp, "/");
}

QString Network::encodeQuerySlashes(const QString& query)
{
    static QRegExp regexp("\\b\\/");
    QString temp(query);
    return temp.replace(regexp, "%2F");
}

void Network::onReply(QNetworkReply *reply)
{
    QVariant statusAttr = reply->attribute(
                            QNetworkRequest::HttpStatusCodeAttribute);
    if (!statusAttr.isValid()) {
        QString message("Invalid reply status");
        qWarning() << message;
        Q_EMIT error(message);
        return;
    }

    RequestObject* state = qobject_cast<RequestObject*>(reply->request().originatingObject());
    if (state->operation.isEmpty()) {
        QString message("Reply received for non valid state.");
        qWarning() << message;
        Q_EMIT error(message);
        return;
    }

    int httpStatus = statusAttr.toInt();
    qDebug() << "Reply status:" << httpStatus;
    if (httpStatus == 200 || httpStatus == 201) {
        QByteArray payload = reply->readAll();
        qDebug() << payload;
        QJsonDocument document = QJsonDocument::fromJson(payload);

        if (state->operation.contains(PAYMENT_TYPES) && document.isArray()) {
            qDebug() << "Reply state: PAYMENT_TYPES";
            QVariantList listPays;
            QJsonArray array = document.array();
            for (int i = 0; i < array.size(); i++) {
                QJsonObject object = array.at(i).toObject();
                QString description = object.value("description").toString();
                QString backend = object.value("id").toString();
                bool preferredBackend = object.value("preferred").toBool();
                QJsonArray choices = object.value("choices").toArray();
                for (int j = 0; j < choices.size(); j++) {
                    QJsonObject choice = choices.at(j).toObject();
                    QString name = choice.value("description").toString();
                    QString paymentId = QString::number(choice.value("id").toInt());
                    bool requiresInteracion = choice.value("requires_interaction").toBool();
                    bool preferred = choice.value("preferred").toBool() && preferredBackend;
                    PayInfo* pay = new PayInfo();
                    pay->setPayData(name, description, paymentId, backend, requiresInteracion, preferred);
                    listPays.append(qVariantFromValue((QObject*)pay));
                    if (preferred) {
                        m_preferred = pay;
                    }
                }
            }
            qDebug() << "Emit signal paymentTypesObtained";
            Q_EMIT paymentTypesObtained(listPays);
            if (m_preferred == nullptr) {
                Q_EMIT noPreferredPaymentMethod();
            }
            qDebug() << "Emit signal certificateFound";
            CertificateAdapter* cert = new CertificateAdapter(reply->sslConfiguration().peerCertificate());
            Q_EMIT certificateFound(cert);
        } else if (state->operation.contains(BUY_ITEM) && document.isObject()) {
            qDebug() << "Reply state: BUY_ITEM";
            QJsonObject object = document.object();
            QString state = object.value("state").toString();

            if (state == BUY_COMPLETE) {
                qDebug() << "BUY STATE: complete";
                Q_EMIT buyItemSucceeded();
            } else if (state == BUY_IN_PROGRESS) {
                QUrl url(getPayApiUrl(object.value("redirect_to").toString()));
                qDebug() << "BUY STATE: in progress";
                qDebug() << "BUY Redirect URL:" << url.toString();
                QString sign = m_token.signUrl(url.toString(), "GET", true);
                url.setQuery(encodeQuerySlashes(sign));
                Q_EMIT buyInteractionRequired(url.toString());
            } else {
                qDebug() << "BUY STATE: failed";
                Q_EMIT buyItemFailed();
            }
        } else if (state->operation.contains(ITEM_INFO) && document.isObject()) {
            qDebug() << "Reply state: ITEM_INFO";
            QJsonObject object = document.object();
            QString icon;
            QString publisher;
            if (object.contains("publisher")) {
                publisher = object.value("publisher").toString();
            }
            if (object.contains("icon_url")) {
                icon = object.value("icon_url").toString();
            } else if (object.contains("icon")) {
                icon = object.value("icon").toString();
            }

            QString title = object.value("title").toString();

            QJsonObject prices = object.value("prices").toObject();
            QString suggested_currency = DEFAULT_CURRENCY;
            QString currency = DEFAULT_CURRENCY;
            if (reply->hasRawHeader(SUGGESTED_CURRENCY_HEADER_NAME)) {
                suggested_currency = reply->rawHeader(SUGGESTED_CURRENCY_HEADER_NAME);
            }
            const char* env_value = std::getenv(CURRENCY_ENVVAR);
            if (env_value != NULL) {
                suggested_currency = env_value;
            }

            if (isSupportedCurrency(suggested_currency) && prices.contains(suggested_currency)) {
                currency = suggested_currency;
            }
            double price = 0.00;
            if (prices[currency].isDouble()) {
                    price = prices[currency].toDouble();
            } else if (prices[currency].isString()) {
                price = prices[currency].toString().toDouble();
            }
            QLocale locale;
            QString formatted_price = locale.toCurrencyString(price, getSymbolForCurrency(currency));
            qDebug() << "Sending signal: itemDetailsObtained: " << title << " " << formatted_price;
            Q_EMIT itemDetailsObtained(title, publisher, currency, formatted_price, icon);
        } else if (state->operation.contains(CHECK_PURCHASED)) {
            QJsonObject object = document.object();
            auto state = object.value("state").toString();
            if (state == "Complete" || state == "purchased "||
                state == "approved") {
                Q_EMIT buyItemSucceeded();
            } else {
                Q_EMIT itemNotPurchased();
            }
        } else {
            QString message("Reply received for non valid state.");
            qWarning() << message;
            Q_EMIT error(message);
        }

    } else if (httpStatus == 401 || httpStatus == 403) {
        qWarning() << "Credentials no longer valid. Invalidating.";
        m_service.invalidateCredentials();
        Q_EMIT authenticationError();
    } else if (httpStatus == 404 && state->operation.contains(CHECK_PURCHASED)) {
        Q_EMIT itemNotPurchased();
    } else {
        QString message(QString::number(httpStatus));
        message += ": ";
        message += reply->readAll().data();
        qWarning() << message;
        Q_EMIT error(message);
    }
    reply->deleteLater();
}

void Network::requestPaymentTypes(const QString& currency)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QUrl url(getPayApiUrl(QString(PAY_API_ROOT) + PAY_PAYMENTMETHODS_PATH + "/"));
    QUrlQuery query;
    query.addQueryItem("currency", currency);
    url.setQuery(query);
    qDebug() << "Request Payment Types:" << url.toString();
    signRequestUrl(request, url.toString());
    request.setRawHeader("Accept", "application/json");
    request.setUrl(url);
    RequestObject* reqObject = new RequestObject(QString(PAYMENT_TYPES));
    request.setOriginatingObject(reqObject);
    m_nam.get(request);
}

void Network::checkPassword(const QString& email, const QString& password,
                            const QString& otp, bool purchasing)
{
    m_startPurchase = purchasing;
    m_service.login(email, password, otp);
}

void Network::buyItemWithPreferredPaymentType(const QString& email, const QString& password, const QString& otp,
                                              const QString& appid, const QString& itemid, const QString& currency,
                                              bool recentLogin)
{
    m_selectedPaymentId = m_preferred->paymentId();
    m_selectedBackendId = m_preferred->backendId();
    m_selectedAppId = appid;
    m_selectedItemId = itemid;
    m_currency = currency;
    if (recentLogin) {
        purchaseProcess();
    } else {
        checkPassword(email, password, otp, true);
    }
}

void Network::buyItem(const QString& email, const QString& password,
                      const QString& otp,
                      const QString& appid, const QString& itemid, const QString& currency,
                      const QString& paymentId, const QString& backendId, bool recentLogin)
{
    m_selectedPaymentId = paymentId;
    m_selectedBackendId = backendId;
    m_selectedAppId = appid;
    m_selectedItemId = itemid;
    m_currency = currency;
    if (recentLogin) {
        purchaseProcess();
    } else {
        checkPassword(email, password, otp, true);
    }
}

void Network::purchaseProcess()
{
    QUrl url(getPayApiUrl(QString(PAY_API_ROOT) + PAY_PURCHASES_PATH + "/"));
    qDebug() << "Request Purchase:" << url;
    qDebug() << "Payment" << m_selectedAppId << m_selectedBackendId << m_selectedPaymentId;
    QJsonObject serializer;

    serializer.insert("name", m_selectedAppId);
    if (!m_selectedItemId.isEmpty()) {
        serializer.insert("item_sku", m_selectedItemId);
    }
    serializer.insert("backend_id", m_selectedBackendId);
    serializer.insert("method_id", m_selectedPaymentId);
    serializer.insert("currency", m_currency);
    QJsonDocument doc(serializer);

    QByteArray content = doc.toJson();

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(DEVICE_ID_HEADER, getDeviceId().toUtf8().data());

    // Get the partner ID and add it to the request.
    QByteArray partner_id = getPartnerId();
    if (!partner_id.isEmpty()) {
        request.setRawHeader(PARTNER_ID_HEADER, partner_id);
    }
    request.setUrl(url);
    signRequestUrl(request, url.toString(), QString("POST"));
    RequestObject* reqObject = new RequestObject(QString(BUY_ITEM));
    request.setOriginatingObject(reqObject);
    m_nam.post(request, content);
}

QByteArray Network::getPartnerId()
{
    QByteArray id;

    if (QFile::exists(PARTNER_ID_FILE)) {
        QFile pid_file(PARTNER_ID_FILE);
        if (pid_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // Always use lowercase, and trim whitespace.
            id = pid_file.readLine().toLower().trimmed();
            qDebug() << "Found partner ID:" << id;
        } else {
            qWarning() << "Failed to open partner ID file.";
        }
    } else {
        qDebug() << "No partner ID file found.";
    }

    return id;
}

QString Network::getDeviceId()
{
    QDBusInterface iface("com.ubuntu.WhoopsiePreferences",
                         "/com/ubuntu/WhoopsiePreferences",
                         "com.ubuntu.WhoopsiePreferences",
                         QDBusConnection::systemBus(), 0);
    QDBusReply<QString> reply = iface.call("GetIdentifier");
    return reply.value();
}

void Network::getItemInfo(const QString& packagename, const QString& sku)
{
    QUrl url;

    if (sku.isEmpty()) {
        url = getSearchApiUrl(QString(SEARCH_API_ROOT) + "/package/" + packagename);
        qDebug() << "Request Item Info:" << url;
        QUrlQuery query;
        query.addQueryItem("fields", "title,description,price,icon_url");
        url.setQuery(query);
    } else {
        url = getPayApiUrl(QString(IAP_API_ROOT) + "/packages/" + packagename + "/items/by-sku/" + sku);
    }
    qDebug() << "Request Item Info:" << url;

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setUrl(url);
    signRequestUrl(request, url.toString(), QStringLiteral("GET"));
    RequestObject* reqObject = new RequestObject(QString(ITEM_INFO));
    request.setOriginatingObject(reqObject);
    m_nam.get(request);
}

QString Network::getEnvironmentValue(const QString& key,
                                     const QString& defaultValue)
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    QString result = environment.value(key, defaultValue);
    return result;
}

QString Network::getPayApiUrl(const QString& path)
{
    QUrl url(getEnvironmentValue(PAY_BASE_URL_ENVVAR, PAY_BASE_URL).append(path));
    return sanitizeUrl(url);
}

QString Network::getSearchApiUrl(const QString& path)
{
    QUrl url(getEnvironmentValue(SEARCH_BASE_URL_ENVVAR, SEARCH_BASE_URL).append(path));
    return sanitizeUrl(url);
}

void Network::signRequestUrl(QNetworkRequest& request, QString url, QString method)
{
    QString sign = m_token.signUrl(url, method);
    request.setRawHeader("Authorization", sign.toUtf8());
}

QString Network::getAddPaymentUrl(const QString& currency)
{
    QUrl payUrl(getPayApiUrl(QString(PAY_API_ROOT) + PAY_PAYMENTMETHODS_ADD_PATH + "/"));
    QUrlQuery query;
    query.addQueryItem("currency", currency);
    payUrl.setQuery(query);
    qDebug() << "Get Add Payment URL:" << payUrl;
    QString sign = m_token.signUrl(payUrl.toString(),
                                   QStringLiteral("GET"), true);
    payUrl.setQuery(encodeQuerySlashes(sign));
    return payUrl.toString();
}

QDateTime Network::getTokenUpdated()
{
    return m_token.updated();
}

void Network::checkItemPurchased(const QString& appid, const QString& sku)
{
    QUrl url;
    if (sku.isEmpty()) {
        url = getPayApiUrl(QString(PAY_API_ROOT) + PAY_PURCHASES_PATH + "/" + appid + "/");
    } else {
        url = getPayApiUrl(QString(IAP_API_ROOT) + "/packages/" + appid + "/items/by-sku/" + sku);
    }

    qDebug() << "Checking for previous purchase:" << url;
    QNetworkRequest request;
    request.setUrl(url);
    signRequestUrl(request, url.toString(), QStringLiteral("GET"));
    RequestObject* reqObject = new RequestObject(QString(CHECK_PURCHASED));
    request.setOriginatingObject(reqObject);
    m_nam.get(request);
}

void Network::handleCredentialsFound(Token token)
{
    m_token = token;
    Q_EMIT credentialsFound();
}

void  Network::handleCredentialsStored()
{
    // Get credentials again to update token object.
    getCredentials();
    if (m_startPurchase) {
        purchaseProcess();
    } else {
        Q_EMIT passwordValid();
    }
}

}
