#ifndef PURCHASE_H
#define PURCHASE_H

#include <QDateTime>
#include <QObject>
#include <QString>

#include "network.h"
#include "certificateadapter.h"

namespace UbuntuPurchase {

class Purchase : public QObject
{
    Q_OBJECT

public:
    explicit Purchase(QObject *parent = 0);
    ~Purchase();

    Q_INVOKABLE void getItemDetails();
    Q_INVOKABLE void getPaymentTypes(const QString &currency);
    Q_INVOKABLE void buyItemWithPreferredPayment(QString email, QString password, QString otp, QString currency, bool recentLogin);
    Q_INVOKABLE void buyItem(QString email, QString password, QString otp, QString currency, QString paymentId, QString backendId, bool recentLogin);
    Q_INVOKABLE void checkCredentials();
    Q_INVOKABLE QString getAddPaymentUrl(QString currency);
    Q_INVOKABLE void checkWallet(QString email, QString password, QString otp);

    Q_INVOKABLE void quitSuccess();
    Q_INVOKABLE void quitCancel();
    Q_INVOKABLE QDateTime getTokenUpdated();
    Q_INVOKABLE void checkItemPurchased();

Q_SIGNALS:
    void itemDetailsObtained(QString title, QString publisher, QString currency, QString formatted_price, QString icon);
    void paymentTypesObtained(QVariantList payments);
    void buyItemSucceeded();
    void buyItemFailed();
    void buyInterationRequired(QString url);
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

private:
    Network m_network;
    QString m_appid;
    QString m_itemid;
};

}

#endif // PURCHASE_H
