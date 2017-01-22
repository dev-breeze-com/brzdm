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
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/randr.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include "numlock.h"
#include "brzdm.h"
#include "utils.h"
#include "imlib.h"
#include "const.h"

#ifdef HAVE_SHADOW
#include <shadow.h>
#endif

namespace breeze {

bool App::_force_restart = (true);

//-----------------------------------------------------------------------------
App::~App()
//-----------------------------------------------------------------------------
{
	delete _panels;
	delete _config;
	closeLog();
}

//-----------------------------------------------------------------------------
App::App(int argc, char** argv) : _mcookiesize(32) /* Must be divisible by 4 */
//-----------------------------------------------------------------------------
{
	std::string cfgfile( "/etc/brzdm.conf" );
	int opt = 0;

	_fork_server = true;
	_first_login = true;
	_secure_mode = false;
	_daemon_mode = false;
	_force_nodaemon = false;
	_nopasswd_reboot = false;
	_nopasswd_shutdown = false;

	_mcookie = std::string( _mcookiesize, 'a' );

	_panels = new Panels();
	_config = new Config();

    ::mkdir( "/var/lib/brzdm/", 0700 );

#ifdef _USE_BRZDM_UID_
    ::chown( "/var/lib/brzdm/", BRZDM_UID, BRZDM_GID );
#endif

	while ((opt = ::getopt(argc, argv, "NSsvhD:c:n:d?")) != EOF) {
		switch (opt) {
			case 'c':	/* Config file */
				cfgfile = optarg;
			break;
			case 'd':	/* Daemon mode */
				_daemon_mode = true;
			break;
			case 'N':	/* No restart mode */
				_force_restart = false;
			break;
			case 'n':	/* No Daemon mode */
				_daemon_mode = false;
				_force_nodaemon = true;
			break;
			case 'S':	/* Server forked */
				_fork_server = false;
			break;
			case 'D':	/* Secure mode */
				_dpy_name = optarg;
			break;
			case 's':	/* Secure mode */
				_secure_mode = true;
			break;
			case 'v':	/* Version */
				std::cout << "brzdm: v1.0.0 (c) 2015 Tsert.Inc\n";
				std::exit(OK_EXIT);
			break;
			case '?':
			case 'h':   /* Help */
				std::cerr << "usage: brzdm [option ...]\n"
				<< "options:\n"
				<< "	-d: daemon mode\n"
				<< "	-D: display name\n"
				<< "	-c: use specified config file\n"
				<< "	-s: secure mode\n"
				<< "	-S: skip exec display server\n"
				<< "	-n: no-daemon mode\n"
				<< "	-N: no-restart mode (rc.4 mode)\n"
				<< "	-v: show version\n";
				std::exit(OK_EXIT);

			default:
				std::cerr << "Illegal !\n";
			break;
		}
	}

	openLog();

	if (getuid() != 0) {
		::syslog( LOGFLAGS, "Only root can run this program" );
		std::exit(ERR_EXIT);
	}

	if (! _config->load( cfgfile )) {
		::syslog(LOGFLAGS, "Could not load config file '%s'!", cfgfile.c_str());
		std::exit(ERR_EXIT);
	}

	_nopasswd_reboot = _config->getBool( "Allow/reboot" );
	_nopasswd_shutdown = _config->getBool( "Allow/shutdown" );

	loadTheme( _config->get( "theme" ));

	if ( !_secure_mode ) {
		_secure_mode = _config->getBool( "secure" );
	}
}

//-----------------------------------------------------------------------------
bool App::secureMode() const { return _secure_mode; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool App::noPasswdReboot() const
//-----------------------------------------------------------------------------
{
	return _nopasswd_reboot;
}

//-----------------------------------------------------------------------------
bool App::noPasswdShutdown() const
//-----------------------------------------------------------------------------
{
	return _nopasswd_shutdown;
}

//-----------------------------------------------------------------------------
bool App::noPasswdHalt() const
//-----------------------------------------------------------------------------
{
	return _nopasswd_reboot || _nopasswd_shutdown;
}

//-----------------------------------------------------------------------------
void App::loadTheme(const std::string& name)
//-----------------------------------------------------------------------------
{
	std::string theme( name );
	Config themecfg;

	for (;;) {

		std::string folder( "/usr/share/brzdm/themes/" );
		std::string path( "/usr/share/brzdm/themes/" );

		folder += theme;
		folder += "/";

		path += theme;
		path += "/brzdm.theme";

		if (themecfg.load( path )) {
			themecfg.set( "themedir", folder );
			themecfg.set( "themepath", path );
			_config->operator+=( themecfg );
			break;
		}

		if (theme == "default") {
			::syslog( LOGFLAGS, "Failed to open default theme file !" );
			std::exit(ERR_EXIT);
		}
		::syslog( LOGFLAGS, "Invalid theme '%s'", theme.c_str() );
		theme = "default";
	}
}

//-----------------------------------------------------------------------------
bool App::authenticateUser(bool focuspass)
//-----------------------------------------------------------------------------
{
	std::string username, password;

	if ( !focuspass ) {
		Screen::EventHandler( Screen::Get_Name );
		Panel::Action action = Screen::getAction();

		if (action == Panel::Exit || action == Panel::Console) {
			::syslog( LOGFLAGS, "Got a special command !" );
			return true; /* <--- This is simply fake! */
		}
	}

	Screen::EventHandler( Screen::Get_Passwd );

	struct passwd *pw = 0L;
	char *encrypted = 0L;
	char *correct = 0L;

	switch ( Screen::getAction() ) {
		case Panel::Kiosk:
			pw = ::getpwnam( _kiosk_username.c_str() );
		break;
		case Panel::Reboot:
		case Panel::Suspend:
		case Panel::Shutdown:
			pw = ::getpwnam( "root" );
		break;
		case Panel::Exit:
		case Panel::Console:
			username = _panels->getName();
			pw = ::getpwnam( username.c_str() );
		break;
		case Panel::Login:
			username = _panels->getName();

			if (username != "root") {
				pw = ::getpwnam( username.c_str() );
			} else if (!_config->getBool( "root-login" )) {
				Screen::message( "No root login allowed !" );
			}
		break;
		default:
		break;
	}

	endpwent();

	if (pw == 0) { return false; }

#ifdef HAVE_SHADOW
	struct spwd *sp = ::getspnam( pw->pw_name );
	endspent();
	if ( sp ) {
		correct = sp->sp_pwdp;
	}
#else
	correct = pw->pw_passwd;
#endif

	if (correct == 0 || correct[0] == '\0')
		return true;

	password = _panels->getPassword();
	encrypted = ::crypt( password.c_str(), correct );

	return std::strcmp( encrypted, correct ) == 0;
}

//-----------------------------------------------------------------------------
void App::createServerAuth()
//-----------------------------------------------------------------------------
{
	// Create mit cookie
	//
	// We rely on the fact that all bits generated by random()
	// are usable, so we are taking full words from its output.
	//
	std::string authfile( _config->get( "Auth/file" ));
	const char *digits = "0123456789abcdef";
	uint16_t word;
	uint8_t hi, lo;
	int i;

	Utils::srandom();

	if ( authfile.empty() ) {
		::syslog( LOGFLAGS, "XAuthority file was not specified !" );
		std::exit(ERR_EXIT);
	}

	for (i = 0; i < _mcookiesize; i+=4) {
		word = Utils::random() & 0xffff;
		lo = word & 0xff; hi = word >> 8;

		_mcookie[i] = digits[lo & 0x0f];
		_mcookie[i+1] = digits[lo >> 4];
		_mcookie[i+2] = digits[hi & 0x0f];
		_mcookie[i+3] = digits[hi >> 4];
	}

	::unlink( authfile.c_str() );
	::putenv( Utils::strcat( "XAUTHORITY=", authfile ));

	//std::cout << "MCOOKIE='" << _mcookie << "'\n";
	_dpy_name = Screen::displayName();

	Utils::add_mcookie(
		_mcookie,
		_dpy_name,
		_config->get( "Auth/command" ),
		authfile
	);
}

//-----------------------------------------------------------------------------
void App::closeLog() { ::closelog(); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void App::openLog()
//-----------------------------------------------------------------------------
{
	int options = LOG_NDELAY|LOG_NOWAIT|LOG_PID;
	int facility = LOG_AUTHPRIV|LOG_DAEMON;
	::openlog( "brzdm", options, facility );
}

//-----------------------------------------------------------------------------
void App::updatePid()
//-----------------------------------------------------------------------------
{
	std::string path( _config->get( "lockfile" ));
	std::ofstream lockfile( path.c_str() );

	if ( !lockfile ) {
		::syslog( LOGFLAGS, "Could not update lock file !" );
		std::exit( ERR_EXIT );
	}
	lockfile << getpid() << "\n";
	lockfile.close();
}

//-----------------------------------------------------------------------------
void App::getLock()
//-----------------------------------------------------------------------------
{
	std::string path( _config->get( "lockfile" ));
	std::ifstream lockfile( path.c_str() );

	if (!lockfile) {
		std::ofstream lockfile( path.c_str() );

		if (!lockfile) {
			::syslog( LOGFLAGS, "Could not create lock file !" );
			std::exit(ERR_EXIT);
		}
		lockfile << getpid() << "\n";
		lockfile.close();
	}
	else {
		// lockfile present, read pid from it
		int pid = 0;

		lockfile >> pid;
		lockfile.close();

		if (pid > 0) {
			/* see if process with this pid exists */
			int ret = kill(pid, 0);

			if (ret == 0 || (ret == -1 && errno == EPERM) ) {
				::syslog( LOGFLAGS, "Another instance is already running !" );
				std::exit(0);
			}

			::syslog( LOGFLAGS, "Stale lockfile found, removing it !" );
			std::ofstream lockfile( path.c_str() );

			if (!lockfile) {
				::syslog( LOGFLAGS, "Could not create new lock file !" );
				std::exit(ERR_EXIT);
			}
			lockfile << getpid() << "\n";
			lockfile.close();
		}
	}
}

//-----------------------------------------------------------------------------
void App::removeLock()
//-----------------------------------------------------------------------------
{
	std::string lockfile( _config->get( "lockfile" ));

	if ( !lockfile.empty() ) {
		::unlink( lockfile.c_str() );
	}
}

};

