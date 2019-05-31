// Brzdm - Breeze::OS Login/Display Manager
//
// @author Pierre Innocent <dev@breezeos.com>
// Copyright (C) Tsert.Inc, All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _PANELS_H_
#define _PANELS_H_

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
#include <X11/cursorfont.h>
#include <X11/Xmu/WinUtil.h>

#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>

#ifdef NEEDS_BASENAME
#include <libgen.h>
#endif

#include "utils.h"
#include "config.h"
#include "imlib.h"

namespace breeze {

class Panel {
public:
	enum Type {
		Pane=0x0001,
		Button=0x0002,
		Message=0x0004,
		Input=0x0008,
		Background=0x0010,

		TextPane=Pane|0x0100,
		InputPane=Pane|Input|0x0200,
		PasswdPane=Pane|Input|0x0400,
		PhotoPane=Button|0x0800,

		Static=0x1000
	};

	enum Action {
		Noop=0,
		Login,
		SecureLogin,
		Console,
		Reboot,
		Halt,
		Shutdown=Halt,
		Suspend,
		Kiosk,
		Exit
	};

protected:
	Panel *_parent;
	Panel::Type _type;
	Imlib_Image _image;
	Window _x11win;
	//Imlib_Updates _updates;
	//Pixmap _pixmap;

	std::string _cmd;
	std::string _name;
	std::string _spec;
	std::string _text;
	std::string _font;
	std::string _mode;

	RGBA _rgba;
	RGBA _font_rgba;

	bool _dirty;
	bool _ondemand;
	bool _shadow;
	bool _text_shadow;

	int _w, _h;
	int _x, _y;
	int _tx, _ty;
	int _z_idx;

	Imlib::Alignment _align;
	Imlib_Text_Direction _rtl;

public:
	Panel(Panel::Type t=Panel::Pane);
	Panel(Panel::Type t, const std::string& n);
	virtual ~Panel();

public:
	int compare(const Panel& elem) const;

	bool dirty() const { return _dirty; }
	bool onDemand() const { return _ondemand; }
	bool withShadow() const { return _shadow; }
	bool isa(Panel::Type mask) const { return mask == (_type & mask); }

	const std::string& spec() const { return _spec; }
	const std::string& drawmode() const { return _mode; }
	const std::string& getName() const { return _name; }
	const std::string& getText() const { return _text; }

	Imlib_Image image() const { return _image; }
	Window window() const { return _x11win; }
	//Pixmap pixmap() const { return _pixmap; }
	Panel *parent() const { return _parent; }

	int x() const { return _x; }
	int y() const { return _y; }
	int width() const { return _w; }
	int height() const { return _h; }
	int zIndex() const { return _z_idx; }

	int textX() const { return _tx; }
	int textY() const { return _ty; }

	Imlib_Text_Direction direction() const { return _rtl; }

	void setX(int x) { _x = x; }
	void setY(int y) { _y = y; }
	void setWidth(int w) { _w = w; }
	void setHeight(int h) { _h = h; }
	void setZIndex(int idx) { _z_idx = idx; }

	void setTextX(int x) { _tx = x; }
	void setTextY(int y) { _ty = y; }

	void setType(Panel::Type t) { _type = t; }
	void setImage(Imlib_Image img) { _image = img; }
	//void setPixmap(Pixmap img) { _pixmap = img; }
	void setParent(Panel *parent) { _parent = parent; }

	void setDirty(bool flag) { _dirty = flag; }
	void setOnDemand(bool flag) { _ondemand = flag; }
	void setTextShadow(bool flag) { _text_shadow = flag; }

	void setColor(const RGBA& rgba) { _rgba = rgba; }
	void setColor(const std::string& color) { Imlib::setRGBA( color, &_rgba ); }

	void setDrawMode(const std::string& mode) { _mode = mode; }

	void setFont(const std::string& font) { _font = font; }
	void setFontColor(const std::string& color)
		{ Imlib::setRGBA( color, &_font_rgba ); }

public:
	Window parentWindow() const;
	Imlib_Image parentImage() const;

	void clear();
	void create();
	void destroy();

	void show();
	void showDate();
	void showClock();

	void setStatic(bool);

	void setValue(const std::string& text);
	void setText(const std::string& text, bool shadow=false);
	void showText(const std::string& text);

	void setAlignment(const std::string& spec);
	void setDirection(const std::string& spec);
	void setPosition(const std::string& spec, Panel *parent);

	void newButton(const std::string& img, bool shadow=false);
	void newText(const std::string& text, bool shadow, bool windowed=false);
	void newInput(const std::string& text, bool shadow=false, bool windowed=false);
	void newBackground(const std::string& spec, const std::string& themedir,
		bool shadow=false, bool forcebg=false);

};

struct ComparePanel {
public:
	ComparePanel() {}
	~ComparePanel() {}
		bool operator()(Panel *item, Panel *elem) {
		return item->compare( *elem ) > 0;
	}
};

class Panels : std::set<Panel*,ComparePanel> {
public:
	Panels() { _curidx = 0; }
	~Panels() { release(); }

	bool load(Config *config, const std::string& bgfile=std::string());

	Panel::Action getAction() const;

	Panel *get(Window wid) const;
	Panel *get(int x, int y) const;
	Panel *get(const std::string& name) const;

	Imlib_Image getBackground() const;

	void zap();
	//void show();
	void release();

	//void clear();
	//void clear(const std::string& name);
	//void release(const std::string& name);
	//void focus(const std::string& name);

	void prevUser();
	void nextUser();
	void setUser(int);

	void prevSession();
	void nextSession();
	void showSession();

	void resetSession();
	void setSession(int);

	void reset();
	void resetUsername();
	void resetPassword();

	void showClock();
	void showEnterMesg();

	void setUsername(const std::string& name);
	void setPassword(const std::string& text);

	void showUsername(const std::string& text);
	void showPassword(const std::string& text);

	const std::string& getName() const;
	const std::string& getUser() const;
	const std::string& getPassword() const;
	const std::string& getSession() const;

protected:
	bool init(Config *theme);
	bool scanDesktops(const std::string& folder,
		const std::vector<std::string>& xpaths);

protected:
	Panel::Action _action;
	std::string _namebuf;
	std::string _password;
	std::string _hidden_passwd;

	std::vector<std::string> _users;
	std::vector<std::string> _sessions;

	Config *_themecfg;
	int _curidx;
	int _curuser;

};

};

#endif /* _PANEL_H_ */
