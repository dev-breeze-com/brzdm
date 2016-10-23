// Brzdm - Breeze::OS Login/Display Manager
//
// Copyright (C) 2015 Tsert.Inc <contact@tsert.com>
// Copyright (C) 1997, 1998 Per Liden
// Copyright (C) 2004-06 Simone Rota <sip@varlock.com>
// Copyright (C) 2004-06 Johannes Winkelmann <jw@tks6.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
#ifndef _BRZDM_SCREEN_H_
#define _BRZDM_SCREEN_H_

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
#include <X11/cursorfont.h>
#include <X11/Xmu/WinUtil.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>

#ifdef NEEDS_BASENAME
#include <libgen.h>
#endif

#include "imlib.h"
#include "utils.h"
#include "panel.h"
#include "switchuser.h"

namespace breeze {

typedef std::map<std::string, XftFont*> XftFonts;

typedef struct X11Info {
	Window window;
	Pixmap rootpix;
	Window rootwin;
	Visual* visual;
	Display *dpy;
	int screen;
	int depth;
} X11Info;

typedef struct Cursors {
	Cursor move;
	Cursor pointer;
} Cursors;

typedef struct WaylandInfo {
	int screen;
	int depth;
} WaylandInfo;

class Screen {
public:
	enum SessionType { X11, Wayland };
	enum FieldType { Get_Name, Get_Passwd };

	static int IgnoreXIO(Display *dpy);

private:
	Screen() {}
	~Screen() {}

public:
	static void init(Config *cfg, Panels *panels, const std::string& dpy="");

	static void open();
	static void close(Panel::Action);
	static void bell(int);
	static void tick(int);
	static void sync();

	static void blank();
	static void showText();

	static void destroy(Window wid);
	static void clear(Window wid=0);
	static void clear(const std::string& pane);

	static void ungrabKeyboard();
	static void grabKeyboard(Window);

	static void setName(const std::string& name);
	static void setPassword(const std::string& pw);

	static void takeSnapshot();
	static void setBackground();
	static void setBackground(const std::string& url);

	static void switchSession();
	static void showSession(XftDraw *drawarea);

	static void disallow(int sig);
	static void clearmesg(int sig);

	static void errmesg(const std::string& text, int secs=0);
	static void message(const std::string& text, int secs=0);
	static void warning(const std::string& text, int secs=0);
	static void mesg(Panel*, const std::string& text, int secs=0);

	static void eventHandler(const FieldType& curfield);
	static void selectInput(bool);

	static void stopServer(Panel::Action);
	static int startServer(bool asdaemon, bool forked);

	static int CatchErrors(Display *dpy, XErrorEvent *ev);
	static void killAllClients(bool fromtop);

public:
	static std::string getSession();
	static Panel::Action getAction();

	static int width();
	static int height();
	static int screenNb();

	static char* displayName();
	static Visual *visual();
	static Display* display();
	static Colormap colormap();
	static Window rootWindow();
	static FieldType field();

	static void createServerAuth();
	static int getServerPID();

	static bool noPasswdHalt();
	static bool isServerStarted();
	static bool session(SessionType);

	static void EventHandler(const Screen::FieldType& curfield);

	static void getGeometry(Window, int& x, int& y, uint& w, uint& h);

	static void clearArea(Window, int w, int h, int x, int y);

	static Window create(Window parent, int w, int h, int x, int y,
		uint inputmask, int border);

	//static void drawText(const std::string& text, int x, int y);
	//static void drawX11Text(const std::string& text,
	//const std::string& font_name, const String& color_name, int x, int y);

private:
	static bool secureLogin();

	static int forkServer(bool);
	static int waitForServer();
	static int timeout(int delay, char* text);

	static void createCursors();
	static void releaseCursors();

	static void onExpose(XEvent& event);
	static bool onKeyPress(XEvent& event);
	static bool onButtonRelease(XEvent& event);

private:
	static struct timeval _now;
	static std::string _dpy_name;
	static std::string _mcookie;
	static SessionType _session_type;
	static Panel::Action _action;
	static Panels *_panels;
	static FieldType _field;
	static Config *_config;
	static X11Info _x11;
	static XftFonts _fonts;
	static Cursors _cursors;

	static int _screen_w;
	static int _screen_h;

	static int _server_pid;
	static int _server_started;

	static bool _nopasswd_halt;
	/*
	GC TextGC;
	int X, Y;
	int W, H;
	*/
};

};

#endif
