// SlimBrz - Simple Login Manager
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
#include "utils.h"
#include "switchuser.h"
#include "screen.h"

namespace breeze {

//-----------------------------------------------------------------------------
SwitchUser::SwitchUser(struct passwd *pw, Config *c, const std::string& display, char** env)
//-----------------------------------------------------------------------------
{
	_env = env;
	_config = c;
	_pw_info = pw;
	_dpy_name = display;

	if (Screen::debugMode()) {
		std::cerr << "Switching user " << display << " " << env << " " << pw << " " << c << "\n";
	}
}

//-----------------------------------------------------------------------------
SwitchUser::~SwitchUser()
//-----------------------------------------------------------------------------
{
	/* Never called */
}

//-----------------------------------------------------------------------------
void SwitchUser::Login(const std::string& cmd, const std::string& mcookie)
//-----------------------------------------------------------------------------
{
	SetUserId();
	SetClientAuth( mcookie );
	Execute(cmd);
}

//-----------------------------------------------------------------------------
void SwitchUser::SetUserId()
//-----------------------------------------------------------------------------
{
	if ((_pw_info == 0) ||
		(initgroups( _pw_info->pw_name,  _pw_info->pw_gid) != 0) ||
		(setgid( _pw_info->pw_gid) != 0) ||
		(setuid( _pw_info->pw_uid) != 0))
	{
		if (Screen::debugMode()) {
			std::cerr << "Could not switch user id\n";
			std::cerr.flush();
		}
		::syslog( LOGFLAGS, "Could not switch user id" );
		std::exit(ERR_EXIT);
	}
}

//-----------------------------------------------------------------------------
void SwitchUser::Execute(const std::string& cmd)
//-----------------------------------------------------------------------------
{
	::chdir( _pw_info->pw_dir );

	if (Screen::debugMode()) {
		::syslog( LOGFLAGS, "Trying to execute login command %s", cmd.c_str() );
	}

	::execle(
		_pw_info->pw_shell,
		_pw_info->pw_shell,
		"-l",
		"-c",
		cmd.c_str(),
		NULL,
		_env
	);

	::syslog( LOGFLAGS, "Could not execute login command" );
}

//-----------------------------------------------------------------------------
void SwitchUser::SetClientAuth(const std::string& mcookie)
//-----------------------------------------------------------------------------
{
	std::string home( _pw_info->pw_dir );
	std::string authcmd( _config->get( "Auth/command" ));
	std::string authfile( home + "/.Xauthority" );
	::unlink( authfile.c_str() );
	(void) Utils::add_mcookie( mcookie, _dpy_name, authcmd, authfile );
}

};

