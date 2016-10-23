// Brzdm - Breeze::OS Login/Display Manager
//
// Copyright (C) 2015 Tsert.Inc <contact@tsert.com>
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
	enum Type { Pane, Button, Message };

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
	//Pixmap _pixmap;

	std::string _cmd;
	std::string _name;
	std::string _spec;
	std::string _text;
	std::string _font;
	std::string _mode;

	RGBA _rgba;
	RGBA _font_rgba;

	bool _ondemand;
	bool _text_shadow;

	int _text_x;
	int _text_y;
	int _w, _h;
	int _x, _y;
	int _z_idx;
	Imlib::Alignment _align;
	Imlib_Text_Direction _rtl;

public:
	Panel(Panel::Type t=Panel::Pane)
	{
		_type = t;
		_x11win = 0L;
		_parent = 0L;
		_rtl = IMLIB_TEXT_TO_RIGHT;
	}
	Panel(Panel::Type t, const std::string& n)
	{
		_type = t;
		_name = n;
		_x11win = 0L;
		_parent = 0L;
		_rtl = IMLIB_TEXT_TO_RIGHT;
	}
	~Panel() { destroy(); }

	int compare(const Panel& elem) const
		{
			int result = zIndex() - elem.zIndex();
			if (result == 0)
				return _name.compare( elem._name );
			return result;
		}

	bool isa(Panel::Type t) const { return _type == t; }
	bool onDemand() const { return _ondemand; }

	const std::string& spec() const { return _spec; }
	const std::string& drawmode() const { return _mode; }
	const std::string& getName() const { return _name; }

	Imlib_Image image() const { return _image; }
	Window window() const { return _x11win; }

	int x() const { return _x; }
	int y() const { return _y; }
	int textX() const { return _text_x; }
	int textY() const { return _text_y; }
	int width() const { return _w; }
	int height() const { return _h; }
	int zIndex() const { return _z_idx; }
	Imlib_Text_Direction direction() const { return _rtl; }

	void setX(int x) { _x = x; }
	void setY(int y) { _y = y; }
	void setWidth(int w) { _w = w; }
	void setHeight(int h) { _h = h; }
	void setTextX(int x) { _text_x = x; }
	void setTextY(int y) { _text_y = y; }
	void setZIndex(int idx) { _z_idx = idx; }

	void setImage(Imlib_Image img) { _image = img; }
	void setParent(Panel *parent) { _parent = parent; }
	void setOnDemand(bool flag) { _ondemand = flag; }
	void setTextShadow(bool flag) { _text_shadow = flag; }

	void setColor(const RGBA& rgba) { _rgba = rgba; }
	void setColor(const std::string& color) { Imlib::setRGBA( color, &_rgba ); }

	void setDrawMode(const std::string& mode) { _mode = mode; }

	void setFont(const std::string& font) { _font = font; }
	void setFontColor(const std::string& color)
		{ Imlib::setRGBA( color, &_font_rgba ); }

public:
	void clear();
	void create();
	void destroy();

	void show();
	void showDate();
	void showClock();

	void showText();
	void showText(const std::string& text, bool shadow=false);

	void setInput(const std::string& text, bool shadow=false);
	void setText(const std::string& text, bool shadow, bool windowed=false);
	void setBackground(const std::string& spec, const std::string& themedir, bool shadow=false);
	void setButtons(const std::string& img, bool shadow=false);

	void setAlignment(const std::string& spec);
	void setDirection(const std::string& spec);
	void setPosition(const std::string& spec, Panel *parent);

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

	bool load(Config *config);

	Panel::Action getAction() const;

	Panel *get(Window wid);
	Panel *get(int x, int y);
	Panel *get(const std::string& name, bool dotake=false);

	void zap();
	void show();
	//void clear();
	//void clear(const std::string& name);
	void focus(const std::string& name);

	void release();
	void release(const std::string& name);

	void prevSession();
	void nextSession();
	void showSession();
	void resetSession();

	void showClock();
	void showEnterMesg();
	void showMessage(const std::string& text);

	void reset();
	void resetName();
	void resetPassword();

	void setName(const std::string& name);
	void setPassword(const std::string& pw);

	const std::string& getName() const;
	const std::string& getPassword() const;
	const std::string& getSession() const;

protected:
	bool scanDesktops(const std::string& folder);
	bool init(Config *theme);

protected:
	Panel::Action _action;
	std::string _namebuf;
	std::string _password;
	std::string _hidden_passwd;
	std::vector<std::string> _sessions;
	Config *_themecfg;
	uint _curidx;

};

};

#endif /* _PANEL_H_ */
