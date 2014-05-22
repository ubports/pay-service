/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef VERIFICATION_CURL_HPP__
#define VERIFICATION_CURL_HPP__ 1

namespace Verification {

class CurlFactory : public Factory {
public:
	CurlFactory ();
	CurlFactory (const std::string& endpoint);
	~CurlFactory ();

	virtual bool running ();
	virtual Item::Ptr verifyItem (std::string& appid, std::string& itemid);

	void setEndpoint (std::string& endpoint);
	void setDevice (std::string& device);

private:
	std::string endpoint;
	std::string device;
};

} // ns Verification

#endif /* VERIFICATION_CURL_HPP__ */
