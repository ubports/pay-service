#include "package.h"

Package::Package (QObject * parent):
	QObject(parent)
{
	return;
}

Package::~Package (void)
{
	if (pkg != nullptr) {
		pay_package_delete(pkg);
		pkg = nullptr;
	}
}

QString pkgname (void)
{
	return _pkgname;
}

void setPkgname (const QString * pkgname)
{
	if (pkgname == _pkgname) {
		return;
	}

	if (pkg != nullptr) {
		pay_package_delete(pkg);
		pkg = nullptr;
	}

	_pkgname = pkgname;
	pkg = pay_package_new(_pkgname.toUTF8().data());

	pkgnameChanged();
}

QString enum2str (PayPackageItemStatus val)
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
	}

	return QString("Unknown");
}

QString itemStatus (const QString &item)
{
	QString retval;

	if (pkg == nullptr) {
		return retval;
	}

	return enum2string(pay_package_item_status(pkg, item.toUTF8().data()));
}
