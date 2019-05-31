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

#include <stdio.h>
#include <unistd.h>
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
Panel::~Panel() { destroy(); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
Panel::Panel(Panel::Type t)
//-----------------------------------------------------------------------------
{
	_type = t;
	_image = 0L;
	_x11win = 0L;
	_parent = 0L;
	_x = _y = 0;
	_w = _h = 0;
	_dirty = false;
	_rtl = IMLIB_TEXT_TO_RIGHT;
	Imlib::setRGBA( "#00FFFFFF", &_rgba );
}

//-----------------------------------------------------------------------------
Panel::Panel(Panel::Type t, const std::string& n)
//-----------------------------------------------------------------------------
{
	_type = t;
	_name = n;
	_image = 0L;
	_x11win = 0L;
	_parent = 0L;
	_x = _y = 0;
	_w = _h = 0;
	_dirty = false;
	_rtl = IMLIB_TEXT_TO_RIGHT;
	Imlib::setRGBA( "#00FFFFFF", &_rgba );
}

//-----------------------------------------------------------------------------
void Panel::newButton(const std::string& button, bool shadow)
//-----------------------------------------------------------------------------
{
	Panel::clear();

	if ( _image ) {
		_image = Imlib::set( _image, button );
	} else {
		_shadow = shadow;
		_image = Imlib::load( button, false );
	}

	if ( _image ) {
		_w = Imlib::width( _image );
		_h = Imlib::height( _image );
		assert( _w < 256 && _h < 256 );
	}
}

//-----------------------------------------------------------------------------
void Panel::newInput(const std::string& position, bool shadow, bool windowed)
//-----------------------------------------------------------------------------
{
	_shadow = shadow;
	_image = Imlib::rectangle( 0L, _rgba, _w, _h, shadow );

	setDirty( true );
	setPosition( position, _parent );
}

//-----------------------------------------------------------------------------
void Panel::newText(const std::string& text, bool shadow, bool windowed)
//-----------------------------------------------------------------------------
{
	_shadow = shadow;

	_w = _w > 0 ? _w : Screen::width() / 2;
	_x = _x > 0 ? _x : (Screen::width() - _w) / 2;

	_image = Imlib::rectangle( 0L, _rgba, _w, _h, shadow );

	setValue( text );
}

//-----------------------------------------------------------------------------
void Panel::newBackground(const std::string& bg, const std::string& themedir, bool shadow, bool forcebg)
//-----------------------------------------------------------------------------
{
	std::string::size_type offset = std::string::npos;
	std::string spec( bg );
	std::string gradtype;
	RGBA rgba, ergba;

	_spec = spec;
	_shadow = shadow;

	if (spec[0] != '/') {
		if (spec.find( "gradient" ) != std::string::npos)
			spec.erase( 0, 8 );

		Utils::strtrim( spec );
		offset = spec.find( '#' );
	}

	if (offset != std::string::npos) {

		_w = _w > 0 ? _w : Screen::width();
		_h = _h > 0 ? _h : Screen::height();

		if (offset == 0) {
			Imlib::setRGBA( spec, &rgba );
			_image = Imlib::rectangle( 0L, rgba, _w, _h, 1 );

		} else {
			gradtype = spec.substr( 0, offset );
			Utils::strtrim( gradtype );

			spec.erase( 0, offset );
			Utils::strtrim( spec );

			if ((offset = spec.find( ' ' )) !=  std::string::npos) {

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

		_image = forcebg ? Imlib::load( spec ) : 0L;
		_image = _image ? _image : Imlib::loadSized( themedir, "png" );
		_image = _image ? _image : Imlib::loadSized( themedir, "jpg" );
		_image = _image ? _image : Imlib::load( themedir+spec );

	} else {
		if (spec[0] == '/') {
			_image = Imlib::load( spec );
		} else {
			_image = Imlib::load( themedir+spec );
		}

		_w = Imlib::width( _image );
		_h = Imlib::height( _image );
	}

	setType( Panel::Background );

	if (_name == "Background" ||
		(_w == Screen::width() && _h == Screen::height()))
	{
		_x11win = Screen::rootWindow();
	}
}

//-----------------------------------------------------------------------------
void Panel::setStatic(bool flag)
//-----------------------------------------------------------------------------
{
	uint mask = _type;

	if ( flag ) {
		mask |= Panel::Static;
	} else {
		mask &= ~Panel::Static;
	}

	_type = static_cast<Panel::Type>( mask );
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
	} else if (_x < 1) {
		_align = Imlib::CENTER;
	} else {
		_align = Imlib::LEFT;
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
		} else if (position == "top-right") {
			_x = parent->width() - _w;
			_y = parent->y();
		} else if (position == "top-left") {
			_x = parent->x();
			_y = parent->y();
/*
		} else if (parent->window()) {
			_x += parent->x();
			_y += parent->y();
		} else {
			_x += parent->x();
			_y += parent->y();
*/
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
		if (_x11win && _x11win != Screen::rootWindow()) {
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
		//Screen::sync();
	}
	else { // Wayland
	}
}

//-----------------------------------------------------------------------------
Window Panel::parentWindow() const
//-----------------------------------------------------------------------------
{
	return parent() ? parent()->window() : 0L;
}

//-----------------------------------------------------------------------------
Imlib_Image Panel::parentImage() const
//-----------------------------------------------------------------------------
{
	return parent() ? parent()->image() : 0L;
}

//-----------------------------------------------------------------------------
int Panel::compare(const Panel& elem) const
//-----------------------------------------------------------------------------
{
	int result = zIndex() - elem.zIndex();
	if (result == 0)
		return _name.compare( elem._name );
	return result;
}

//-----------------------------------------------------------------------------
void Panel::setValue(const std::string& text)
//-----------------------------------------------------------------------------
{
	Panel *panel = this;

	_text = text;

	while ( panel ) {
		panel->setDirty( true );
		panel = panel->parent();
	}
#if 0
#endif
}

//-----------------------------------------------------------------------------
void Panel::setText(const std::string& text, bool shadow)
//-----------------------------------------------------------------------------
{
	int tw = 0; int th = 0;

	Imlib::drawText( _image, text, _font, _font_rgba, tw, th, _align, _rtl );

	if (_x < 1) {
		_tx = (Screen::width() - tw) / 2;
	} else {
		_tx = _x;
	}
}

//-----------------------------------------------------------------------------
void Panel::showText(const std::string& text)
//-----------------------------------------------------------------------------
{
	setValue( text );
	Panel::show();
}

//-----------------------------------------------------------------------------
void Panel::show()
//-----------------------------------------------------------------------------
{
	Window wid = Screen::rootWindow();

	if ( dirty() ) {

		if (isa( Panel::Input )) {
			Imlib::rectangle( _image, _rgba, _w, _h, 0 );
		} else {
			Panel::clear();
		}

		if ( !_text.empty() ) {
			setText( _text );
		}

		Imlib::draw( _image, wid, _x, _y, 1 );
	}

//std::cerr << "Panel::show '" << getName() << "' DIRTY=" << (dirty() ? "true" : "false") << " X=" << _x << " TX=" << _tx << " Y=" << _y << " W=" << width() << " H=" << height() << " IMG=[ " << image() << "," << img << " ] '" << _text << "' !\n";
//std::cerr.flush();

	setDirty( false );
}

//------------------------------- Panels --------------------------------------

//-----------------------------------------------------------------------------
void Panels::zap()
//-----------------------------------------------------------------------------
{
	resetUsername();
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
Imlib_Image Panels::getBackground() const
//-----------------------------------------------------------------------------
{
	Panel *bpanel = Panels::get( "Background" );
	Imlib_Image image = Imlib::create( bpanel->width(), bpanel->height() );

	Imlib::blend( image, bpanel->image(), 0, 0 );

	for (auto& panel: (*this)) {
		if (panel != bpanel &&
			(panel->isa( Panel::Static ) ||
				panel->isa( Panel::Button ) || panel->isa( Panel::Background )))
		{
			if (panel->isa( Panel::Static ) && panel->isa( Panel::TextPane ))
				panel->show();

			Imlib::blend( image, panel->image(), panel->x(), panel->y() );
		}
	}

	Imlib::draw( image, Screen::rootWindow() );
	return image;
}

//-----------------------------------------------------------------------------
Panel* Panels::get(int x, int y) const
//-----------------------------------------------------------------------------
{
	for (const auto& panel: (*this)) {

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
Panel* Panels::get(Window wid) const
//-----------------------------------------------------------------------------
{
	for (const auto& panel: (*this)) {
		if (wid == panel->window())
			return panel;
	}

	return 0L;
}

//-----------------------------------------------------------------------------
Panel* Panels::get(const std::string& name) const
//-----------------------------------------------------------------------------
{
//	Panels::iterator it = begin();

	for (const auto& panel: (*this)) {
//for (; it != end(); it++) {
//	Panel *panel = (*it);
		if (name == panel->getName()) {
//		if ( dotake ) {
//			Panels::erase( it );
//		}
			return panel;
		}
	}

	return 0L;
}

//-----------------------------------------------------------------------------
void Panels::showUsername(const std::string& text)
//-----------------------------------------------------------------------------
{
	Panel *panel = get( "InputPanel" );

	panel = panel ? panel : get( "UsernamePanel" );

	if ( panel ) {
		panel->setValue( text );
		panel->show();
	}

	setUsername( text );
}

//-----------------------------------------------------------------------------
void Panels::showPassword(const std::string& text)
//-----------------------------------------------------------------------------
{
	Panel *panel = get( "InputPanel" );

	panel = panel ? panel : get( "PasswordPanel" );

	if ( panel ) {
		panel->setValue( text );
		panel->show();
	}

/*
	if ( text.empty() ) {
		Panels::resetPassword();
	}
*/
}

//-----------------------------------------------------------------------------
bool Panels::scanDesktops(const std::string& folder, const std::vector<std::string>& xpaths)
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

			std::string lname( Utils::strlower( name ));

			for (const auto& folder: xpaths) {

				std::string xinit( folder );
				std::string xinit2( folder );

				if (std::strncmp( folder.c_str(), "/etc/X11/xinit", 14 )) {
					xinit += "/" + name;
					xinit2 += "/" + lname;
				} else {
					xinit += "/xinitrc." + name;
					xinit2 += "/xinitrc." + lname;
				}

				if (::lstat( xinit.c_str(), &statbuf )) {
					if (!::lstat( xinit2.c_str(), &statbuf ))
						xinit = xinit2;
				}

				if (S_ISREG( statbuf.st_mode )) {
					std::cerr << "Found Theme '" << name << " !\n";
					if (::access( xinit.c_str(), R_OK ) == 0) {
						::chmod( xinit.c_str(), 0755 );

						if (theme != name && theme != lname) {
							_sessions.push_back( name );
						}
					}
				}
			}
		}
	}

	::closedir( dirp );
	return true;
}

//-----------------------------------------------------------------------------
bool Panels::load(Config *config, const std::string& bgfile)
//-----------------------------------------------------------------------------
{
	const std::list<std::string>& keys( config->getKeys() );
	std::string themedir( config->get( "themedir" ));
	std::list<std::string>::const_iterator it;
	std::vector<std::string> xpaths;
	Config defaults;

	_themecfg = config;
	_themecfg->get( "Defaults/", defaults, true );
	_themecfg->splitKey( xpaths, "xpaths", ':' );

	if (! scanDesktops( "/usr/share/xsessions/", xpaths ))
		return false;

	for (const auto& key: keys) {

		std::string paneltype( _themecfg->get( key + "/type" ));

		paneltype = Utils::strlower( paneltype );

//std::cout << "Panels KEY='" << key << "' TYPE='" << paneltype << "'\n";
//std::cout.flush();

		if (key == "Snapshot" && !_themecfg->getBool( "Allow/snapshot" ))
			continue;

		if (paneltype == "panel" || paneltype == "button") {

			Config settings;
			Panel *ppanel = 0L;
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
				std::string w( settings.get( "width", "-1" ));
				std::string h( settings.get( "height", "-1" ));
				int cx = settings.getInt( "coord-x", 0 );
				int cy = settings.getInt( "coord-y", 0 );

				std::string text( settings.get( "text" ));
				std::string position( settings.get( "position" ));
				std::string direction( settings.get( "direction" ));
				std::string drawmode( settings.get( "mode" ));

				bool windowed = settings.getBool( "windowed", false );
				bool shadow = false; //settings.getBool( "shadow", false );
				bool ondemand = settings.getBool( "ondemand", false );
				bool isstatic = settings.getBool( "static", false );

				panel->setFontColor( color );
				panel->setFont( font + "/" + fontsize );

				panel->setOnDemand( ondemand );
				panel->setDrawMode( drawmode );
				panel->setDirection( direction );

				panel->setTextX( settings.getInt( "text-x", cx ));
				panel->setTextY( settings.getInt( "text-y", cy ));
				panel->setAlignment( settings.get( "alignment" ));

				if (w == "-1") {
					panel->setWidth( -1 );
				} else if (w.find( "%" ) != std::string::npos) {
					panel->setWidth( Config::percent2int( w, Screen::width() ));
				} else {
					panel->setWidth( settings.string2int( w ));
				}

				if (h == "-1") {
					panel->setHeight( -1 );
				} else if (h.find( "%" ) != std::string::npos) {
					panel->setHeight( Config::percent2int( h, Screen::height() ));
				} else {
					panel->setHeight( settings.string2int( h ));
				}

				panel->setZIndex( settings.getInt( "z-index", 0 ));

				if ( !bgcolor.empty() ) {
					panel->setColor( bgcolor );

				} else if ( !opacity.empty() ) {
					RGBA rgba;
					rgba.r = 0; rgba.g = 0; rgba.b = 0;
					rgba.a = Config::percent2int( opacity, 255 );
					panel->setColor( rgba );
				}

				std::string parent( settings.get( "parent" ));

				if (parent.empty() || parent == "Background") {
					panel->setX( Config::percent2int( x, Screen::width() ));
					panel->setY( Config::percent2int( y, Screen::height() ));

				} else if ((ppanel = Panels::get( parent ))) {

					panel->setZIndex( 1 );
					panel->setParent( ppanel );
					panel->setX( Config::percent2int( x, ppanel->width() ));
					panel->setY( Config::percent2int( y, ppanel->height() ));

					/*
					std::cerr << "Panel '" << panel->getName() << " !\n";
					std::cerr << "Parent panel W='" << ppanel->width() << " !\n";
					std::cerr << "Parent panel H='" << ppanel->height() << " !\n";
					std::cerr << "Panel PX='" << ppanel->x() << " !\n";
					std::cerr << "Panel PY='" << ppanel->y() << " !\n";
					std::cerr << "Panel X='" << panel->x() << " !\n";
					std::cerr << "Panel Y='" << panel->y() << " !\n";
					std::cerr << "Panel W='" << panel->width() << " !\n";
					std::cerr << "----------------------------------------\n";
					std::cerr.flush();
					*/
				} else {
					std::cerr << "No such parent panel '" << parent << " !\n";
					std::cerr.flush();
				}

				if (panel->isa( Panel::Button )) {
					std::string image( themedir + settings.get( "button" ));
					int dh = defaults.getBool( "button-height", -1 );
					int dw = defaults.getBool( "button-width", -1 );

					panel->setWidth( settings.getInt( "width", dw ));
					panel->setHeight( settings.getInt( "height", dh ));
					panel->newButton( image, shadow );

				} else {
					std::string background( settings.get( "background" ));

					if (key == "InputPanel") {
						panel->setType( Panel::InputPane );
						panel->newInput( position, shadow );

					} else if (key == "UsernamePanel") {
						panel->setType( Panel::InputPane );
						panel->newInput( position, shadow );

					} else if (key == "PasswordPanel") {
						panel->setType( Panel::PasswdPane );
						panel->newInput( position, shadow );

					} else if (key == "Photo") {
						panel->setType( Panel::PhotoPane );
						panel->newButton( background, shadow );

					} else if (key == "Background") {
						background = bgfile.empty() ? background : bgfile;
						panel->newBackground( background, themedir, shadow, !bgfile.empty() );

					} else if (!text.empty()) {
						panel->setType( Panel::TextPane );
						panel->newText( text, shadow, windowed );

					} else if ( !background.empty() ) {
						panel->newBackground( background, themedir, shadow );

					} else {
						panel->setType( Panel::TextPane );
						panel->newText( text, shadow, false );
					}

					if (ppanel && ppanel != Panels::get( "Background" )) {
						panel->setX( ppanel->x() + panel->x() );
						panel->setY( ppanel->y() + panel->y() );
					/*
					std::cerr << "Panel '" << panel->getName() << " !\n";
					std::cerr << "Panel X='" << panel->x() << " !\n";
					std::cerr << "Panel Y='" << panel->y() << " !\n";
					*/
					}
				}

				panel->setStatic( isstatic );

				Panels::insert( panel );
			}
		}
	}

	Panel *parent = Panels::get( "Background" );

	for (auto& panel: (*this)) {
		if ( !panel->parent() ) {
			if (panel->getName() != "Background") {
				panel->setParent( parent );
			}
		}
	}

	return size() > 0;
}

//-----------------------------------------------------------------------------
void Panels::showEnterMesg()
//-----------------------------------------------------------------------------
{
	Panel *panel = get( "InputPanel" );

	if ( panel ) {
		panel->show();

	} else if ((panel = get( "UsernamePanel" ))) {
		panel->show();

		if ((panel = get( "PasswordPanel" ))) {
			panel->show();
		}
	}

	if ((panel = get( "InputMesg" ))) {
		if (Screen::field() == Screen::Get_Name) {
			panel->setValue( _themecfg->get( "Message/username" ));
		} else {
			panel->setValue( _themecfg->get( "Message/password" ));
		}

		panel->show();
#if 0
	} else if ((panel = get( "UsernameMesg" ))) {
		panel->show();

		if ((panel = get( "PasswordMesg" ))) {
			panel->show();
		}
#endif
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
			format = "%A, %d %B, %Y %H:%M:%S";
		}

		panel->showText( Utils::dateString( format ));

		::signal( SIGALRM, Screen::tick );
		::alarm( secs );
	}
}

//-----------------------------------------------------------------------------
void Panels::resetSession() { _curidx = 0; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void Panels::prevSession() { Panels::setSession( -1 ); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void Panels::nextSession() { Panels::setSession( +1 ); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
const std::string& Panels::getSession() const { return _sessions.at( _curidx ); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void Panels::setSession(int step)
//-----------------------------------------------------------------------------
{
	Panel *panel = get( "SessionPanel" );
	int nb_sessions = _sessions.size();

	_curidx += step;

	if (_curidx >= nb_sessions) {
		_curidx = 0;
	} else if (_curidx < 0) {
		_curidx = nb_sessions-1;
	}

	if ( panel ) {
		std::string text( _themecfg->get( "SessionPanel/text" ));
		text = Utils::strrepl( text, "%session", getSession() );
		panel->showText( text );
	}
}

//-----------------------------------------------------------------------------
void Panels::prevUser() { Panels::setUser( -1 ); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void Panels::nextUser() { Panels::setUser( +1 ); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
const std::string& Panels::getUser() const { return _users.at( _curuser ); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void Panels::setUser(int step)
//-----------------------------------------------------------------------------
{
	Panel *panel = get( "PhotoPanel" );
	int nb_users = _users.size();

	_curuser += step;

	if (_curuser >= nb_users) {
		_curuser = 0;
	} else if (_curuser < 0) {
		_curuser = nb_users-1;
	}

	if ( panel ) {
		std::string user( _users.at( _curuser ));
		Config entries;

		if (_themecfg->get( user, entries, true )) {
			std::string photo( entries.get( "photo" ));
			panel->newButton( photo, true );
			panel->show();
		}
	}
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
	resetUsername();
	resetPassword();
}

//-----------------------------------------------------------------------------
void Panels::resetUsername()
//-----------------------------------------------------------------------------
{
	_namebuf.clear();
}

//-----------------------------------------------------------------------------
void Panels::resetPassword()
//-----------------------------------------------------------------------------
{
	Utils::strzap( _password );
	Utils::strzap( _hidden_passwd );
}

//-----------------------------------------------------------------------------
void Panels::setUsername(const std::string& name)
//-----------------------------------------------------------------------------
{
	_namebuf = name;
	_action = Panel::Login;
}

//-----------------------------------------------------------------------------
void Panels::setPassword(const std::string& text)
//-----------------------------------------------------------------------------
{
	_password = text;
	_hidden_passwd = std::string( '*', text.length() );
}

};

//=================================== EOF =====================================
