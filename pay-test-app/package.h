
#ifndef PACKAGE_H
#define PACKAGE_H 1

#include <memory>

#include <QObject>

#include <libpay/pay-package.h>

class Package : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString pkgname READ pkgname WRITE setPkgname NOTIFY pkgnameChanged)

public:
    QString pkgname() const;
    void setPkgname(const QString &pkgname);

    Package (QObject * parent = nullptr);

    Q_INVOKABLE QString itemStatus (const QString & item);
    Q_INVOKABLE bool verifyItem (const QString & item);
    Q_INVOKABLE bool purchaseItem (const QString & item);

signals:
    void pkgnameChanged();
    void itemStatusChanged(const QString &item, const QString& status);

private:
    std::shared_ptr<PayPackage> pkg;
    QString _pkgname;

public:
    void emitItemChanged(const QString& item, const QString& status);
};

#endif /* PACKAGE_H */
