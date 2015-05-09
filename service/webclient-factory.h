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

#include <core/signal.h>
#include <map>
#include <memory>
#include <string>


#ifndef WEBCLIENT_FACTORY_HPP__
#define WEBCLIENT_FACTORY_HPP__ 1

namespace Web {

class Response {
public:
	virtual std::string& body () = 0;
	virtual bool is_success () = 0;

	typedef std::shared_ptr<Response> Ptr;
};

class Request {
public:
	virtual bool run (void) = 0;
	virtual void set_header (const std::string& key,
	                         const std::string& value) = 0;

	typedef std::shared_ptr<Request> Ptr;

	core::Signal<std::string> error;
	core::Signal<Response::Ptr> finished;
};

class Factory {
public:
	virtual ~Factory() = default;

	virtual bool running () = 0;
	virtual Request::Ptr create_request (const std::string& url,
	                                     bool sign) = 0;

	/** A testing hook invoked before calling the web;
            can be used for verifying url & headers, and for injecting test ones */
        virtual void setPreWebHook(std::function<void(std::string&, std::map<std::string,std::string>&)> hook) { preWebHook = hook; }

	typedef std::shared_ptr<Factory> Ptr;

protected:
        std::function<void(std::string&, std::map<std::string,std::string>&)> preWebHook;
};

} // ns Web

#endif /* WEBCLIENT_FACTORY_HPP__ */
