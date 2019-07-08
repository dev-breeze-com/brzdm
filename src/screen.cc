// Brzdm - Simple Login/Display Manager
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
#include <sstream>
#include <poll.h>
#include <sys/stat.h>

#include "utils.h"
#include "panel.h"
#include "screen.h"
#include "brzdm.h"

namespace breeze {

struct timeval Screen::_now;

std::string	Screen::_dpy_name;
std::string Screen::_mcookie;

Panel::Action Screen::_action = Panel::Noop;

Screen::FieldType Screen::_field = Screen::Get_Name;
Screen::SessionType Screen::_session_type = Screen::X11;

Config* Screen::_config = 0L;
Panels* Screen::_panels = 0L;

X11Info Screen::_x11;
Cursors Screen::_cursors;

int Screen::_screen_w = 1280;
int Screen::_screen_h = 1024;

int Screen::_server_pid = 0;
int Screen::_server_started = false;

bool Screen::_test_mode = false;
bool Screen::_nopasswd_halt = false;

//-----------------------------------------------------------------------------
void Screen::init(Config* config, Panels *panels, const std::string& dpy)
//-----------------------------------------------------------------------------
{
	_config = config;
	_panels = panels;
	_dpy_name = dpy;

	std::memset( &_x11, 0, sizeof(X11Info) );
}

//-----------------------------------------------------------------------------
void Screen::selectInput(bool flag)
//-----------------------------------------------------------------------------
{
	uint inputmask = KeyPressMask|KeyReleaseMask|
		ButtonPressMask|ButtonReleaseMask|ExposureMask;

	::XFlush( _x11.dpy );

	if ( !testMode() ) {
		::XSelectInput( _x11.dpy, _x11.rootwin, flag ? inputmask : 0 );
	}

	::XSync( _x11.dpy, false );
}

//-----------------------------------------------------------------------------
void Screen::destroy(Window win)
//-----------------------------------------------------------------------------
{
	Display *dpy = Screen::display();

	::XUngrabKeyboard( dpy, CurrentTime );
	::XUnmapWindow( dpy, win );
	::XSync( dpy, false );
	::XDestroyWindow( dpy, win );
	::XSync( dpy, false );
}

//-----------------------------------------------------------------------------
void Screen::blank()
//-----------------------------------------------------------------------------
{
	GC gc = ::XCreateGC( _x11.dpy, _x11.rootwin, 0, 0 );

	::XSetForeground( _x11.dpy, gc, BlackPixel( _x11.dpy, _x11.screen ));
	::XFillRectangle(
		_x11.dpy, _x11.rootwin, gc, 0, 0, Screen::width(), Screen::height()
	);

	::XFlush( _x11.dpy );
	::XFreeGC( _x11.dpy, gc);
}

//-----------------------------------------------------------------------------
void Screen::close(Panel::Action action)
//-----------------------------------------------------------------------------
{
	Screen::selectInput( false );

	::signal( SIGALRM, SIG_IGN );

	if (Screen::session( Screen::X11 )) {

		if ( _x11.rootpix ) {
			::XFreePixmap( _x11.dpy, _x11.rootpix );
			::XSync( _x11.dpy, false );
			::XFlush( _x11.dpy );
			_x11.rootpix = 0L;
		}

		if (action == Panel::Kiosk || action == Panel::Login) {
			::XSetWindowBackgroundPixmap( _x11.dpy, _x11.rootwin, None );
			::XClearWindow( _x11.dpy, _x11.rootwin );
			::XSync( _x11.dpy, false );
			::XFlush( _x11.dpy );
		}
	}
}

//-----------------------------------------------------------------------------
void Screen::open()
//-----------------------------------------------------------------------------
{
	setBackground( "Background", _panels->getBackground() );

	_panels->zap();

	if (_config->getBool( "Session/use-default-on-logout" ))
		_panels->resetSession();

	_panels->showClock();
	_panels->showEnterMesg();
}

//-----------------------------------------------------------------------------
void Screen::message(const std::string& text, int secs)
//-----------------------------------------------------------------------------
{
	Panel *panel = _panels->get( "MesgPanel" );
	Screen::mesg( panel, text, secs );
}

//-----------------------------------------------------------------------------
void Screen::warning(const std::string& text, int secs)
//-----------------------------------------------------------------------------
{
	Panel *panel = _panels->get( "WarningPanel" );
	panel = panel ? panel : _panels->get( "MesgPanel" );
	Screen::mesg( panel, text, secs );
}

//-----------------------------------------------------------------------------
void Screen::errmesg(const std::string& text, int secs)
//-----------------------------------------------------------------------------
{
	Panel *panel = _panels->get( "ErrorPanel" );
	panel = panel ? panel : _panels->get( "MesgPanel" );
	Screen::mesg( panel, text, secs );
}

//-----------------------------------------------------------------------------
void Screen::mesg(Panel *panel, const std::string& text, int secs)
//-----------------------------------------------------------------------------
{
	if ( panel ) {
		panel->showText( text );
	}

	if (secs > 0) {
		secs = secs > 12 ? 12 : secs;
		::signal( SIGALRM, Screen::clearmesg );
		::alarm( secs );
	}
}

//-----------------------------------------------------------------------------
void Screen::clearmesg(int sig)
//-----------------------------------------------------------------------------
{
	signal(SIGALRM, SIG_IGN);
	Screen::clear( "MesgPanel" );
	Screen::tick(sig);
}

//-----------------------------------------------------------------------------
bool Screen::testMode() { return _test_mode; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void Screen::setTestMode(bool flag) { _test_mode = flag; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void Screen::setBackground(const std::string& name, Imlib_Image bgimg)
//-----------------------------------------------------------------------------
{
	Panel *panel = _panels->get( name );
	int w = panel ? panel->width() : 0;
	int h = panel ? panel->height() : 0;

	if (Screen::session( Screen::X11 )) {

		Imlib_Image img = bgimg ? bgimg : panel->image();
		uint depth = DefaultDepth( _x11.dpy, _x11.screen );
		std::string mode( panel->drawmode() );
		std::string url( panel->spec() );
		Window wid = panel->window();

		Pixmap pixmap = XCreatePixmap( _x11.dpy, wid, w, h, depth );

		if (url.find( "background." ) != std::string::npos) {
			Imlib::scale( img, pixmap, w, h, 0, 0, 0 );
		} else if (mode == "scale") {
			Imlib::scale( img, pixmap, w, h, 0, 0, 0 );
		} else {
			Imlib::draw( img, pixmap, 0, 0, 0 );
		}

		::XSetWindowBackgroundPixmap( _x11.dpy, wid, pixmap );
		::XClearWindow( _x11.dpy, wid );
		::XSync( _x11.dpy, false );

		if (_x11.rootwin == wid) {

			_x11.rootpix = 0L;
			::XFreePixmap( _x11.dpy, pixmap );

			if ( bgimg ) {
				Imlib::release( bgimg );
			}

			/*
			panel->setImage( 0L );
			if (_x11.rootpix) {
				::XFreePixmap( _x11.dpy, _x11.rootpix );
				_x11.rootpix = pixmap;
				panel->setPixmap( pixmap );
			}
			*/
		} else {
			::XFreePixmap( _x11.dpy, pixmap );
		}
	}
}

//-----------------------------------------------------------------------------
int Screen::width() { return _screen_w; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int Screen::height() { return _screen_h; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int Screen::screenNb() { return DefaultScreen( _x11.dpy ); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool Screen::session(SessionType t) { return _session_type == t; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
Display* Screen::display() { return _x11.dpy; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
Colormap Screen::colormap() { return DefaultColormap( _x11.dpy, _x11.screen ); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
Visual* Screen::visual() { return DefaultVisual( _x11.dpy, _x11.screen ); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
char* Screen::displayName() { return (char*) _dpy_name.c_str(); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int Screen::getServerPID() { return _server_pid; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool Screen::isServerStarted() { return _server_started; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
Panel::Action Screen::getAction() { return _action; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
Window Screen::rootWindow() { return _x11.rootwin; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
Pixmap Screen::rootPixmap() { return _x11.rootpix; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
Screen::FieldType Screen::field() { return _field; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void Screen::bell(int volume)
//-----------------------------------------------------------------------------
{
	if (Screen::session( Screen::X11 )) {
		//::putchar( '\a' );
		::XBell( _x11.dpy, volume );
		::XSync( _x11.dpy, true );
	} else {
	}

	_panels->showUsername( std::string() );
	_panels->showPassword( std::string() );
}

//-----------------------------------------------------------------------------
void Screen::sync()
//-----------------------------------------------------------------------------
{
	if (Screen::session( Screen::X11 )) {
		::XSync( _x11.dpy, true );
	} else {
	}
}

//-----------------------------------------------------------------------------
void Screen::setName(const std::string& name) { _panels->setUsername( name ); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void Screen::setPassword(const std::string& pw) { _panels->setPassword( pw ); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void Screen::clear(Window wid)
//-----------------------------------------------------------------------------
{
	if (Screen::session( Screen::X11 )) {
		::XClearWindow( _x11.dpy, wid == 0 ? _x11.rootwin : wid );
		::XSync( _x11.dpy, false );
	} else {
	}
}

//-----------------------------------------------------------------------------
void Screen::clear(const std::string& name)
//-----------------------------------------------------------------------------
{
	Panel *panel = _panels->get( name );
	if ( panel ) { panel->clear(); }
}

//-----------------------------------------------------------------------------
void Screen::disallow(int sig)
//-----------------------------------------------------------------------------
{
	_nopasswd_halt = false;
}

//-----------------------------------------------------------------------------
void Screen::tick(int sig)
//-----------------------------------------------------------------------------
{
	_panels->showClock();
}

//-----------------------------------------------------------------------------
bool Screen::noPasswdHalt()
//-----------------------------------------------------------------------------
{
	return _nopasswd_halt;
}

//-----------------------------------------------------------------------------
bool Screen::secureLogin()
//-----------------------------------------------------------------------------
{
	Screen::message( "No secure auto-login in this release!", 3 );
	return true;
#if 0
	if ( !Brzdm::BRZDM->secureMode()) {
		Screen::message( "Secure mode not enabled !", 3 );
		return true;
	}
	_action = Panel::SecureLogin;
	setName( KeyUtils::getSecureName() );
	setPassword( KeyUtils::getSecurePassword() );
	return false;
#endif
}

//-----------------------------------------------------------------------------
void Screen::takeSnapshot()
//-----------------------------------------------------------------------------
{
	if (_config->getBool( "Allow/snapshot" )) {

		std::string mesg( _config->get( "Snapshot/message" ));
		std::string cmd( _config->get( "Snapshot/command" ));

		selectInput( false );
		std::system( cmd.c_str() );
		selectInput( true );

		Screen::bell(100);
		Screen::message( mesg, 2 );
	}
}

//-----------------------------------------------------------------------------
void Screen::EventHandler(const Screen::FieldType& curfield)
//-----------------------------------------------------------------------------
{
	struct pollfd x11_pfd = {0};
	bool loop = true;
	XEvent event;

	x11_pfd.fd = ConnectionNumber( _x11.dpy );
	x11_pfd.events = POLLIN;

	_field = curfield;

	_panels->showEnterMesg();

	while ( loop ) {

		if (XPending(_x11.dpy) || poll(&x11_pfd, 1, -1) > 0) {

			while (XPending( _x11.dpy )) {
				XNextEvent( _x11.dpy, &event );

				switch ( event.type ) {
					case Expose:
						//onExpose( event );
std::cerr << "X server Expose event.\n";
std::cerr.flush();
						_panels->showEnterMesg();
					break;
					case ButtonPress:
						::gettimeofday( &_now, 0L );
					break;
					case ButtonRelease:
						if (Utils::elapsedMSecs( _now ) < 200) {
							loop = onButtonRelease( event );
						}
					break;
					case KeyPress:
						loop = onKeyPress( event );
					break;
					default:
					break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
void Screen::onExpose(XEvent& event)
//-----------------------------------------------------------------------------
{
	Panel *panel = _panels->get( event.xmotion.window );
	if ( panel ) {
		panel->show();
	}
}

//-----------------------------------------------------------------------------
bool Screen::onButtonRelease(XEvent& event)
//-----------------------------------------------------------------------------
{
	Panel *panel = _panels->get( event.xmotion.x, event.xmotion.y );

	if (panel && panel->isa( Panel::Button )) {

		std::string name( panel->getName() );

		if (name == "Login")
			return secureLogin();

		if (name == "Next") {
			_panels->nextSession();
			return true;
		}

		if (name == "Prev") {
			_panels->prevSession();
			return true;
		}

		if (name == "Snapshot") {
			Screen::takeSnapshot();
			return true;
		}

		if (name == "Shutdown") {
			_action = Panel::Shutdown;
			setName( SHUTDOWN_STR );

			if ( !noPasswdHalt() ) {
				warning( "The admin's password is required !", 3 );
			}
			return false;
		}

		if (name == "Reboot") {
			_action = Panel::Reboot;
			setName( REBOOT_STR );

			if ( !noPasswdHalt() ) {
				warning( "The admin's password is required !", 3 );
			}
			return false;
		}

#if 0
		if (name == "Suspend") {
			_action = Panel::Suspend;
			setName( SUSPEND_STR );
			return false;
		}

		if (name == "Exit") {
			_action = Panel::Exit;
			setName( EXIT_STR );
			return false;
		}

		if (name == "Console") {
			_action = Panel::Console;
			setName( CONSOLE_STR );
			return false;
		}
#endif
	}
	return true;
}

//-----------------------------------------------------------------------------
bool Screen::onKeyPress(XEvent& event)
//-----------------------------------------------------------------------------
{
	//Panel *panel = _panels->get( event.xmotion.window );
	XComposeStatus compstatus;
	KeySym keysym;
	char ascii;

	XLookupString( &event.xkey, &ascii, 1, &keysym, &compstatus );

	_nopasswd_halt = false;

	switch ( keysym ) {
		case XK_F1:
			_panels->nextSession();
			return true;

		case XK_F2:
			return secureLogin();

		case XK_F5:
			_action = Panel::Shutdown;
			setName( SHUTDOWN_STR );
			return false;

		case XK_F6:
			_action = Panel::Reboot;
			setName( REBOOT_STR );
			return false;

		case XK_F7:
			_action = Panel::Suspend;
			setName( SUSPEND_STR );
			return false;

		case XK_F8:
			Screen::message( "No kiosk mode in this release!", 3 );
			return true;

#if 0
			_action = Panel::Kiosk;
			setName( KIOSK_STR );
			setPassword( KeyUtils::getKioskPassword() );
			return false;

		case XK_F12:
			_action = Panel::Console;
			setName( CONSOLE_STR );
			return false;
#endif

		case XK_F11:
			takeSnapshot();
			return true;

		case XK_Return:
		case XK_KP_Enter:

			_panels->showPassword( std::string() );

			if (_field == Get_Name) {
				std::string name( _panels->getName() );

				if ( name.empty() ) {
					Screen::bell(100);
					return true;
				}
#if 0
				_panels->focus( "PasswordPanel" );
				if (name == CONSOLE_STR) {
					_action = Panel::Console;
				} else
#endif
				if (name == KIOSK_STR) {
					_action = Panel::Kiosk;
				} else if (name == SHUTDOWN_STR) {
					_action = Panel::Shutdown;
				} else if (name == REBOOT_STR) {
					_action = Panel::Reboot;
				} else if (name == SUSPEND_STR) {
					_action = Panel::Suspend;
				} else if (name == EXIT_STR) {
					_action = Panel::Exit;
				} else {
					_action = Panel::Login;
				}
//		} else if (_field == Get_Passwd) {
//			_panels->focus( "UsernamePanel" );
			}

			return false;

		default:
		break;
	};

	std::string password( _panels->getPassword() );
	std::string name( _panels->getName() );

	switch ( keysym ) {
		case XK_Delete:
		case XK_BackSpace:
			switch ( _field ) {
				case GET_NAME:
					if ( !name.empty() ) {
						name.erase( --name.end() );
						_panels->showUsername( name );
					}
				break;
				case GET_PASSWD:
					if ( !password.empty() ) {
						password.erase( --password.end() );
						std::string hidden( password.length(), '*' );
						_panels->showPassword( hidden );
						setPassword( password );
					}
				break;
			}
			break;

		case XK_w:
		case XK_u:
			if (reinterpret_cast<XKeyEvent&>(event).state & ControlMask) {
				switch ( _field ) {
					case Get_Name:
						_panels->showUsername( std::string() );
					break;
					case Get_Passwd:
						password.clear();
						_panels->showPassword( std::string() );
						setPassword( std::string() );
					break;
				}
				break;
			}
			/* Deliberate fall-through */

		default:
			if (isprint(ascii) && (keysym < XK_Shift_L || keysym > XK_Hyper_R)) {
				switch ( _field ) {
					case GET_NAME:
						if (name.length() < INPUT_MAXLENGTH_USERNAME-1) {
							name.append( &ascii, 1 );
							_panels->showUsername( name );
						}
					break;
					case GET_PASSWD:
						if (password.length() < INPUT_MAXLENGTH_PASSWORD-1) {
							password.append( &ascii, 1 );
							std::string hidden( password.length(), '*' );
							_panels->showPassword( hidden );
							setPassword( password );
						}
					break;
				}
			}
		break;
	}

	return true;
}

//-----------------------------------------------------------------------------
void Screen::createCursors()
//-----------------------------------------------------------------------------
{
	_cursors.move = XCreateFontCursor( _x11.dpy, XC_fleur );
	_cursors.pointer = XCreateFontCursor( _x11.dpy, XC_left_ptr );
	::XDefineCursor( _x11.dpy, _x11.rootwin, _cursors.pointer );
}

//-----------------------------------------------------------------------------
void Screen::releaseCursors()
//-----------------------------------------------------------------------------
{
	::XFreeCursor( _x11.dpy, _cursors.move );
	::XFreeCursor( _x11.dpy, _cursors.pointer );
	::XDefineCursor( _x11.dpy, _x11.rootwin, None );
}

//-----------------------------------------------------------------------------
jmp_buf CloseEnv;
int Screen::IgnoreXIO(Display *d)
//-----------------------------------------------------------------------------
{
	::syslog( LOGFLAGS, "Connection to display server lost !" );
	::longjmp( CloseEnv, 1 );
}

//-----------------------------------------------------------------------------
void Screen::stopServer(Panel::Action action)
//-----------------------------------------------------------------------------
{
	::signal( SIGQUIT, SIG_IGN );
	::signal( SIGINT, SIG_IGN );
	::signal( SIGHUP, SIG_IGN );
	::signal( SIGPIPE, SIG_IGN );
	::signal( SIGUSR1, SIG_IGN );
	::signal( SIGUSR2, SIG_IGN );
	::signal( SIGTERM, SIG_DFL );
	::signal( SIGKILL, SIG_DFL );

	_panels->release();

	releaseCursors();

	if (Brzdm::BRZDM->noPasswdHalt()) {
		_nopasswd_halt = true;
		::signal( SIGALRM, Screen::disallow );
		::alarm( 15 );
	}

	switch( action ) {
		case Panel::Reboot:
		case Panel::Shutdown:
		case Panel::Suspend:
		case Panel::Exit:
			if (Screen::session( Screen::X11 )) {
				if( _x11.dpy ) {
					Imlib::release();
					XCloseDisplay( _x11.dpy );
					_x11.dpy = 0L;
				}
			} else {
			}
		break;
		default:
			if (Screen::session( Screen::X11 )) {
				XSetIOErrorHandler( Screen::IgnoreXIO );

				if (_x11.dpy && !::setjmp( CloseEnv )) {
					Imlib::release();
					XCloseDisplay( _x11.dpy );
					_x11.dpy = 0L;
				}
			} else {
			}
		break;
	}

	/* Send HUP to process group */
	errno = 0;

	if ((::killpg( ::getpid(), SIGHUP) != 0) && (errno != ESRCH)) {
		::syslog( LOGFLAGS, "Cannot send HUP to process group !" );
	}

	/* Send TERM to server */
	if(getServerPID() < 0)
		return;

	errno = 0;

	if (::killpg( getServerPID(), SIGTERM ) < 0) {
		if (errno == EPERM) {
			::syslog( LOGFLAGS, "Cannot kill display server !" );
			std::exit(ERR_EXIT);
		}

		if (errno == ESRCH) {
			return;
		}
	}

	// Wait for server to shut down
	if (!timeout( 10, (char*) "X server to shut down" )) {
		return;
	}

	if ( testMode() ) {
		std::cerr << "X server slow to shut down, sending KILL signal.\n";
		std::cerr.flush();
	}

	errno = 0;

	// Send KILL to server
	if (killpg( getServerPID(), SIGKILL ) < 0) {
		if(errno == ESRCH)
			return;
	}

	if (timeout(3, (char*) "Waiting for server to die")) {
		::syslog( LOGFLAGS, "Cannot kill display server !" );
		std::exit(ERR_EXIT);
	}
}

//-----------------------------------------------------------------------------
int Screen::timeout(int delay, char* text)
//-----------------------------------------------------------------------------
{
	int pid = -1;
	int	i = 0;

	for (;;) {
		pid = ::waitpid( getServerPID(), NULL, WNOHANG );

		if (pid == getServerPID())
			break;

		if ( delay ) {
			if ( testMode() ) {
				std::cerr << "brzdm: waiting for " << text << "\n";
			}
			::sleep(1);
		}

		if (++i > delay) {
			break;
		}
	}
	return getServerPID() != pid;
}

//-----------------------------------------------------------------------------
int Screen::waitForServer()
//-----------------------------------------------------------------------------
{
	int	ncycles = 120;
	int	cycles;

	for (cycles = 0; cycles < ncycles; cycles++) {

		if ((_x11.dpy = XOpenDisplay( _dpy_name.c_str() ))) {
			XSetIOErrorHandler( Brzdm::xioerror );
			XFlush( _x11.dpy );
			return 1;
		}

		if (!timeout(1, (char*)"X server to begin accepting connections" ))
			break;
	}

	std::cerr << "Giving up.\n";
	std::cerr.flush();

	return 0;
}

//-----------------------------------------------------------------------------
int Screen::startServer(bool daemon_mode, bool fork_server)
//-----------------------------------------------------------------------------
{
	int errcode = (0);

	if ( _dpy_name.empty() ) {
		char* display = ::getenv( "DISPLAY" );

		if (display && display[0]) {
			_dpy_name = display;
		} else {
			_dpy_name = DISPLAY;
		}
	}

	if (_config->get( "Display/type" ) == "wayland") {
		_session_type = Screen::Wayland;
	} else {
		_session_type = Screen::X11;
	}

	if ( fork_server ) {
		Brzdm::BRZDM->createServerAuth();
		errcode = forkServer( daemon_mode );
	}

	if (Screen::session( Screen::X11 )) {
		errno = 0;

		if ( !fork_server && !_x11.dpy ) {
			_x11.dpy = XOpenDisplay( _dpy_name.c_str() );
		}

		if ( !_x11.dpy ) {
			::syslog( LOGFLAGS, "Unable to open display '%s' '%s'", _dpy_name.c_str(), strerror(errno));
			stopServer( Panel::Login );
			std::exit( ERR_EXIT );
		}

		if (_config->getBool( "XRandr/enabled" )) {
			std::string cmd( _config->get( "XRandr/command" ));

			if (cmd.find( "randr" ) != std::string::npos)
				std::system( cmd.c_str() );

			Screen::sync();
		}

		_x11.screen = DefaultScreen( _x11.dpy );
		_x11.rootwin = RootWindow( _x11.dpy, _x11.screen );

		_screen_w = XWidthOfScreen( ScreenOfDisplay( _x11.dpy, _x11.screen ));
		_screen_h = XHeightOfScreen( ScreenOfDisplay( _x11.dpy, _x11.screen ));

		createCursors();
		selectInput( true );

		if ( testMode() ) {
			std::cerr << "Opened dpy '" << _dpy_name.c_str() <<  "' -- " << strerror(errno) << "\n";
			std::cerr << "Screen " << _screen_w << "x" << _screen_h << "+0+0\n";
			std::cerr.flush();
		}
	} else {
	}

	return errcode;
}

//-----------------------------------------------------------------------------
int Screen::forkServer(bool daemon_mode)
//-----------------------------------------------------------------------------
{
	std::string arguments( _config->get( "Display/arguments" ));
	std::string srvcmd( _config->get( "Display/server" ));
	static const int MAX_XSERVER_ARGS = 256;
	static char* server[MAX_XSERVER_ARGS+2] = {0L};
	bool hasVtSet = false;
	int argc = 1, pos = 0;

	server[0] = (char*) srvcmd.c_str();

	/* Add mandatory xauthority option */
	arguments += " -auth ";
	arguments += _config->get( "Auth/file" );

	char* args = new char[ arguments.length()+2 ];
	std::strcpy( args, arguments.c_str() );

	while (args[pos] != '\0') {

		if (args[pos] == ' ' || args[pos] == '\t') {
			*(args+pos) = '\0';
			server[argc++] = args + pos + 1;
		} else if (pos == 0) {
			server[argc++] = args + pos;
		}

		++pos;

		if (argc+1 >= MAX_XSERVER_ARGS) {
			/* ignore _all_ arguments to make sure the server starts at all */
			argc = 1;
			break;
		}
	}

	for (int i=0; i < argc; i++) {
		if (server[i][0] == 'v' && server[i][1] == 't') {
			bool ok = false;
			Config::string2int( server[i]+2, &ok );
			if (ok) { hasVtSet = true; }
		}
	}

	if ( !hasVtSet && daemon_mode ) {
		server[argc++] = (char*)"vt07";
	}

	server[argc] = 0L;

	if ( testMode() ) {
		for (int i=0; i < argc; i++)
			std::cerr << "X11/Xorg Arg[" << i << "]='" << server[i] << "'\n";
	}

	_server_pid = fork();
	_server_started = false;

	switch (getServerPID()) {
		case -1:
			::syslog( LOGFLAGS, "Unable to start display server !" );
			std::exit( ERR_EXIT );
		break;
		case 0:
			signal(SIGTTIN, SIG_IGN);
			signal(SIGTTOU, SIG_IGN);
			signal(SIGUSR1, SIG_IGN);
			signal(SIGALRM, SIG_IGN);

			setpgid( 0,getpid() );
			execvp( server[0], server );

			std::cerr << "X server could not be started\n";
			std::cerr.flush();

			::syslog( LOGFLAGS, "Unable to start display server !" );

			std::exit( ERR_EXIT );
		break;
		default:
			errno = 0;

			if (!timeout(0, (char*) "")) {
				_server_pid = -1;
				break;
			}

			if (waitForServer() == 0) {
				std::string mesg( "Unable to connect to display server !" );
				::syslog( LOGFLAGS, "%s", mesg.c_str() );
				stopServer( Panel::Login );
				_server_pid = -1;
				std::exit(ERR_EXIT);
			}
		break;
	}

	delete [] args;

	_server_started = _server_pid > 0;

	return _server_pid;
}

//-----------------------------------------------------------------------------
void Screen::getGeometry(Window wid, int& x, int& y, uint& w, uint& h)
//-----------------------------------------------------------------------------
{
	uint depth;
	uint border;
	Window root;

	::XGetGeometry(
		_x11.dpy, wid, &root, &x, &y, &w, &h, &border, &depth
	);
}

//-----------------------------------------------------------------------------
void Screen::clearArea(Window wid, int w, int h, int x, int y)
//-----------------------------------------------------------------------------
{
	long retcode = ::XClearArea(
		_x11.dpy,
		wid ? wid : _x11.rootwin,
		x, y, w, h,
		wid ? False : False
	);

	if (retcode != Success && testMode()) {
		std::cerr << "Screen::clearText FAILED DRAW ERRCODE=" << retcode << " WID=" << wid << " " << w << "x" << h << "+" << x << "+" << y << "\n";
		std::cerr.flush();
	}
}

//-----------------------------------------------------------------------------
Window Screen::create(Window parent, int w, int h, int x, int y, uint inputmask, int border) 
//-----------------------------------------------------------------------------
{
	Display *dpy = Screen::display();
	Visual *visual = DefaultVisual( dpy, Screen::screenNb() );
	XSetWindowAttributes winattr;
	unsigned long valuemask;
	Window window;

	if (parent == Screen::rootWindow()) {

		valuemask = CWEventMask|CWSaveUnder|CWOverrideRedirect;
		valuemask |= CWBackingStore|CWBackPixmap|CWColormap;

		winattr.save_under = 1;
		winattr.override_redirect = 1;
		winattr.background_pixmap = CopyFromParent|ParentRelative;
		winattr.backing_store = Always;
		winattr.colormap = CopyFromParent;
		winattr.event_mask =
			StructureNotifyMask|EnterWindowMask|LeaveWindowMask|
			ExposureMask|ButtonPressMask|ButtonReleaseMask|
			OwnerGrabButtonMask|KeyPressMask|KeyReleaseMask;

		window = XCreateWindow(
			dpy, parent, x, y, w, h, border,
			CopyFromParent,
			CopyFromParent|InputOutput,
			visual,
			valuemask,
			&winattr
		);
	} else {
		window = XCreateSimpleWindow(
			dpy, parent,
			x, y, w, h,
			0, // border
			0x00,
			0x00 // background
		);
	}

	::XSelectInput( dpy, window, ExposureMask|inputmask );
	::XMapWindow( dpy, window );
	::XSync( dpy, false );
	::XFlush( dpy );

	return window;
}

#if 0
//-----------------------------------------------------------------------------
void Screen::grabKeyboard(Window window)
//-----------------------------------------------------------------------------
{
	if (Screen::session( Screen::X11 )) {
		::XGrabKeyboard(
			_x11.dpy,
			window, False,
			GrabModeAsync, GrabModeAsync,
			CurrentTime
		);
	} else {
	}
}

//-----------------------------------------------------------------------------
void Screen::ungrabKeyboard()
//-----------------------------------------------------------------------------
{
	if (Screen::session( Screen::X11 )) {
		::XUngrabKeyboard( _x11.dpy, CurrentTime );
	} else {
	}
}
#endif

//-----------------------------------------------------------------------------
int Screen::CatchErrors(Display *dpy, XErrorEvent *ev)
//-----------------------------------------------------------------------------
{
	return 0;
}

//-----------------------------------------------------------------------------
void Screen::killAllClients(bool fromtop)
//-----------------------------------------------------------------------------
{
	unsigned int nchildren = 0;
	unsigned int i = 0;

	if (Screen::session( Screen::X11 )) {

		XWindowAttributes attr;
		Window dummywindow;
		Window *children;

		::XSync( _x11.dpy, false );
		::XSetErrorHandler( Screen::CatchErrors );

		XQueryTree(
			_x11.dpy, Screen::rootWindow(),
			&dummywindow, &dummywindow,
			&children, &nchildren
		);

		if ( !fromtop ) {
			for (i=0; i<nchildren; i++) {
				if (XGetWindowAttributes(_x11.dpy, children[i], &attr) &&
					(attr.map_state == IsViewable))
				{
					children[i] = XmuClientWindow( _x11.dpy, children[i] );
				} else {
					children[i] = 0;
				}
			}
		}

		for (i=0; i<nchildren; i++) {
			if ( children[i] ) {
				XKillClient( _x11.dpy, children[i] );
			}
		}
		XFree((char *)children );
		XSync( _x11.dpy, false );
		XSetErrorHandler( 0L );
	}
	else {
	}
}

};

