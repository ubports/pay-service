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

#include "webclient-null.h"

namespace Web
{

class NullRequest : public Request
{
public:
    NullRequest (void)
    {
    }

    ~NullRequest (void)
    {
    }

    virtual bool run (void)
    {
        return false;
    }

    virtual void set_header (const std::string& key,
                             const std::string& value)
    {
    }

    virtual void set_parameter (const std::string& key,
                                const std::string& value)
    {
    }
};


bool
NullFactory::running ()
{
    return false;
}

Request::Ptr
NullFactory::create_request (const std::string& url,
                             bool sign)
{
    return std::make_shared<NullRequest>();
}

} // ns Web
