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

#ifndef TOKEN_GRABBER_NULL_HPP__
#define TOKEN_GRABBER_NULL_HPP__ 1

#include "service/token-grabber.h"

class TokenGrabberNull : public TokenGrabber
{
public: 
	virtual std::string signUrl (std::string url, std::string type)
	{
		std::string retval;
		return retval;
	}
};

#endif /* TOKEN_GRABBER_NULL_HPP__ */
