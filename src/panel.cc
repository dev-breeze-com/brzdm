// Brzdm - Simple Login/Display Manager
//
// @author Pierre Innocent <dev@breezeos.com>
// Copyright (C) Tsert.Inc, All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
#include <sys/types.h>
#include <dirent.h>

#include <sstream>
#include <poll.h>
#include <sys/stat.h>

#include "utils.h"
#include "imlib.h"
#include "screen.h"
#include "config.h"
#include "panel.h"

namespace breeze {

//-----------------------------------------------------------------------------
void Panel::setButtons(const std::string& button, bool shadow)
//-----------------------------------------------------------------------------
{
	_image = Imlib::load( button, shadow );
}

//-----------------------------------------------------------------------------
void Panel::setInput(const std::string& position, bool shadow)
//-----------------------------------------------------------------------------
{
	Window wid = Screen::rootWindow();
	uint inputmask =
		KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask;

	_image = Imlib::rectangle( 0L, _rgba, _w, _h, shadow );

	setPosition( position, _parent );

	_x11win = Screen::create( wid, _w, _h, _x, _y, inputmask, 1 );
//std::cout << "setInput Name=" << _name << " " << _w << "x" << _h << "+" << _x << "+" << _y << "\n";
//std::cout.flush();
}

//-----------------------------------------------------------------------------
void Panel::setText(const std::string& text, bool shadow, bool windowed)
//-----------------------------------------------------------------------------
{
	//Window wid = Screen::rootWindow();
	//uint inputmask = 0;

	_text = text;

	_w = _w > 0 ? _w : Screen::width() / 2;
	_x = _x > 0 ? _x : (Screen::width() - _w) / 2;

	setPosition( "", _parent );

	_image = Imlib::rectangle( 0L, _rgba, _w, _h, shadow );

//_x11win = Screen::create( wid, _w, _h, _x, _y, inputmask, 0 );
//std::cout << "setText Name=" << _name << " !windowed " << _w << "x" << _h << "+" << _x << "+" << _y << "\n";
//std::cout.flush();
}

//-----------------------------------------------------------------------------
void Panel::setBackground(const std::string& bg, const std::string& themedir, bool shadow)
//-----------------------------------------------------------------------------
{
	std::string::size_type npos = std::string::npos;
	std::string spec( bg );
	std::string gradtype;
	RGBA rgba, ergba;

	bool gradient = spec.find( "gradient" ) == 0;

	_spec = spec;

	if ( gradient ) {
		spec.erase( 0, 8 );
	}

	Utils::strtrim( spec );

	std::string::size_type offset = spec.find( '#' );

	if (offset != std::string::npos) {

		_w = _w > 0 ? _w : Screen::width();
		_h = _h > 0 ? _h : Screen::height();

		if (offset == 0) {
			Imlib::setRGBA( spec, &rgba );
			_image = Imlib::rectangle( 0L, rgba, _w, _h, shadow );
		}
		else {
			gradtype = spec.substr( 0, offset );
			Utils::strtrim( gradtype );

			spec.erase( 0, offset );
			Utils::strtrim( spec );

			if ((offset = spec.find( ' ' )) != npos) {

				std::string rgbstr( spec.substr( 0, offset ));
				Utils::strtrim( rgbstr );
				Imlib::setRGBA( rgbstr, &rgba );

				spec.erase( 0, offset );
				Utils::strtrim( spec );
				Imlib::setRGBA( spec, &ergba );
			}
			_image = Imlib::gradient( 0L, gradtype, rgba, ergba, _w, _h );
		}
	} else if (_name == "Background") {

		_w = Screen::width();
		_h = Screen::height();

		_image = Imlib::loadSized( themedir, "png" );
		_image = _image ? _image : Imlib::loadSized( themedir, "jpg" );
		_image = _image ? _image : Imlib::load( themedir+spec );

	} else {
		_image = Imlib::load( themedir+spec, shadow );
		_w = Imlib::width( _image );
		_h = Imlib::height( _image );
		//_x11win = Screen::create( wid, _w, _h, _x, _y, inputmask, 0 );
	}
//std::cout << "setBackground Name=" << _name << " " << _w << "x" << _h << "+" << _x << "+" << _y << "\n";
}

//-----------------------------------------------------------------------------
void Panel::setDirection(const std::string& direction)
//-----------------------------------------------------------------------------
{
	if (direction == "left") {
		_rtl = IMLIB_TEXT_TO_LEFT;
	} else if (direction == "down") {
		_rtl = IMLIB_TEXT_TO_DOWN;
	} else if (direction == "up") {
		_rtl = IMLIB_TEXT_TO_UP;
	} else {
		_rtl = IMLIB_TEXT_TO_RIGHT;
	}
}

//-----------------------------------------------------------------------------
void Panel::setAlignment(const std::string& alignment)
//-----------------------------------------------------------------------------
{
	if (alignment == "center") {
		_align = Imlib::CENTER;
	} else if (alignment == "right") {
		_align = Imlib::RIGHT;
	} else if (alignment == "left") {
		_align = Imlib::LEFT;
	} else {
		_align = Imlib::NOALIGN;
	}
}

//-----------------------------------------------------------------------------
void Panel::setPosition(const std::string& position, Panel *parent)
//-----------------------------------------------------------------------------
{
	if ( parent ) {
		if (position == "bottom-right") {
			_x = parent->width() - _w;
			_y = parent->height() - _h;
		} else if (position == "bottom-left") {
			_x = parent->x();
			_y = parent->height() - _h;
		}
		else if (position == "top-right") {
			_x = parent->width() - _w;
			_y = parent->y();
		} else if (position == "top-left") {
			_x = parent->x();
			_y = parent->y();
		}
		else
		if (parent->window()) {
			_x += parent->x();
			_y += parent->y();
		} else {
			_x += parent->x();
			_y += parent->y();
		}
	}
}

//-----------------------------------------------------------------------------
void Panel::destroy()
//-----------------------------------------------------------------------------
{
	Panel::clear();

	if ( _image ) {
		Imlib::release( _image );
		_image = 0L;
	}

	if (Screen::session( Screen::X11 )) {
		if ( _x11win ) {
			Screen::destroy( _x11win );
			_x11win = 0L;
		}
	} else {
	}
}

//-----------------------------------------------------------------------------
void Panel::clear()
//-----------------------------------------------------------------------------
{
	if ( image() ) {
		Imlib::clear( image() );
	}

	if (Screen::session( Screen::X11 )) {
		if ( window() ) {
			Screen::clear( window() );
		} else {
			Screen::clearArea( 0L, width(), height(), x(), y() );
		}
		Screen::sync();
	}
	else { // Wayland
	}
}

//-----------------------------------------------------------------------------
void Panel::showText(const std::string& text, bool shadow)
//-----------------------------------------------------------------------------
{
	Window wid = _x11win ? _x11win : Screen::rootWindow();
	int x = _x11win ? 0 : _x;
	int y = _x11win ? 0 : _y;

	//std::cout << "Panel::showText() '" << _name << "\n";
	//std::cout.flush();

	if (_name == "InputPanel" ||
		_name == "UsernamePanel" ||
		_name == "PasswordPanel")
	{
		Imlib::rectangle( _image, _rgba, _w, _h, shadow );
	} else {
		Panel::clear();
	}

	Imlib::drawText( _image, text, _font, _font_rgba, textX(), textY(), _align, _rtl );
	Imlib::draw( _image, wid, x, y );

	Screen::sync();
}

//-----------------------------------------------------------------------------
void Panel::show()
//-----------------------------------------------------------------------------
{
	//std::cout << "Panel::show() '" << _name << "\n";
	//std::cout.flush();

	if ( !_text.empty() ) {
		showText( _text );

	} else if (_image && _x11win) {
		Imlib::draw( _image, _x11win, 0, 0 );

	} else if ( _image ) {

		if (isa( Panel::Button )) {
			Imlib::draw( _image, Screen::rootWindow(), x(), y(), 1, &_rgba );
		} else {
			Imlib::draw( _image, Screen::rootWindow(), x(), y() );
		}
	}
	Screen::sync();
}

//------------------------------- Panels --------------------------------------

//-----------------------------------------------------------------------------
void Panels::zap()
//-----------------------------------------------------------------------------
{
	resetName();
	resetPassword();
}

//-----------------------------------------------------------------------------
void Panels::release()
//-----------------------------------------------------------------------------
{
	Panels::const_iterator it = begin();

	for (; it != end(); it++)
		delete (*it);

	Panels::clear();
}

//-----------------------------------------------------------------------------
void Panels::release(const std::string& name)
//-----------------------------------------------------------------------------
{
	Panel *panel = get( name, true );
	if ( panel ) { delete panel; }
}

#if 0
//-----------------------------------------------------------------------------
void Panels::clear()
//-----------------------------------------------------------------------------
{
	std::set<Panel*,ComparePanel>::clear();
}

//-----------------------------------------------------------------------------
void Panels::clear(const std::string& name)
//-----------------------------------------------------------------------------
{
	Panel *panel = get( name );
	if ( panel ) { panel->clear(); }
}
#endif

//-----------------------------------------------------------------------------
void Panels::focus(const std::string& name)
//-----------------------------------------------------------------------------
{
	Panel *panel = get( name );

	if ( !panel && name != "InputPanel" )
		panel = get( "InputPanel" );

	if ( panel ) {
		Screen::ungrabKeyboard();
		Screen::grabKeyboard( panel->window() );
	}
	Screen::sync();
}

//-----------------------------------------------------------------------------
void Panels::show()
//-----------------------------------------------------------------------------
{
	Panels::const_iterator it = begin();

	for (; it != end(); it++) {
		Panel *panel = (*it);
//std::cout << "Panels::show KEY='" << panel->getName() << "'\n";
		if ( !panel->onDemand() ) {
			panel->show();
		}
	}
	Panels::showSession();
}

//-----------------------------------------------------------------------------
Panel* Panels::get(int x, int y)
//-----------------------------------------------------------------------------
{
	Panels::const_iterator it = begin();

	for (; it != end(); it++) {
		Panel *panel = (*it);
		int px = panel->x();
		int py = panel->y();

		if ((x >= px && x < px+panel->width()) &&
			(y >= py && y < py+panel->height()))
		{
			return panel;
		}
	}
	return 0L;
}

//-----------------------------------------------------------------------------
Panel* Panels::get(Window wid)
//-----------------------------------------------------------------------------
{
	Panels::const_iterator it = begin();

	for (; it != end(); it++) {
		Panel *panel = (*it);
		if (wid == panel->window())
			return panel;
	}
	return 0L;
}

//-----------------------------------------------------------------------------
Panel* Panels::get(const std::string& name, bool dotake)
//-----------------------------------------------------------------------------
{
	Panels::iterator it = begin();

	for (; it != end(); it++) {
		Panel *panel = (*it);
		if (name == panel->getName()) {
			if ( dotake ) {
				Panels::erase( it );
			}
			return panel;
		}
	}
	return 0L;
}

//-----------------------------------------------------------------------------
bool Panels::scanDesktops(const std::string& folder)
//-----------------------------------------------------------------------------
{
	std::string theme( _themecfg->get( "Session/default" ));
	std::string themes( _themecfg->get( "Session/available" ));
	std::vector<std::string> sessions;
	DIR *dirp = ::opendir( folder.c_str() );
	struct dirent *entry = 0L;
	struct stat statbuf;
	uint i = 0;

	_sessions.clear();
	_sessions.push_back( theme );

	//std::cout << "scanDesktops '" << theme << "'\n";
	//std::cout << "scanDesktops '" << themes << "'\n";

	if ( !dirp ) { return false; }

	_themecfg->split( sessions, themes, ',' );

	while ((entry = readdir( dirp ))) {

		if (std::strcmp( entry->d_name, "." ) == 0 ||
			std::strcmp( entry->d_name, ".." ) == 0)
		{
			continue;
		}

		if (std::strstr( entry->d_name, ".desktop~" ))
			continue;

		std::string name( entry->d_name );
		std::string::size_type pos = name.find( ".desktop" );

		if (pos == std::string::npos)
			continue;

		name.erase( pos, 8 );

		for (i = 0; i < sessions.size(); i++) {
			if (name == sessions.at(i)) {
				break;
			}
		}

		if (i < sessions.size()) {
			std::string xinit( "/etc/X11/xinit/xinitrc." + name );

			if (::lstat( xinit.c_str(), &statbuf ))
				continue;

			if (S_ISREG( statbuf.st_mode ) &&
				::access( xinit.c_str(), R_OK|X_OK ) == 0)
			{
				if (theme != name) {
					_sessions.push_back( name );
				}
			}
		}
	}
	::closedir( dirp );
	return true;
}

//-----------------------------------------------------------------------------
bool Panels::load(Config *config)
//-----------------------------------------------------------------------------
{
	const std::list<std::string>& keys( config->getKeys() );
	std::string themedir( config->get( "themedir" ));
	std::list<std::string>::const_iterator it;

	_themecfg = config;

	if (! scanDesktops( "/usr/share/xsessions/" ))
		return false;

	for (it = keys.begin(); it != keys.end(); it++) {

		std::string key( (*it) );
		std::string paneltype( _themecfg->get( key + "/type" ));

		paneltype = Utils::strlower( paneltype );

//std::cout << "Panels key='" << key << "' key='" << paneltype << "'\n";
//std::cout.flush();

		if (paneltype == "panel" || paneltype == "button") {

			Config settings;
			Panel *panel = 0L;

			if (_themecfg->get( key, settings, true )) {

				if (!settings.getBool( "enabled", true ))
					continue;

				if (paneltype == "panel") {
					panel = new Panel( Panel::Pane, key );
				} else {
					panel = new Panel( Panel::Button, key );
				}

				std::string font( settings.get( "font-name", "Arial" ));
				std::string fontsize( settings.get( "font-size", "12" ));
				std::string color( settings.get( "font-color", "#000000" ));
				std::string bgcolor( settings.get( "bgcolor", "#00EEEEEE" ));
				std::string opacity( settings.get( "opacity" ));
				std::string x( settings.get( "coord-x", "0" ));
				std::string y( settings.get( "coord-y", "0" ));

				std::string text( settings.get( "text" ));
				std::string position( settings.get( "position" ));
				std::string direction( settings.get( "direction" ));
				std::string drawmode( settings.get( "mode" ));

				bool windowed = settings.getBool( "windowed", false );
				bool shadow = false; //settings.getBool( "shadow", false );
				bool ondemand = settings.getBool( "ondemand", false );

				panel->setColor( bgcolor );
				panel->setFontColor( color );
				panel->setFont( font + "/" + fontsize );
				panel->setOnDemand( ondemand );
				panel->setDrawMode( drawmode );
				panel->setDirection( direction );

				panel->setTextX( settings.getInt( "text-x", -1 ));
				panel->setTextY( settings.getInt( "text-y", -1 ));
				panel->setAlignment( settings.get( "alignment" ));

				panel->setWidth( settings.getInt( "width", -1 ));
				panel->setHeight( settings.getInt( "height", -1 ));
				panel->setZIndex( settings.getInt( "z-index", 0 ));

				if ( !opacity.empty() ) {
					RGBA rgba;
					rgba.r = 0; rgba.g = 0; rgba.b = 0;
					rgba.a = Config::percent2int( opacity, 255 );
					panel->setColor( rgba );
				}

				std::string parent( settings.get( "parent" ));

				if ( parent.empty() ) {
					panel->setX( Config::percent2int( x, Screen::width() ));
					panel->setY( Config::percent2int( y, Screen::height() ));
				} else {
					Panel *ppanel = Panels::get( parent );
					panel->setZIndex( 1 );
					panel->setParent( ppanel );
					panel->setX( Config::percent2int( x, ppanel->width() ));
					panel->setY( Config::percent2int( y, ppanel->height() ));
				}

				if (panel->isa( Panel::Button )) {
					std::string image( themedir + settings.get( "button" ));
					panel->setWidth( 32 ); panel->setHeight( 32 );
					panel->setButtons( image, shadow );
				}
				else {
					std::string background( settings.get( "background" ));

					if (key == "InputPanel" ||
						key == "UsernamePanel" ||
						key == "PasswordPanel")
					{
						//std::string left( settings.get( "message-position" ));
						panel->setInput( position, shadow );
					} else if (key == "Background") {
						panel->setBackground( background, themedir, shadow );
						panel->setOnDemand( true );
					} else if (!text.empty()) {
						panel->setText( text, shadow, windowed );
					} else if ( !background.empty() ) {
						panel->setBackground( background, themedir, shadow );
					} else {
						panel->setText( text, shadow, false );
					}
				}
				Panels::insert( panel );
			}
		}
	}
	//std::cout << "Panels#" << size() << "\n";
	//std::cout.flush();
	return size() > 0;
}

//-----------------------------------------------------------------------------
void Panels::showEnterMesg()
//-----------------------------------------------------------------------------
{
	Panel *panel = get( "InputMesg" );

	if ( panel ) {
		if (Screen::field() == Screen::Get_Name) {
			panel->showText( _themecfg->get( "Message/username" ));
		} else {
			panel->showText( _themecfg->get( "Message/password" ));
		}
	}
}

//-----------------------------------------------------------------------------
void Panels::showClock()
//-----------------------------------------------------------------------------
{
	Panel *panel = get( "ClockPanel" );

	if ( panel ) {
		std::string format( _themecfg->get( "ClockPanel/format" ));
		int secs = std::time(0) % 60;

		if ( format.empty() ) {
			format = "%A, %d %B, %Y %H:%M";
		}

		panel->showText( Utils::dateString( format ));

		::signal( SIGALRM, Screen::tick );
		::alarm( secs );
//std::cout << "Panel::showClock(" << secs << ")\n";
//std::cout.flush();
	}
}

//-----------------------------------------------------------------------------
void Panels::resetSession() { _curidx = 0; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void Panels::prevSession()
//-----------------------------------------------------------------------------
{
	_curidx = _curidx > 0 ? (_curidx-1) : _sessions.size()-1;
	showSession();
}

//-----------------------------------------------------------------------------
void Panels::nextSession()
//-----------------------------------------------------------------------------
{
	_curidx = ++_curidx >= _sessions.size() ? 0 : _curidx;
	showSession();
}

//-----------------------------------------------------------------------------
const std::string& Panels::getSession() const
//-----------------------------------------------------------------------------
{
	return _sessions.at( _curidx );
}

//-----------------------------------------------------------------------------
void Panels::showSession()
//-----------------------------------------------------------------------------
{
	Panel *panel = get( "SessionPanel" );

	if ( panel ) {
		std::string text( _themecfg->get( "SessionPanel/text" ));
		text = Utils::strrepl( text, "%session", getSession() );
		panel->showText( text );
	}
}

//-----------------------------------------------------------------------------
void Panels::showMessage(const std::string& text)
//-----------------------------------------------------------------------------
{
	Panel *panel = get( "MessagePanel" );
	if ( panel ) { panel->showText( text ); }
}

//-----------------------------------------------------------------------------
Panel::Action Panels::getAction() const { return _action; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
const std::string& Panels::getName() const { return _namebuf; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
const std::string& Panels::getPassword() const { return _password; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void Panels::reset()
//-----------------------------------------------------------------------------
{
	resetName();
	resetPassword();
}

//-----------------------------------------------------------------------------
void Panels::resetName() { _namebuf.clear(); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void Panels::resetPassword()
//-----------------------------------------------------------------------------
{
	Utils::strzap( _password );
	_password.clear();
	_hidden_passwd.clear();
}

//-----------------------------------------------------------------------------
void Panels::setName(const std::string& name)
//-----------------------------------------------------------------------------
{
	_namebuf = name;
	_action = Panel::Login;
}

//-----------------------------------------------------------------------------
void Panels::setPassword(const std::string& pw)
//-----------------------------------------------------------------------------
{
	_password = pw;
	_hidden_passwd = std::string( '*', pw.length() );
}

};

//=================================== EOF =====================================
