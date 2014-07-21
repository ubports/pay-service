
#include <QObject>
#include <libpay/pay-package.h>

class Package : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString pkgname READ pkgname NOTIFY pkgnameChanged)

	QString pkgname() const;
	void setPkgname(const QString &pkgname);

	QString itemStatus (const QString & item);
	Package (QObject * parent = nullptr);

signals:
	void pkgnameChanged();
	void itemStatusChanged(const QString &item, const QString& status);

private:
	PayPackage * pkg;
	QString _pkgname;
};
