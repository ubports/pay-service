/*
 * Copyright © 2015 Canonical Ltd.
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
 */

#include "webclient-factory.h"

#ifndef WEBCLIENT_NULL_HPP__
#define WEBCLIENT_NULL_HPP__ 1

namespace Web {

class NullFactory : public Factory {
public:
	virtual bool running ();
	virtual Request::Ptr create_request (const std::string& url,
                                         const std::string& method,
                                         bool sign,
                                         const std::string& data);

	typedef std::shared_ptr<NullFactory> Ptr;
};

} // ns Web

#endif /* WEBCLIENT_NULL_HPP__ */
