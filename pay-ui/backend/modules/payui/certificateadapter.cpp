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

#include <QStringList>
#include "certificateadapter.h"

CertificateAdapter::CertificateAdapter()
{
}

CertificateAdapter::CertificateAdapter(const CertificateAdapter &other)
{
    m_cert = other.m_cert;
}

CertificateAdapter::~CertificateAdapter()
{
}

CertificateAdapter::CertificateAdapter(QSslCertificate cert, QObject *parent) :
    QObject(parent), m_cert(cert)
{
}

QString CertificateAdapter::subjectDisplayName() {
    return m_cert.subjectInfo(QSslCertificate::CommonName).join(", ");
}

QStringList CertificateAdapter::getSubjectInfo(int subject) {
    return m_cert.subjectInfo((QSslCertificate::SubjectInfo)subject);
}
