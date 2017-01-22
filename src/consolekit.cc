// SlimBrz - Simple Login Manager
//
// Copyright (C) 2011 David Hauweele
//
// @author Pierre Innocent <dev@breezeos.com>
// Copyright (C) Tsert.Inc, All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
#include <cstdio>
#include <iostream>

#include <ck-connector.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdarg.h>

#include "consolekit.h"

namespace Ck {

	Exception::Exception(const std::string &func, const std::string &errstr): func(func), errstr(errstr) {}

	Session::Session() { dbus_error_init(&error); }
	Session::~Session() { dbus_error_free(&error); }

	dbus_bool_t Session::start(Session::Type t, const std::string &dpy, uid_t uid) {
		dbus_bool_t local		= true;
		const char *session_type = t == Session::X11 ? "x11" : "wayland";
		const char *display  = dpy.c_str();
		const char *device   = get_x11_device(display);
		const char *remote_host  = "";
		const char *dpy_dev  = "";

		return ck_connector_open_session_with_parameters(ckc, &error,
			"unix-user", &uid,
			"session-type", &session_type,
			"x11-display", &display,
			"x11-display-device", &device,
			"display-device", &dpy_dev,
			"remote-host-name", &remote_host,
			"is-local", &local,
			NULL);
	}

	const char* Session::get_wayland_device(const std::string &display) {
		static char device[32]={0};
		return device;
	}

	const char* Session::get_x11_device(const std::string &display) {

		static char device[32]={0};

		Display *xdisplay = XOpenDisplay(display.c_str());

		if(!xdisplay)
		  throw Exception(__func__, "cannot open display");

		Window root;
		Atom xfree86_vt_atom;
		Atom return_type_atom;
		int return_format;
		unsigned long return_count;
		unsigned long bytes_left;
		unsigned char *return_value;
		long vt;

		xfree86_vt_atom = XInternAtom( xdisplay, "XFree86_VT", true );

		if (xfree86_vt_atom == None)
		  throw Exception(__func__, "cannot get XFree86_VT");

		root = DefaultRootWindow( xdisplay );

		if(XGetWindowProperty( xdisplay, root, xfree86_vt_atom,
			0L, 1L, false, XA_INTEGER,
			&return_type_atom, &return_format,
			&return_count, &bytes_left,
			&return_value) != Success)
		  throw Exception(__func__, "cannot get root window property");

		if(return_type_atom != XA_INTEGER)
		  throw Exception(__func__, "bad atom type");

		if(return_format != 32)
		  throw Exception(__func__, "invalid return format");

		if(return_count != 1)
		  throw Exception(__func__, "invalid count");

		if(bytes_left != 0)
		  throw Exception(__func__, "invalid bytes left");

		vt = *((long *)return_value);

		std::snprintf(device, 32, "/dev/tty%ld", vt);

		if(return_value)
		  XFree(return_value);

		return device;
	}

	void Session::open(Session::Type t, const std::string &dpy, uid_t uid) {

		ckc = ck_connector_new();

		if(!ckc) {
			throw Exception(__func__, "error setting up connection to ConsoleKit");
		}

		if (! Session::start( t, dpy, uid )) {

			if(dbus_error_is_set(&error))
				throw Exception(__func__, error.message);
			else {
				throw Exception(__func__, "cannot open ConsoleKit session: OOM, DBus system bus " " not available or insufficient privileges");
			}
		}
	}

	const char * Session::get_xdg_session_cookie() {
		return ck_connector_get_cookie(ckc);
	}

	void Session::close() {
		if(!ck_connector_close_session(ckc, &error)) {
			if(dbus_error_is_set(&error))
				throw Exception(__func__, error.message);
			else {
				throw Exception(__func__, "cannot close ConsoleKit session: OOM, DBus system bus " " not available or insufficient privileges");
			}
		}
	}

};

std::ostream& operator<<( std::ostream& os, const Ck::Exception& e)
{
  os << e.func << ": " << e.errstr;
  return os;
}
