
#include <QQuickView>
#include <QtGui/QGuiApplication>

#include "package.h"

int main (int argc, char * argv[])
{
	auto app = new QApplication(argc, argv);

	qmlRegisterType<Package>("Pay", 1, 0, "Package");

	auto view = new QQuickView();
	view->setTitle("Pay Test App");
	view->setSource(Resource::getRcURL("pay-test-app.qml"));
	view->show();

	return app->exec();
}
