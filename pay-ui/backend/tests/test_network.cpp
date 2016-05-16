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

#include <stdlib.h>

#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QDebug>
#include <QVariant>
#include <QUrlQuery>
#include <QString>
#include <QDir>
#include <QTest>
#include <QTimer>
#include <QSignalSpy>
#include <QVariant>
#include <QVariantList>

#include <modules/payui/network.h>
#include <modules/payui/pay_info.h>

using namespace UbuntuPurchase;

class TestNetwork: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testNetworkAuthenticationError();
    void testNetworkGetPaymentTypes();
    void testNetworkGetPaymentTypesFail();
    void testNetworkBuyItem();
    void testNetworkBuyItemInProgress();
    void testNetworkBuyItemFail();
    void testNetworkButItemWithPaymentType();
    void testNetworkButItemWithPaymentTypeFail();
    void testNetworkButItemWithPaymentTypeInProgress();
    void testNetworkGetItemInfo();
    void testNetworkGetItemInfoEurozone();
    void testNetworkGetItemInfoOtherCoins();
    void testNetworkGetItemInfoOverride();
    void testNetworkGetItemInfoOverrideOther();
    void testNetworkGetItemInfoIAPAppIconFallback();
    void testNetworkGetItemInfoFail();
    void testUseExistingCredentials();
    void testCheckAlreadyPurchased();
    void testCheckAlreadyPurchasedFail();
    void testSanitizeUrl();
    void testEncodeQuerySlashes();
    void cleanupTestCase();

private:
    UbuntuPurchase::Network network;
    QProcess* process;
};

void TestNetwork::initTestCase()
{
    setenv("SSO_AUTH_BASE_URL", "http://localhost:8000/", 1);
    qDebug() << "Starting Server";
    network.setCredentials(Token("token_key", "token_secret", "consumer_key", "consumer_secret"));
    process = new QProcess(this);
    QSignalSpy spy(process, SIGNAL(started()));
    process->setWorkingDirectory(QDir::currentPath() + "/backend/tests/");
    qDebug() << process->workingDirectory();
    QString program = "python3";
    QString script = "mock_click_server.py";
    process->start(program, QStringList() << script);
    QTRY_COMPARE(spy.count(), 1);
    // Wait for server to start
    QTimer timer;
    QSignalSpy spy2(&timer, SIGNAL(timeout()));
    timer.setInterval(2000);
    timer.setSingleShot(true);
    timer.start();
    QTRY_COMPARE(spy2.count(), 1);
}

void TestNetwork::testNetworkAuthenticationError()
{
    setenv(PAY_BASE_URL_ENVVAR, "http://localhost:8000/authError/", 1);
    QSignalSpy spy(&network, SIGNAL(authenticationError()));
    network.buyItem("email", "password", "otp", "USD", "appid", "itemid", "paymentid", "backendid", false);
    QTRY_COMPARE(spy.count(), 1);
}

void TestNetwork::testNetworkGetPaymentTypes()
{
    setenv(PAY_BASE_URL_ENVVAR, "http://localhost:8000/", 1);    
    QSignalSpy spy(&network, SIGNAL(paymentTypesObtained(QVariantList)));
    network.requestPaymentTypes("USD");
    QTRY_COMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toList().count() == 3);
    QVariantList listPays = arguments.at(0).toList();
    for (int i = 0; i < listPays.count(); i++) {
        QVariant var = listPays.at(i);
        PayInfo* info = var.value<PayInfo*>();
        if (i == 2) {
            QVERIFY(info->preferred() == true);
        } else {
            QVERIFY(info->preferred() == false);
        }
    }
}

void TestNetwork::testNetworkGetPaymentTypesFail()
{
    setenv(PAY_BASE_URL_ENVVAR, "http://localhost:8000/fail/", 1);
    QSignalSpy spy(&network, SIGNAL(error(QString)));
    network.requestPaymentTypes("USD");
    QTRY_COMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString().startsWith("404:"));
}

void TestNetwork::testNetworkBuyItem()
{
    setenv(PAY_BASE_URL_ENVVAR, "http://localhost:8000/", 1);
    QSignalSpy spy(&network, SIGNAL(buyItemSucceeded()));
    network.buyItem("email", "password", "otp", "USD", "appid", "itemid", "paymentid", "backendid", false);
    QTRY_COMPARE(spy.count(), 1);
}

void TestNetwork::testNetworkBuyItemInProgress()
{
    setenv(PAY_BASE_URL_ENVVAR, "http://localhost:8000/interaction/", 1);
    QSignalSpy spy(&network, SIGNAL(buyInteractionRequired(QString)));
    network.buyItem("email", "password", "otp", "USD", "appid", "itemid", "paymentid", "backendid", false);
    QTRY_COMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QString url(arguments.at(0).toString());
    QString expected("http://localhost:8000/interaction/redirect.url?currency=USD");
    QVERIFY(url.startsWith(expected));
}

void TestNetwork::testNetworkBuyItemFail()
{
    setenv(PAY_BASE_URL_ENVVAR, "http://localhost:8000/fail/", 1);
    QSignalSpy spy(&network, SIGNAL(buyItemFailed()));
    network.buyItem("email", "password", "otp", "USD", "appid", "itemid", "paymentid", "backendid", false);
    QTRY_COMPARE(spy.count(), 1);
}

void TestNetwork::testNetworkButItemWithPaymentType()
{
    setenv(PAY_BASE_URL_ENVVAR, "http://localhost:8000/", 1);
    QSignalSpy spy(&network, SIGNAL(paymentTypesObtained(QVariantList)));
    network.requestPaymentTypes("USD");
    QTRY_COMPARE(spy.count(), 1);
    QSignalSpy spy2(&network, SIGNAL(buyItemSucceeded()));
    network.buyItemWithPreferredPaymentType("email", "password", "otp", "USD", "appid", "itemid", false);
    QTRY_COMPARE(spy2.count(), 1);
}

void TestNetwork::testNetworkButItemWithPaymentTypeFail()
{
    setenv(PAY_BASE_URL_ENVVAR, "http://localhost:8000/", 1);
    QSignalSpy spy(&network, SIGNAL(paymentTypesObtained(QVariantList)));
    network.requestPaymentTypes("USD");
    QTRY_COMPARE(spy.count(), 1);
    setenv(PAY_BASE_URL_ENVVAR, "http://localhost:8000/fail/", 1);
    QSignalSpy spy2(&network, SIGNAL(buyItemFailed()));
    network.buyItemWithPreferredPaymentType("email", "password", "otp", "USD", "appid", "itemid", false);
    QTRY_COMPARE(spy2.count(), 1);
}

void TestNetwork::testNetworkButItemWithPaymentTypeInProgress()
{
    setenv(PAY_BASE_URL_ENVVAR, "http://localhost:8000/", 1);
    QSignalSpy spy(&network, SIGNAL(paymentTypesObtained(QVariantList)));
    network.requestPaymentTypes("USD""USD");
    QTRY_COMPARE(spy.count(), 1);
    setenv(PAY_BASE_URL_ENVVAR, "http://localhost:8000/interaction/", 1);
    QSignalSpy spy2(&network, SIGNAL(buyInteractionRequired(QString)));
    network.buyItemWithPreferredPaymentType("email", "password", "otp", "USD", "appid", "itemid", false);
    QTRY_COMPARE(spy2.count(), 1);
}

void TestNetwork::testNetworkGetItemInfo()
{
     setenv(SEARCH_BASE_URL_ENVVAR, "http://localhost:8000/iteminfo/", 1);
     unsetenv(CURRENCY_ENVVAR);
     QSignalSpy spy(&network, SIGNAL(itemDetailsObtained(QString,QString,QString,QString,QString)));
     network.getItemInfo("packagename", "");
     QTRY_COMPARE(spy.count(), 1);
     QList<QVariant> arguments = spy.takeFirst();
     QCOMPARE(arguments.at(2).toString(), QStringLiteral("USD"));
     QCOMPARE(arguments.at(3).toString(), QStringLiteral("US$1.99"));
}

void TestNetwork::testNetworkGetItemInfoEurozone()
{
     setenv(SEARCH_BASE_URL_ENVVAR, "http://localhost:8000/iteminfo/eurozone/", 1);
     unsetenv(CURRENCY_ENVVAR);
     QSignalSpy spy(&network, SIGNAL(itemDetailsObtained(QString,QString,QString,QString,QString)));
     network.getItemInfo("packagename", "");
     QTRY_COMPARE(spy.count(), 1);
     QList<QVariant> arguments = spy.takeFirst();
     QCOMPARE(arguments.at(2).toString(), QStringLiteral("EUR"));
     QCOMPARE(arguments.at(3).toString(), QStringLiteral("€1.69"));
}

void TestNetwork::testNetworkGetItemInfoOtherCoins()
{
     setenv(SEARCH_BASE_URL_ENVVAR, "http://localhost:8000/iteminfo/dotar/", 1);
     unsetenv(CURRENCY_ENVVAR);
     QSignalSpy spy(&network, SIGNAL(itemDetailsObtained(QString,QString,QString,QString,QString)));
     network.getItemInfo("packagename", "");
     QTRY_COMPARE(spy.count(), 1);
     QList<QVariant> arguments = spy.takeFirst();
     QCOMPARE(arguments.at(2).toString(), QStringLiteral("USD"));
     QCOMPARE(arguments.at(3).toString(), QStringLiteral("US$1.99"));
}

void TestNetwork::testNetworkGetItemInfoOverride()
{
     setenv(SEARCH_BASE_URL_ENVVAR, "http://localhost:8000/iteminfo/", 1);
     setenv(CURRENCY_ENVVAR, "EUR", true);
     QSignalSpy spy(&network, SIGNAL(itemDetailsObtained(QString,QString,QString,QString,QString)));
     network.getItemInfo("packagename", "");
     QTRY_COMPARE(spy.count(), 1);
     QList<QVariant> arguments = spy.takeFirst();
     QCOMPARE(arguments.at(2).toString(), QStringLiteral("EUR"));
     QCOMPARE(arguments.at(3).toString(), QStringLiteral("€1.69"));
     unsetenv(CURRENCY_ENVVAR);
}

void TestNetwork::testNetworkGetItemInfoOverrideOther()
{
     setenv(SEARCH_BASE_URL_ENVVAR, "http://localhost:8000/iteminfo/dotar/", 1);
     setenv(CURRENCY_ENVVAR, "EUR", true);
     QSignalSpy spy(&network, SIGNAL(itemDetailsObtained(QString,QString,QString,QString,QString)));
     network.getItemInfo("packagename", "");
     QTRY_COMPARE(spy.count(), 1);
     QList<QVariant> arguments = spy.takeFirst();
     QCOMPARE(arguments.at(2).toString(), QStringLiteral("EUR"));
     QCOMPARE(arguments.at(3).toString(), QStringLiteral("€1.69"));
     unsetenv(CURRENCY_ENVVAR);
}

void TestNetwork::testNetworkGetItemInfoIAPAppIconFallback()
{
     setenv(PAY_BASE_URL_ENVVAR, "http://localhost:8000/iteminfo/", 1);
     QSignalSpy spy(&network, SIGNAL(itemDetailsObtained(QString,QString,QString,QString,QString)));
     network.getItemInfo("donations-ubuntu.canonical", "donate5");
     QTRY_COMPARE(spy.count(), 1);
     QList<QVariant> arguments = spy.takeFirst();
     QCOMPARE(arguments.at(4).toString(), QString(FALLBACK_ICON_URL));
     unsetenv(CURRENCY_ENVVAR);
}

void TestNetwork::testNetworkGetItemInfoFail()
{
    setenv(SEARCH_BASE_URL_ENVVAR, "http://localhost:8000/fail/iteminfo/", 1);
    unsetenv(CURRENCY_ENVVAR);
    QSignalSpy spy(&network, SIGNAL(error(QString)));
    network.getItemInfo("packagename", "");
    QTRY_COMPARE(spy.count(), 1);
}

void TestNetwork::testUseExistingCredentials()
{
    setenv(PAY_BASE_URL_ENVVAR, "http://localhost:8000/", 1);
    QSignalSpy spy(&network, SIGNAL(buyItemSucceeded()));
    network.buyItem("email", "password", "otp", "USD", "appid", "itemid", "paymentid", "backendid", true);
    QTRY_COMPARE(spy.count(), 1);
}

void TestNetwork::testCheckAlreadyPurchased()
{
    setenv(PAY_BASE_URL_ENVVAR, "http://localhost:8000/", 1);
    QSignalSpy spy(&network, SIGNAL(buyItemSucceeded()));
    network.checkItemPurchased("com.example.fakeapp", "");
    QTRY_COMPARE(spy.count(), 1);
}

void TestNetwork::testCheckAlreadyPurchasedFail()
{
    setenv(PAY_BASE_URL_ENVVAR, "http://localhost:8000/notpurchased/", 1);
    QSignalSpy spy(&network, SIGNAL(itemNotPurchased()));
    network.checkItemPurchased("com.example.fakeapp", "");
    QTRY_COMPARE(spy.count(), 1);
}

void TestNetwork::testSanitizeUrl()
{
    QUrl url("https://example.com//test/this/heavily///really%2f/");
    QUrl expected("https://example.com/test/this/heavily/really%2f/");
    QString result = network.sanitizeUrl(url);
    QTRY_COMPARE(result, expected.toString());
}

void TestNetwork::testEncodeQuerySlashes()
{
    QString query("abcdef/01235%3D");
    QTRY_COMPARE(network.encodeQuerySlashes(query),
                 QStringLiteral("abcdef%2F01235%3D"));
}

void TestNetwork::cleanupTestCase()
{
    process->close();
    process->deleteLater();
}

QTEST_MAIN(TestNetwork)
#include "test_network.moc"
