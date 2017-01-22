// Brzdm - Breeze::OS Login/Display Manager
//
// Copyright (C) 2007 Martin Parm
//
// @author Pierre Innocent <dev@breezeos.com>
// Copyright (C) Tsert.Inc, All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _CK_H_
#define _CK_H_

#include <string>
#include <ck-connector.h>
#include <dbus/dbus.h>

namespace Ck {

class Exception {
public:
	std::string func;
	std::string errstr;
	Exception(const std::string &func, const std::string &errstr);
};

class Session {
public:
	enum Type { X11, WAYLAND };

protected:
	Session::Type _type;
	CkConnector *ckc;
	DBusError error;

	const char* get_x11_device(const std::string &display);
	const char* get_wayland_device(const std::string &display);

	dbus_bool_t start(Session::Type, const std::string &dpy, uid_t uid);

public:
	Session();
	~Session();

	const char* get_xdg_session_cookie();

	void open(Session::Type, const std::string &display, uid_t uid);
	void close();
};

};

std::ostream &operator<<(std::ostream &os, const Ck::Exception &e);

#endif /* _CK_H_ */
