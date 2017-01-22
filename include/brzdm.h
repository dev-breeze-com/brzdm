// Brzdm - Breeze::OS Login/Display Manager
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
#ifndef _BRZDM_H_
#define _BRZDM_H_

#include "config.h"
#include "panel.h"
#include "screen.h"
#include "app.h"

namespace breeze {

class Brzdm : public App {
public:
	static void User1Signal(int sig);
	static void CatchSignal(int sig);
	static int xioerror(Display *dpy);

	static char _username[128];
	static int BRZDM_UID;
	static int BRZDM_GID;
	static Brzdm* BRZDM;

public:
	Brzdm(int argc, char **argv);
	~Brzdm();

	//void stopServer(Panel::Action);

	void run();
	void restart();

	void kiosk();
	void login();
	void reboot();
	void shutdown();
	void suspend();
	//void console();
	void terminate();

protected:
	bool authenticateUser();

	bool undoSetuid();
	bool doSetuid(uid_t uid, gid_t gid);
	bool doSetuid(const std::string& user);
	bool setGroups(const std::string& user);

	//char* getHomeDir(const char* user);
	//void setEnvironment(gid_t gid);
	void setLocale(const std::string& optarg);

	bool verify(const std::string& progpath);

	void updateWtmp(const std::string& username,
		const std::string& hostname, bool logined=true);

};

};

#endif /* _BRZDM_H_ */
