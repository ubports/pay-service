/*
 * Copyright © 2014-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Ted Gould <ted.gould@canonical.com>
 */

#include "verification-factory.h"
#include "click-purchases-api.h"

#include <string>

#ifndef VERIFICATION_HTTP_HPP__
#define VERIFICATION_HTTP_HPP__ 1

namespace Verification {

class HttpFactory : public Factory {
public:
	HttpFactory (Web::ClickPurchasesApi::Ptr cpa_in);
	virtual bool running () override;
	virtual Item::Ptr verifyItem (const std::string& appid, const std::string& itemid) override;

private:
	Web::ClickPurchasesApi::Ptr cpa;
};

} // ns Verification

#endif /* VERIFICATION_HTTP_HPP__ */
