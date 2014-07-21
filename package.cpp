#include "package.h"
#include <QDebug>

Package::Package (QObject * parent):
	QObject(parent)
{
	return;
}

QString Package::pkgname (void) const
{
	return _pkgname;
}

void Package::setPkgname (const QString &pkgname)
{
	if (pkgname == _pkgname) {
		return;
	}

	_pkgname = pkgname;
	pkg = std::shared_ptr<PayPackage>(pay_package_new(_pkgname.toUtf8().data()),
	[](PayPackage * pkg) {if (pkg != nullptr) pay_package_delete(pkg);});

	qDebug() << "Pay Package built for:" << _pkgname.toUtf8().data();

	pkgnameChanged();
}

static QString enum2str (PayPackageItemStatus val)
{
	switch (val) {
	case PAY_PACKAGE_ITEM_STATUS_VERIFYING:
		return QString("Verifying");
	case PAY_PACKAGE_ITEM_STATUS_PURCHASED:
		return QString("Purchased");
	case PAY_PACKAGE_ITEM_STATUS_PURCHASING:
		return QString("Purchasing");
	case PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED:
		return QString("Not Purchased");
	case PAY_PACKAGE_ITEM_STATUS_UNKNOWN:
		break;
	}

	return QString("Unknown");
}

QString Package::itemStatus (const QString &item)
{
	QString retval;

	if (pkg == nullptr) {
		return retval;
	}

	return enum2str(pay_package_item_status(pkg.get(), item.toUtf8().data()));
}

bool Package::verifyItem (const QString & item)
{
	if (pkg == nullptr)
		return false;

	qDebug() << "Verifying item" << item << "for package" << _pkgname;

	return pay_package_item_start_verification(pkg.get(), item.toUtf8().data());
}

bool Package::purchaseItem (const QString & item)
{
	if (pkg == nullptr)
		return false;

	qDebug() << "Purchasing item" << item << "for package" << _pkgname;

	return pay_package_item_start_purchase(pkg.get(), item.toUtf8().data());
}

#include "moc_package.cpp"
