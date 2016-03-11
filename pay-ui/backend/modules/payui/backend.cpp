#include <QtQml>
#include <QtQml/QQmlContext>
#include "backend.h"
#include "purchase.h"
#include "certificateadapter.h"
#include "oxideconstants.h"
#include "pay_info.h"

void BackendPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("payui"));

    qmlRegisterType<UbuntuPurchase::Purchase>(uri, 0, 1, "Purchase");
    qmlRegisterType<UbuntuPurchase::PayInfo>(uri, 0, 1, "PayInfo");
    qmlRegisterType<CertificateAdapter>(uri, 0, 1, "CertificateAdapter");
    qmlRegisterType<SecurityStatus>(uri, 0, 1, "SecurityStatus");
    qmlRegisterType<SslCertificate>(uri, 0, 1, "SslCertificate");
}

void BackendPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    QQmlExtensionPlugin::initializeEngine(engine, uri);
}
