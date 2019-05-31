// Brzdm - Breeze::OS Display Manager
//
// Copyright (C) 1997, 1998 Per Liden
// Copyright (C) 2004-06 Simone Rota <sip@varlock.com>
// Copyright (C) 2004-06 Johannes Winkelmann <jw@tks6.net>
//
// @author Pierre Innocent <dev@breezeos.com>
// Copyright (C) Tsert.Inc, All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//

#ifndef _APP_H_
#define _APP_H_

#include <X11/Xlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <iostream>

#include "config.h"
#include "panel.h"

#if defined( USE_CONSOLEKIT2 )
#include "consolekit.h"
#endif

namespace breeze {

class App {
public:
	static bool _force_restart;
	static int IgnoreXIO(Display *dpy);

public:
	App(int argc, char **argv);
	virtual ~App();

	bool testMode() const;
	bool secureMode() const;
	bool noPasswdHalt() const;
	bool noPasswdReboot() const;
	bool noPasswdShutdown() const;

	void openLog();
	void closeLog();
	void getLock();
	void removeLock();
	void createServerAuth();

	virtual void run() = 0;
	virtual void restart() = 0;

protected:
	void updatePid();
	void printUsage();
	void printQRCode();

	void loadTheme(const std::string&);
	bool authenticateUser(bool focuspass);

protected:
	Panels *_panels;
	Config *_config;
	Window _rootwin;
	Display *_dpy;

#if defined( USE_CONSOLEKIT2 )
	Ck::Session _ck_session;
#endif

	bool _test_mode;
	bool _secure_mode;
	bool _first_login;
	bool _daemon_mode;
	bool _fork_server;

	bool _nopasswd_reboot;
	bool _nopasswd_shutdown;

	std::string _dpy_name;
	std::string _bg_file;
	std::string _background;
	std::string _kiosk_username;

	const int _mcookiesize;
	std::string _mcookie;
};

};

#endif /* _APP_H_ */
