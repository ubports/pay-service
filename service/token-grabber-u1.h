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

#ifndef TOKEN_GRABBER_U1_HPP__
#define TOKEN_GRABBER_U1_HPP__ 1

#include <future>
#include "token-grabber.h"

class TokenGrabberU1Qt;

class TokenGrabberU1 : public TokenGrabber
{
public:
    TokenGrabberU1 (void);
    virtual ~TokenGrabberU1 (void);

    virtual std::string signUrl(std::string url, std::string type);

private:
    std::future<std::shared_ptr<TokenGrabberU1Qt>> qtfuture;
};


#endif /* TOKEN_GRABBER_U1_HPP__ */
