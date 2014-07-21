
#ifndef PACKAGE_H
#define PACKAGE_H 1

#include <memory>

#include <QObject>

#include <libpay/pay-package.h>

class Package : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString pkgname READ pkgname NOTIFY pkgnameChanged)

public:
	QString pkgname() const;
	void setPkgname(const QString &pkgname);

	Q_INVOKABLE QString itemStatus (const QString & item);
	Package (QObject * parent = nullptr);

signals:
	void pkgnameChanged();
	void itemStatusChanged(const QString &item, const QString& status);

private:
	std::shared_ptr<PayPackage> pkg;
	QString _pkgname;
};

#endif /* PACKAGE_H */
