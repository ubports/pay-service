/*
 * Copyright 2014 Canonical Ltd.
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

#ifndef PAY_INFO_H
#define PAY_INFO_H

#include <QObject>
#include <QString>

namespace UbuntuPurchase {

class PayInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString paymentId READ paymentId NOTIFY paymentIdChanged)
    Q_PROPERTY(QString backendId READ backendId NOTIFY backendIdChanged)
    Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)
    Q_PROPERTY(bool requiresInteracion READ requiresInteracion NOTIFY requiresInteracionChanged)
    Q_PROPERTY(bool preferred READ preferred WRITE setPreferred NOTIFY preferredChanged)

public:
    explicit PayInfo(QObject *parent = 0);

    QString name() { return m_name; }
    QString paymentId() { return m_paymentId; }
    QString backendId() { return m_backendId; }
    QString description() { return m_description; }
    bool requiresInteracion() { return m_requiresInteracion; }
    bool preferred() { return m_preferred; }

    void setPayData(QString name, QString description, QString payment, QString backend, bool steps, bool preferred);
    void setName(const QString& name) { m_name = name; Q_EMIT nameChanged(); }
    void setDescription(const QString& description) { m_description = description; Q_EMIT descriptionChanged(); }
    void setPaymentId(const QString& paymentId) { m_paymentId = paymentId; Q_EMIT paymentIdChanged(); }
    void setbackendId(const QString& backendId) { m_backendId = backendId; Q_EMIT backendIdChanged(); }
    void setRequiresInteracion(bool requiresInteracion) { m_requiresInteracion = requiresInteracion; Q_EMIT requiresInteracionChanged(); }
    void setPreferred(bool preferred) { m_preferred = preferred; Q_EMIT preferredChanged(); }

Q_SIGNALS:
    void nameChanged();
    void paymentIdChanged();
    void backendIdChanged();
    void descriptionChanged();
    void requiresInteracionChanged();
    void preferredChanged();

private:
    QString m_name;
    QString m_paymentId;
    QString m_backendId;
    QString m_description;
    bool m_requiresInteracion;
    bool m_preferred;
};
}

#endif // PAY_INFO_H
