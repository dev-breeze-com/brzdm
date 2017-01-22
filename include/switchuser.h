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
#ifndef _SWITCHUSER_H_
#define _SWITCHUSER_H_

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <paths.h>
#include <cstdio>
#include <iostream>

#include "config.h"

namespace breeze {

class SwitchUser {
public:
	SwitchUser(struct passwd *p, Config *c, const std::string& dpy, char** env);
	~SwitchUser();
	void Login(const std::string& cmd, const std::string& mcookie);

protected:
	SwitchUser();
	void SetEnvironment();
	void SetUserId();
	void Execute(const std::string& cmd);
	void SetClientAuth(const std::string& mcookie);

protected:
	struct passwd *_pw_info;
	std::string _dpy_name;
	Config* _config;
	char** _env;

};

};

#endif /* _SWITCHUSER_H_ */
