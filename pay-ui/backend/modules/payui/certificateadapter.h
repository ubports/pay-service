/*
 * Copyright 2014 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
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

#ifndef CERTIFICATEADAPTER_H
#define CERTIFICATEADAPTER_H

#include <QObject>
#include <QSslCertificate>
#include <QMetaType>

/*
 * Adapt a QSSlCertificate to the interface that oxide uses for certs.
 */
class CertificateAdapter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString subjectDisplayName READ subjectDisplayName)
public:
    CertificateAdapter(QSslCertificate cert, QObject *parent = 0);
    CertificateAdapter(const CertificateAdapter& other);
    CertificateAdapter();
    ~CertificateAdapter();

signals:

public slots:
    QStringList getSubjectInfo(int subject);

protected:
    QString subjectDisplayName();
    QSslCertificate m_cert;
};

Q_DECLARE_METATYPE(CertificateAdapter);

#endif // CERTIFICATEADAPTER_H
