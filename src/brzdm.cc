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
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <locale.h>
#include <signal.h>
#include <ctype.h>
#include <pwd.h>
#include <syslog.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <utmp.h>

#include <nettle/md5.h>
#include <nettle/sha.h>
#include <nettle/base16.h>

#include <dirent.h>
#include <getopt.h>

#include "const.h"
#include "numlock.h"
#include "brzdm.h"

#ifdef USE_CONSOLEKIT2
#include "consolekit.h"
#endif

#ifdef HAVE_SHADOW
#include <shadow.h>
#endif

namespace breeze {

char Brzdm::_username[128]={0};

int Brzdm::BRZDM_UID = (0);
int Brzdm::BRZDM_GID = (0);

Brzdm* Brzdm::BRZDM = 0L;

//-------------------------------- Functions ----------------------------------

//-----------------------------------------------------------------------------
int Brzdm::xioerror(Display *dpy)
//-----------------------------------------------------------------------------
{
	if ( Brzdm::_force_restart )
		BRZDM->restart();
	return 0;
}

//-----------------------------------------------------------------------------
void Brzdm::User1Signal(int sig) { ::signal( sig, User1Signal ); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void Brzdm::CatchSignal(int sig)
//-----------------------------------------------------------------------------
{
	::syslog( LOGFLAGS, "Unexpected signal %d", sig );

	if (Screen::isServerStarted())
		Screen::stopServer( Panel::Login );

	BRZDM->removeLock();
	BRZDM->closeLog();

	std::exit( ERR_EXIT );
}

//-----------------------------------------------------------------------------
bool Brzdm::undoSetuid()
//-----------------------------------------------------------------------------
{
#ifdef _POSIX_SAVED_IDS
	//return setuid( 0 ) == 0 && 0 == setgid( 0 );
	return seteuid( 0 ) == 0 && setegid( 0 ) == 0;
#else
	return false;
#endif
}

//-----------------------------------------------------------------------------
bool Brzdm::doSetuid(uid_t uid, gid_t gid)
//-----------------------------------------------------------------------------
{
	bool success = false;

	errno = 0;

#ifdef _POSIX_SAVED_IDS
	if (seteuid( 0 ) == 0)
		success = setegid( gid ) == 0 && seteuid( uid ) == 0;
#else
	success = setreuid( real_uid, uid ) == 0;
	//(void) setregid( BRZDM_UID, BRZDM_GID );
#endif

fprintf( stderr, "brzdm: doSetuid EUID=%d EGID=%d '%s'\n",
	geteuid(), getegid(), strerror( errno ));
fflush( stderr );

	return success;
}

//-----------------------------------------------------------------------------
bool Brzdm::doSetuid(const std::string& user)
//-----------------------------------------------------------------------------
{
	struct passwd *pwbuf = 0L;
	bool success = false;

	(void) strcpy( _username, user.c_str() );

	errno = 0;

	if ((pwbuf = getpwnam( _username )))
		success = doSetuid( pwbuf->pw_uid, pwbuf->pw_gid );

fprintf( stderr, "brzdm: setUID EUID=%d EGID=%d '%s'\n",
	geteuid(), getegid(), strerror( errno ));
fflush( stderr );

	return success;
}

//-----------------------------------------------------------------------------
bool Brzdm::setGroups(const std::string& username)
//-----------------------------------------------------------------------------
{
	gid_t *groups = new gid_t[17];
	int ngroups = 17;
	gid_t gid = 17;
	int i = 0;

	errno = 0;
	doSetuid( 0, 0 );

	if (getgrouplist( username.c_str(), gid, groups, &ngroups ) > 0) {
		setgroups( ngroups, groups );

		for (i = 0; i < ngroups; i++)
			fprintf( stderr, "brzdm: GROUP[%d]=%d\n", i, groups[i] );

		fprintf( stderr, "brzdm: errno '%s'\n", strerror( errno ));
		fflush( stderr );
	}

	delete [] groups;
	return doSetuid( username );
}

//-----------------------------------------------------------------------------
void Brzdm::setLocale(const std::string& locale)
//-----------------------------------------------------------------------------
{
	if ( !locale.empty() ) {
		if (! setlocale( LC_ALL, locale.c_str() )) {
			setlocale( LC_ALL, "en_US.UTF-8" );
		}
	}
}

//-----------------------------------------------------------------------------
bool Brzdm::verify(const std::string& progpath)
//-----------------------------------------------------------------------------
{
	std::string hashed( _config->get( "Server/hashed" ));
	return hashed == Utils::hash( progpath, Utils::SHA1_algo );
}

//-----------------------------------------------------------------------------
void Brzdm::updateWtmp(const std::string& username, const std::string& hostname, bool logined)
//-----------------------------------------------------------------------------
{
	struct utmp rec;
	struct timeval now;
	std::time_t nowthen;

	::gettimeofday( &now, 0L );

	::bzero( &rec, sizeof( struct utmp ));

	rec.ut_type = LOGIN_PROCESS;
	rec.ut_pid = ::getpid();

	int len = std::strlen( "/dev/" );
	char *tty = ::ttyname(STDIN_FILENO);

	if ( tty ) {
		std::strcpy( rec.ut_line, tty+len );
		len = std::strlen( "/dev/tty" );
		std::strcpy( rec.ut_id, tty+len );
	} else {
		std::strcpy( rec.ut_line, "tty1" );
		std::strcpy( rec.ut_id, "1" );
	}

	rec.ut_session = ::getsid( getpid() );
	rec.ut_tv.tv_sec = now.tv_sec;
	rec.ut_tv.tv_usec = now.tv_usec;
	//rec.ut_addr = 0;

	std::time( &nowthen );
	rec.ut_time = nowthen;

	memset( rec.ut_user, 0, UT_NAMESIZE );
	std::strcpy( rec.ut_user, username.c_str() );

	memset( rec.ut_host, 0, UT_HOSTSIZE );
	std::strcpy( rec.ut_host, hostname.c_str() );

#if defined( USE_BRZDM_UID )
	doSetuid( 0, 0 );
#endif

	//updwtmp( "/var/log/wtmp", &rec );

	if ( logined ) {
		::login( &rec );
	} else {
		::logout( rec.ut_line );
	}

#if defined( USE_BRZDM_UID )
	doSetuid( BRZDM_UID, BRZDM_GID );
#endif
}

//-----------------------------------------------------------------------------
Brzdm::~Brzdm() {}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
Brzdm::Brzdm(int argc, char **argv) : App( argc, argv )
//-----------------------------------------------------------------------------
{
	Config envvars;

	setenv( "TERM", "xterm", true );
	setenv( "BASH", "/bin/bash", true );
	setenv( "SHELL", "/bin/sh", true );
	setenv( "DISPLAY", DISPLAY, true );
	setenv( "PS1", "\\u@\\h:\\w\\$", true );
	setenv( "LIBXCB_ALLOW_SLOPPY_LOCK", "1", true );
	setenv( "PATH", BINPATH, true );
	setenv( "DISPLAY_MANAGER", "brzdm", true );

#if defined( USE_BRZDM_UID )
	if (doSetuid( "brzdm" )) {
		BRZDM_UID = getuid();
		BRZDM_GID = getgid();
	}
#endif

	if (_config->get( "Env", envvars, true )) {
		for (const auto& tuple: envvars) {
			std::string key( tuple.first );
			std::string val( tuple.second ); 
			setenv( key.c_str(), val.c_str(), true );
		}
	}

	setLocale( _config->get( "locale" ));

#if defined( USE_KEYUTILS )
	if ( _secure_mode ) {
	}
#endif
}

//-----------------------------------------------------------------------------
void Brzdm::restart()
//-----------------------------------------------------------------------------
{
	Screen::stopServer( Panel::Login );

	App::removeLock();

	// Collects all dead childrens
	while (waitpid(-1, NULL, WNOHANG) > 0);

	Brzdm::run();
}

//-----------------------------------------------------------------------------
void Brzdm::run()
//-----------------------------------------------------------------------------
{
	std::string wallpaper( _config->get( "wallpaper" ));
	std::string numlock( _config->get( "numlock" ));
	std::string username;
	Panel::Action action;

	//App::getLock();

	::setenv( "DISPLAY", Screen::displayName(), true );

	::signal( SIGQUIT, Brzdm::CatchSignal );
	::signal( SIGTERM, Brzdm::CatchSignal );
	::signal( SIGKILL, Brzdm::CatchSignal );
	::signal( SIGINT, Brzdm::CatchSignal );
	::signal( SIGHUP, Brzdm::CatchSignal );
	::signal( SIGPIPE, Brzdm::CatchSignal );
	::signal( SIGUSR1, Brzdm::User1Signal );

	if ( _background.empty() ) {
		_bg_file = wallpaper;
	}

	Screen::init( _config, _panels, _dpy_name );

	if ( testMode() ) {
		Screen::startServer( false, false );

	} else {

		_daemon_mode = _daemon_mode || _config->getBool( "daemon" );

		if ( _daemon_mode ) {
			if (::daemon( 0, 1 ) < 0) {
				::syslog( LOGFLAGS, "%s", strerror( errno ));
				std::exit(ERR_EXIT);
			}

			::syslog( LOGFLAGS, "Now in daemon mode !\n" );
			App::updatePid();
		}

		Screen::startServer( _daemon_mode, _fork_server );

		Screen::killAllClients( false );
		Screen::killAllClients( true );
	}

	_dpy = Screen::display();
	_dpy_name = Screen::displayName();

	Imlib::init( _dpy, _config );

	if (! _panels->load( _config, _bg_file )) {
		if ( testMode() ) {
			std::fprintf( stdout, "Could not load panels !\n" );
		}

		::syslog( LOGFLAGS, "Could not load panels !\n" );
		terminate();
	}

	bool panelclosed = true;
	bool firsttime = true;

	if (numlock == "on") {
		NumLock::setOn( Screen::display() );
	} else if (numlock == "off") {
		NumLock::setOff( Screen::display() );
	}

	while ( true ) {

		if ( panelclosed ) {

			Screen::open();

			if ( firsttime ) {
				char hostname[128]={0};
				::gethostname( hostname, 127 );
				std::string mesg( _config->get( "welcome" ));
				mesg = Utils::strrepl( mesg, "%host", hostname );
				Screen::message( mesg, 3 );
			}
		}

		if ( !authenticateUser() ) {
			_panels->zap();
			panelclosed = false;
			Screen::errmesg( "Wrong username or password !", 3 );
			Screen::bell( 100 );
			continue;
		}

		firsttime = false;
		action = Screen::getAction();
		panelclosed = true;

		Screen::close( action );

		switch ( action ) {
			case Panel::Kiosk:
				Brzdm::kiosk();
				break;
			case Panel::Login:
				Brzdm::login();
				break;
#if 0
			case Panel::Console:
				Brzdm::console();
				break;
#endif
			case Panel::Reboot:
				Brzdm::reboot();
				break;
			case Panel::Shutdown:
				Brzdm::shutdown();
				break;
			case Panel::Suspend:
				Brzdm::suspend();
				break;
			case Panel::Exit:
				Brzdm::terminate();
				break;
			default:
				break;
		}
	}
}

//-----------------------------------------------------------------------------
bool Brzdm::authenticateUser()
//-----------------------------------------------------------------------------
{
	std::string username( "root" );
	struct passwd *pw = 0L;
	char *encrypted = 0L;
	char *correct = 0L;
	Panel *panel = 0L;

	Screen::EventHandler( Screen::Get_Name );
	Panel::Action action = Screen::getAction();

	if (action == Panel::Exit || action == Panel::Console)
		return true;

	if ( !Brzdm::noPasswdHalt() || !Screen::noPasswdHalt())
		Screen::EventHandler( Screen::Get_Passwd );

	switch (Screen::getAction()) {
		case Panel::Kiosk:
			username = _config->get( "kiosk-username" );
			pw = ::getpwnam( username.c_str() );
		break;
		case Panel::Suspend:
			pw = ::getpwnam("root");
		break;
		case Panel::Shutdown:
			pw = ::getpwnam("root");
			if (noPasswdShutdown() && Screen::noPasswdHalt())
				return true;
		break;
		case Panel::Reboot:
			pw = ::getpwnam("root");
			if (noPasswdReboot() && Screen::noPasswdHalt())
				return true;
		break;
		case Panel::Exit:
		case Panel::Console:
			username = _panels->getName();
			pw = ::getpwnam( username.c_str() );
		break;
		case Panel::Login:
			username = _panels->getName();

			if (username == "root") {
				if (_config->getBool( "Allow/sysadmin-login" )) {
					pw = ::getpwnam( "root" );
				} else {
					Screen::message( "No sysadmin login allowed !", 3 );
				}
			} else if (username == "secadmin") {
				if (_config->getBool( "Allow/secadmin-login" )) {
					pw = ::getpwnam( "secadmin" );
				} else {
					Screen::message( "No secadmin login allowed !", 3 );
				}
			} else {
				pw = ::getpwnam( username.c_str() );
			}
		break;
		default:
			std::cerr << "Invalid action ...\n";
			std::cerr.flush();
		break;
	}

	::endpwent();

	if( !pw ) {
		::syslog( LOGFLAGS, "Invalid username '%s'", username.c_str() );
		return false;
	}

	if ((panel = _panels->get( "PasswordPanel" )))
		panel->clear();

	if ((panel = _panels->get( "UsernamePanel" )))
		panel->clear();

#ifdef HAVE_SHADOW
	struct spwd *sp = ::getspnam( pw->pw_name );
	if ( sp ) { correct = sp->sp_pwdp; }
	::endspent();
#else
	correct = pw->pw_passwd;
#endif

	if (correct == 0 || correct[0] == '\0')
		return false;

	username = _panels->getPassword();
	encrypted = ::crypt( username.c_str(), correct );

	//std::cout << "Password '" << correct << "' '" << encrypted << "'\n";
	//std::cout.flush();

	return std::strcmp( encrypted, correct ) == 0;
}

//-----------------------------------------------------------------------------
void Brzdm::send_session_id(::pid_t pid, const std::string& cookie)
//-----------------------------------------------------------------------------
{
#ifdef USE_BRZMQ
	char buffer[1024]={ '\0' };

	::sprintf(
		buffer,
		"(map ((app brzdm) (pid %d) (hostname %s) (cookie %s) (key %s)))",
		getpid(), hostname, cookie.c_str(), cookie.c_str()
	);

	int mq = ::nn_socket( AF_SP, NN_REQ );
	if (mq < 0) {
		::syslog( LOGFLAGS, "BrzMQ -- %s [%s]", ::nn_strerror(nn_errno()), hostname );

	} else if (::nn_connect( mq, "tcp://127.0.0.1:8097" ) < 0) {
		::syslog( LOGFLAGS, "BrzMQ -- %s [%s]", ::nn_strerror(nn_errno()), hostname );

	} else {

		for (int i=0; i<5; i++) {

			if (::nn_send( mq, buffer, ::strlen(buffer), NN_DONTWAIT ) > 0)
				break;

			if (errno != EAGAIN) {
				::syslog( LOGFLAGS, "BrzMQ -- could not send from %s", hostname );
				break;
			}
		}
		::nn_close( mq );
	}
#endif
}

//-----------------------------------------------------------------------------
void Brzdm::kiosk()
//-----------------------------------------------------------------------------
{
}

//-----------------------------------------------------------------------------
void Brzdm::login()
//-----------------------------------------------------------------------------
{
	std::string binpath( _config->get( "paths" ));
	std::string username( _panels->getName() );
	std::string cookie( Utils::get_uuid() );

	struct passwd *pw = ::getpwnam( username.c_str() );
	char* term = ::getenv( "TERM");
	char hostname[128]={0};

	::endpwent();

	if ( !pw ) {
		::syslog( LOGFLAGS, "Invalid username '%s'", username.c_str() );
		return;
	}

	if (pw->pw_shell[0] == '\0') {
		setusershell();
		std::strcpy( pw->pw_shell, getusershell() );
		endusershell();
	}

	std::string maildir( "/var/mail/" );
	std::string xauthority( pw->pw_dir );
	std::string xsession( _panels->getSession() );

	maildir.append( pw->pw_name );
	xauthority.append( "/.Xauthority" );

#ifdef USE_CONSOLEKIT2
	std::cout << "Launching a ConsoleKit session ...\n";
	std::cout.flush();

	try {
		_ck_session.open( Ck::Session::X11, _dpy_name, pw->pw_uid );
	}
	catch(Ck::Exception &e) {
		::syslog( LOGFLAGS, "ConsoleKit2 %s", e.errstr.c_str() );
		Screen::message( "Failed to launch a ConsoleKit session !", 3 );
		_panels->zap();
		return;
	}
#endif

	_panels->zap();

	::gethostname( hostname, 127 );

	::pid_t pid = fork();

	if (pid == 0) {

		char** child_env;
		int cnt = 0;

#ifdef USE_BRZMQ
		send_session_id( pid, cookie );
#endif

#ifdef USE_CONSOLEKIT2
		child_env = static_cast<char**>(malloc(sizeof(char*) * 12));
#else
		child_env = static_cast<char**>(malloc(sizeof(char*) * 11));
#endif

		if ( term ) {
			child_env[cnt++] = Utils::strcat( "TERM=", term );
		}

		child_env[cnt++] = Utils::strcat( "HOME=", pw->pw_dir );
		child_env[cnt++] = Utils::strcat( "PWD=", pw->pw_dir );
		child_env[cnt++] = Utils::strcat( "SHELL=", pw->pw_shell );
		child_env[cnt++] = Utils::strcat( "USER=", pw->pw_name );
		child_env[cnt++] = Utils::strcat( "LOGNAME=", pw->pw_name );
		child_env[cnt++] = Utils::strcat( "PATH=", binpath );
		child_env[cnt++] = Utils::strcat( "DISPLAY=", _dpy_name );
		child_env[cnt++] = Utils::strcat( "MAIL=", maildir );
		child_env[cnt++] = Utils::strcat( "XAUTHORITY=", xauthority );
		child_env[cnt++] = Utils::strcat( "XSESSION=", xsession.c_str() );
		child_env[cnt++] = Utils::strcat( "BRZ_SESSION_COOKIE=", cookie.c_str() );

		Utils::strzap( cookie );

#ifdef USE_CONSOLEKIT2
		child_env[cnt++] = Utils::strcat( "XDG_SESSION_COOKIE=", _ck_session.get_xdg_session_cookie());
#endif

		child_env[cnt] = 0L;

		if ( testMode() ) {
			for (int i=0; i<cnt; i++) {
				std::cout << "env[" << i << "]='" << child_env[i] << "'\n";
			}
		}

		SwitchUser Su( pw, _config, _dpy_name, child_env );

		std::string startcmd( _config->get( "Session/startcmd" ));
		std::string theme( _config->get( "Session/theme" ));
		std::string cmd( _config->get( "Session/command" ));

		cmd = Utils::strrepl( cmd, "%theme", theme );
		cmd = Utils::strrepl( cmd, "%session", xsession );

		if ( testMode() ) {
			std::cout << "Session command '" << cmd << "'\n";
			std::cout << "Start command '" << startcmd << "'\n";
			std::cout.flush();
		}

		if ( !startcmd.empty() ) {
			startcmd = Utils::strrepl( startcmd, USER_VAR, pw->pw_name );
			std::system( startcmd.c_str() );
		}

		updateWtmp( pw->pw_name, hostname, true );

		::syslog( LOGFLAGS, "%s", cmd.c_str() );

		Su.Login( cmd, _mcookie );

		_exit(OK_EXIT);
	}

	// Wait until user is logging out (login process terminates)
	::pid_t wpid = (-1);

	int status = (0);

	while (wpid != pid) {

		wpid = ::wait( &status );

		if (wpid == Screen::getServerPID()) {
			std::cerr << "Server died, simulate IO error !\n";
			std::cerr.flush();
			Brzdm::xioerror( _dpy );
		}
	}

	if (WIFEXITED(status) && WEXITSTATUS(status)) {
		Screen::message( "Failed to execute login command", 3 );
		::sleep(3);
		::sync();

	} else {
		std::string stopcmd( _config->get( "Session/stopcmd" ));

		if ( !stopcmd.empty() ) {
			stopcmd = Utils::strrepl(stopcmd, USER_VAR, pw->pw_name);
			std::system( stopcmd.c_str() );
		}

		updateWtmp( pw->pw_name, hostname, false );
	}

#ifdef USE_CONSOLEKIT2
	try {
		_ck_session.close();
	} catch(Ck::Exception &e) {
		::syslog( LOGFLAGS, "%s", e.errstr.c_str() );
	};
#endif

	Screen::killAllClients( false );
	Screen::killAllClients( true );

	// Send HUP signal to clientgroup
	::killpg( pid, SIGHUP );

	// Send TERM or KILL signal to clientgroup
	if(::killpg( pid, SIGTERM ))
		::killpg( pid, SIGKILL );

	BRZDM->restart();
}

//-----------------------------------------------------------------------------
void Brzdm::reboot()
//-----------------------------------------------------------------------------
{
	std::string cmd( _config->get( "Reboot/command" ));
	Screen::message( _config->get( "Reboot/message" ));

	::sleep(3);

	Screen::stopServer( Panel::Reboot );

	App::removeLock();
	App::closeLog();

	std::system( cmd.c_str() );
	std::exit(OK_EXIT);
}

//-----------------------------------------------------------------------------
void Brzdm::shutdown()
//-----------------------------------------------------------------------------
{
	std::string cmd( _config->get( "Shutdown/command" ));
	Screen::message( _config->get( "Shutdown/message" ));

	::sleep(3);

	Screen::stopServer( Panel::Shutdown );

	App::removeLock();
	App::closeLog();

	std::system( cmd.c_str() );
	std::exit(OK_EXIT);
}

//-----------------------------------------------------------------------------
void Brzdm::suspend()
//-----------------------------------------------------------------------------
{
	std::string cmd( _config->get( "Suspend/command" ));
	Screen::message( _config->get( "Suspend/message" ));

	::sleep(3);

	std::system( cmd.c_str() );
}

#if 0
//-----------------------------------------------------------------------------
void Brzdm::console()
//-----------------------------------------------------------------------------
{
	std::string cmd( _config->get( "Console/command" ));

	if ( !cmd.empty() ) {
		int posx = 40;
		int posy = 40;
		int fontx = 9;
		int fonty = 15;
		int scrw = Screen::width() - (posx * 2)/fontx;
		int scrh = Screen::height() - (posy * 2)/fonty;

		/* Execute console */
		char *tmp = new char[ cmd.length() + 60 ];
		std::sprintf( tmp, cmd.c_str(), scrw, scrh, posx, posy, fontx, fonty );

		std::system( tmp );
		delete [] tmp;
	}
}
#endif

//-----------------------------------------------------------------------------
void Brzdm::terminate()
//-----------------------------------------------------------------------------
{
	Screen::close( Panel::Exit );
	Screen::stopServer( Panel::Exit );
	removeLock();
	closeLog();
	delete _config;
	std::exit( OK_EXIT );
}

};

using namespace breeze;

//-----------------------------------------------------------------------------
int main(int argc, char **argv)
//-----------------------------------------------------------------------------
{
	Brzdm::BRZDM = new Brzdm( argc, argv );
	Brzdm::BRZDM->run();
	std::exit(0);
}

//==================================== EOF ====================================
