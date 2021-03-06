// Brzdm - Breeze::OS Login/Display Manager
//
// Copyright (C) 1997, 1998 Per Liden
// Copyright (C) 2004-06 Simone Rota <sip@varlock.com>
// Copyright (C) 2004-06 Johannes Winkelmann <jw@tks6.net>
//
// @author Pierre Innocent <dev@breezeos.com>
// Copyright (C) 2016 Pierre Innocent <dev@breezeos.com>
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
#include "qrcode.h"

#ifdef HAVE_SHADOW
#include <shadow.h>
#endif

//------------------------------ Watermark ------------------------------------

static const char* const watermark="9460061a-b0df-4ca8-b7f4-4126a10b6d62";

//-----------------------------------------------------------------------------

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
void App::printQRCode()
//-----------------------------------------------------------------------------
{
	std::cout << "QR-Code: " << qrcode::url << "\n\n";

	for (int i=0; i<qrcode::utf8length; i++)
		//fprintf( stdout, "%c", qrcode::utf8data[i] );
		std::cout << qrcode::utf8data[i];

	std::cout.flush();
}

//-----------------------------------------------------------------------------
void App::printUsage()
//-----------------------------------------------------------------------------
{
	std::cout << "Usage: brzdm [option ...]\n"
		<< "Options:\n"
		<< "    -#: show QR code\n"
		<< "    -d: daemon mode\n"
		<< "    -D: display name\n"
		<< "    -c: configuration file\n"
		<< "    -s: secure mode\n"
		<< "    -V: debug mode\n"
		<< "    -t: test your theme\n"
		<< "    -T: use specified theme\n"
		<< "    -B: use specified background\n"
		<< "    -S: Do not exec display server\n"
		<< "    -N: Do not force restart (rc.4 mode)\n"
		<< "    -v: show version\n";
	std::cout.flush();
}

//-----------------------------------------------------------------------------
App::App(int argc, char** argv) : _mcookiesize(32) /* Must be divisible by 4 */
//-----------------------------------------------------------------------------
{
	std::string cfgfile( "/etc/brzdm.conf" );
	std::string theme;
	int opt = 0;

	_test_mode = false;
	_debug_mode = false;
	_fork_server = true;
	_first_login = true;
	_secure_mode = false;
	_daemon_mode = false;

	_nopasswd_reboot = false;
	_nopasswd_shutdown = false;

	_mcookie = std::string( _mcookiesize, 'a' );

	if (std::strlen( watermark ) < 1) {
		std::cerr << "brzdm: the watermark is missing !\n";
		std::cerr.flush();
		std::exit( 1 );
	} else {
		std::cerr << "brzdm: watermark is present -- " << watermark << "\n";
	}

	while ((opt = ::getopt(argc, argv, "#NSVhsvtB:D:T:c:d?n?")) != EOF) {
		switch (opt) {
			case 'B':
				_background = optarg;
			break;
			case 'D':
				_dpy_name = optarg;
			break;
			case 't':
				_test_mode = true;
			break;
			case 'T':
				theme = optarg;
			break;
			case 'c':
				cfgfile = optarg;
			break;
			case 'V':
				_debug_mode = true;
			break;
			case 'n':
				_daemon_mode = false;
			break;
			case 'd':
				_daemon_mode = true;
			break;
			case 'N':
				_force_restart = false;
			break;
			case 'S':
				_fork_server = false;
			break;
			case 's':
				_secure_mode = true;
			break;
			case 'v':
				std::cout << "brzdm: v1.1.0 (c) 2016 Pierre Innocent\n";
				std::exit(OK_EXIT);

			case '#':
				printQRCode();
				std::exit(OK_EXIT);

			case '?':
			case 'h':
				printUsage();
				std::exit(OK_EXIT);

			default:
				std::cerr << "Invalid option !\n";
			break;
		}
	}

	_panels = new Panels();
	_config = new Config();

    ::mkdir( "/var/lib/brzdm/", 0700 );

#ifdef _USE_BRZDM_UID_
    ::chown( "/var/lib/brzdm/", BRZDM_UID, BRZDM_GID );
#endif

	Screen::setTestMode( _test_mode );
	Screen::setDebugMode( _debug_mode );

	openLog();

	if (getuid() != 0) {
		std::cerr << "Only root can run this program\n";
		::syslog( LOGFLAGS, "Only root can run this program" );
		std::exit(ERR_EXIT);
	}

	if (! _config->load( cfgfile )) {
		std::cerr << "Could not load config file " << cfgfile << " !\n";
		::syslog(LOGFLAGS, "Could not load config file '%s'!", cfgfile.c_str());
		std::exit(ERR_EXIT);
	}

	_nopasswd_reboot = _config->getBool( "Allow/reboot" );
	_nopasswd_shutdown = _config->getBool( "Allow/shutdown" );

	if ( theme.empty() ) {
		theme = _config->get( "theme" );
	}

	loadTheme( theme );

	if ( !_secure_mode ) {
		_secure_mode = _config->getBool( "secure" );
	}

	_bg_file = _background;
}

//-----------------------------------------------------------------------------
bool App::secureMode() const { return _secure_mode; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool App::testMode() const { return _test_mode; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool App::debugMode() const { return _debug_mode; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool App::noPasswdReboot() const { return _nopasswd_reboot; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool App::noPasswdShutdown() const { return _nopasswd_shutdown; }
//-----------------------------------------------------------------------------

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
		return false;

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
	std::string authcommand( _config->get( "Auth/command" ));
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

	Utils::add_mcookie( _mcookie, _dpy_name, authcommand, authfile );
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

