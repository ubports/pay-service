
#include <QQuickView>
#include <QtGui/QGuiApplication>
#include <QtQml/qqml.h>

#include "qtquick2applicationviewer.h"
#include "package.h"

int main (int argc, char * argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<Package>("Pay", 1, 0, "Package");

    QtQuick2ApplicationViewer view;
    view.setMainQmlFile(QStringLiteral("pay-test-app.qml"));
    view.showExpanded();

    return app.exec();
}
