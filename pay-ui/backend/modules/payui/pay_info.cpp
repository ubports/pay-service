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

#include "pay_info.h"
namespace UbuntuPurchase {

PayInfo::PayInfo(QObject *parent) :
    QObject(parent)
{
}

void PayInfo::setPayData(QString name, QString description, QString payment, QString backend, bool requiresInteracion, bool preferred)
{
    m_name = name;
    m_paymentId = payment;
    m_description = description;
    m_backendId = backend;
    m_requiresInteracion = requiresInteracion;
    m_preferred = preferred;
}

}
