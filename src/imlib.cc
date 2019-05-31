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
#include <sstream>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "screen.h"
#include "config.h"
#include "imlib.h"

namespace breeze {

//-----------------------------------------------------------------------------
void Imlib::init(::Display *dpy, Config *themecfg)
//-----------------------------------------------------------------------------
{
	std::string fontlist( themecfg->get( "Fonts/paths" ));
	std::vector<std::string> fonts;
	struct stat pathbuf;
	Config entries;

	::imlib_context_set_display( dpy );
	::imlib_set_color_usage(256);
	::imlib_context_set_visual( Screen::visual() );
	::imlib_context_set_colormap( Screen::colormap() );

	// dither for depths < 24bpp
	::imlib_context_set_dither(1);
	::imlib_context_set_anti_alias( 1 );

	if (themecfg->getBool( "Session/hud" )) {
		::imlib_set_cache_size( 2048 * 1024 );
		::imlib_set_font_cache_size( 1024 * 1024 );
	}

	if (fontlist.empty()) {
		if (themecfg->get( "Fonts/", entries, true )) {
			for (const auto& tuple: entries) {
				fonts.push_back( tuple.first );
			}
		}
	} else {
		themecfg->split( fonts, fontlist, ':' );
	}

	::imlib_add_path_to_font_path( "/usr/share/imlib2/data/fonts" );

	for (const auto& path: fonts) {
		if (::lstat( path.c_str(), &pathbuf ) == 0) {
			::imlib_add_path_to_font_path( path.c_str() );
		} else {
			syslog( LOGFLAGS, "No such font path '%s'", path.c_str() );
		}
	}
}

//-----------------------------------------------------------------------------
void Imlib::release()
//-----------------------------------------------------------------------------
{
	::imlib_context_disconnect_display();
}

//-----------------------------------------------------------------------------
void Imlib::release(Imlib_Image image)
//-----------------------------------------------------------------------------
{
	Imlib_Image img = ::imlib_context_get_image();

	::imlib_context_set_image( image );
	::imlib_free_image();

	if ( img ) {
		::imlib_context_set_image( img );
	}
}

//-----------------------------------------------------------------------------
void Imlib::clear(Imlib_Image image)
//-----------------------------------------------------------------------------
{
	::imlib_context_set_image( image );
	::imlib_image_clear();
}

//-----------------------------------------------------------------------------
Imlib_Image Imlib::clone(Imlib_Image image)
//-----------------------------------------------------------------------------
{
	::imlib_context_set_image( image );
	return ::imlib_clone_image();
}

//-----------------------------------------------------------------------------
void Imlib::save(Imlib_Image image, const std::string& path)
//-----------------------------------------------------------------------------
{
	::imlib_context_set_image( image );
	//::imlib_image_set_format( "PNG" );
	::imlib_save_image( path.c_str() );
}

//-----------------------------------------------------------------------------
int Imlib::width(Imlib_Image image)
//-----------------------------------------------------------------------------
{
	Imlib_Image img = ::imlib_context_get_image();
	::imlib_context_set_image( image );
	int w = ::imlib_image_get_width();
	if ( img ) { ::imlib_context_set_image( img ); }
	return w;
}

//-----------------------------------------------------------------------------
int Imlib::height(Imlib_Image image)
//-----------------------------------------------------------------------------
{
	Imlib_Image img = ::imlib_context_get_image();
	::imlib_context_set_image( image );
	int h = ::imlib_image_get_height();
	if ( img ) { ::imlib_context_set_image( img ); }
	return h;
}

//-----------------------------------------------------------------------------
void Imlib::drawText(Imlib_Image image, const std::string& text, const std::string& fontspec, const RGBA& rgba, int& tw, int& th, Imlib::Alignment align, Imlib_Text_Direction rtl)
//-----------------------------------------------------------------------------
{
	Imlib_Font font = ::imlib_load_font( fontspec.c_str() );
	std::string fontname( fontspec );

	if ( ! font ) {
		fontname = Utils::strlower( fontspec );
		font = ::imlib_load_font( fontname.c_str() );
	}

	if ( ! font ) {
		syslog( LOGFLAGS, "Font '%s' could not be loaded", fontspec.c_str() );
		std::string::size_type offset = fontname.find('/');
		fontname.replace( 0, offset, "notepad" );
		font = ::imlib_load_font( fontname.c_str() );
	}

	::imlib_context_set_image( image );
	::imlib_image_set_has_alpha(1);
	::imlib_context_set_blend(1);
	::imlib_context_set_direction( rtl );
	::imlib_context_set_color( rgba.r, rgba.g, rgba.b, rgba.a );
	::imlib_context_set_font( font );
	::imlib_get_text_size( text.c_str(), &tw, &th );

#if 0
	::imlib_text_draw( 0, 0, text.c_str() );
#endif

	int w = ::imlib_image_get_width();
	int h = ::imlib_image_get_height();

	if (align == Imlib::CENTER) {
		::imlib_text_draw( (w-tw)/2, (h-th)/2, text.c_str() );
	} else if (align == Imlib::RIGHT) {
		::imlib_text_draw( (w-tw)-5, (h-th)/2, text.c_str() );
	//} else if (align == Imlib::LEFT) {
	//	::imlib_text_draw( 5, (h-th)/2, text.c_str() );
	//} else if (x < 0 && y < 0) {
	//	::imlib_text_draw( (w-tw)/2, (h-th)/2, text.c_str() );
    } else {
		::imlib_text_draw( 1, 0, text.c_str() );
		//::imlib_text_draw( (w-tw)/2, (h-th)/2, text.c_str() );
		//::imlib_text_draw( x, y, text.c_str() );
	}

	::imlib_free_font();
}

//-----------------------------------------------------------------------------
void Imlib::draw(Imlib_Image image, Drawable win, int x, int y, int blend, RGBA *rgba)
//-----------------------------------------------------------------------------
{
	::imlib_context_set_image( image );
	::imlib_image_set_has_alpha( 1 );
	::imlib_context_set_blend( blend );
	::imlib_context_set_drawable( win );

	//std::cerr << "Imlib::draw X=" << x << " Y=" << y << "\n";
	//std::cerr.flush();

	if ( rgba ) {
		::imlib_context_set_color( rgba->r, rgba->g, rgba->b, rgba->a );
/*
		::fprintf( stdout,
			"Imlib::draw R=0x%x G=0x%x B=0x%x A=0x%x\n",
			rgba->r, rgba->g, rgba->b, rgba->a
		);
*/
	}

	if (x < 0 && y < 0) {
		::imlib_render_image_on_drawable( 0, 0 );
	} else if (x < 0) {
		::imlib_render_image_on_drawable( 0, y );
	} else if (y < 0) {
		::imlib_render_image_on_drawable( x, 0 );
	} else {
		::imlib_render_image_on_drawable( x, y );
	}
}

//-----------------------------------------------------------------------------
void Imlib::blend(Imlib_Image image, Imlib_Image win, int x, int y, int blend)
//-----------------------------------------------------------------------------
{
	int w = Imlib::width( win );
	int h = Imlib::height( win );

	::imlib_context_set_image( image );
	::imlib_image_set_has_alpha( 1 );
	::imlib_context_set_blend( blend );

	/*
	if ( rgba ) {
		::imlib_context_set_color( rgba->r, rgba->g, rgba->b, rgba->a );
	}
	*/
	::imlib_blend_image_onto_image( win, 1, 0, 0, w, h, x, y, w, h );
}

//-----------------------------------------------------------------------------
void Imlib::scale(Imlib_Image image, Drawable win, int w, int h, int x, int y, int blend)
//-----------------------------------------------------------------------------
{
	::imlib_context_set_image( image );
	::imlib_context_set_drawable( win );
	::imlib_image_set_has_alpha( 1 );
	::imlib_context_set_blend( blend );
	::imlib_render_image_on_drawable_at_size( x, y, w, h );
}

//-----------------------------------------------------------------------------
void Imlib::setRGBA(const std::string& color, RGBA *rgb)
//-----------------------------------------------------------------------------
{
	std::string::size_type offset = color.find('#');
	std::string rgba( color );
	unsigned long packed_rgb = 0;

	if (offset == 0)
		rgba = rgba.erase( 0, 1 );

	if (rgba.length() == 6)
		rgba.insert( 0, "FF" );

	const char *hex = rgba.c_str();
	::sscanf( hex, "%lx", &packed_rgb );

	rgb->a = packed_rgb & 0xff000000; rgb->a >>= 24;
	rgb->r = packed_rgb & 0xff0000; rgb->r >>= 16;
	rgb->g = packed_rgb & 0x00ff00; rgb->g >>= 8;
	rgb->b = packed_rgb & 0x0000ff;
	rgb->gradient = 0;

	/*
	::fprintf( stdout,
		"R=0x%x G=0x%x B=0x%x A=0x%x\n",
		rgb->r, rgb->g, rgb->b, rgb->a
	);
	*/
}

//-----------------------------------------------------------------------------
Imlib_Image Imlib::gradient(Imlib_Image image, const std::string& gradtype, const RGBA& srgba, const RGBA& ergba, int w, int h)
//-----------------------------------------------------------------------------
{
	Imlib_Color_Range range = imlib_create_color_range();

	image = image ? image : create( w, h );

	::imlib_context_set_image( image );
	::imlib_context_set_color_range( range );

	::imlib_context_set_color( srgba.r, srgba.g, srgba.b, srgba.a );
	::imlib_add_color_to_color_range(0);

	::imlib_context_set_color( ergba.r, ergba.g, ergba.b, ergba.a );
	::imlib_add_color_to_color_range(5);

//std::cerr << "Imlib::gradient IMG=" << image << " '" << gradtype << "'\n";
//std::cerr.flush();

	if (gradtype == "vertical") {
		::imlib_image_fill_color_range_rectangle( 0, 0, w, h, 0.0 );
	} else if (gradtype == "horizontal") {
		::imlib_image_fill_color_range_rectangle( 0, 0, w, h, -180.0 );
	} else {
		std::string mode( gradtype );

		Utils::strdel( mode, "!" );
		float degrees = Config::string2float( mode, 0L );
//std::cerr << "Imlib::gradient '" << gradtype << "' degrees=" << degrees << "\n";
		::imlib_image_fill_color_range_rectangle( 0, 0, w, h, degrees );
	}

	::imlib_free_color_range();
	return image;
}

//-----------------------------------------------------------------------------
Imlib_Image Imlib::rectangle(Imlib_Image image, const RGBA& rgba, int w, int h, bool shadow)
//-----------------------------------------------------------------------------
{
	image = image ? image : ::imlib_create_image( w, h );

/*
std::cerr << "Imlib::rectangle IMG=" << image << " W=" << w << " H=" << h << "\n";

::fprintf( stdout,
	"Imlib::rectangle R=0x%x G=0x%x B=0x%x A=0x%x\n",
	rgba.r, rgba.g, rgba.b, rgba.a
);
*/

	::imlib_context_set_image( image );
	::imlib_image_set_has_alpha(1);
	::imlib_context_set_blend(1);
	::imlib_context_set_color( rgba.r, rgba.g, rgba.b, rgba.a );
	::imlib_image_fill_rectangle( 0, 0, w, h );

	return image;
}

//-----------------------------------------------------------------------------
Imlib_Image Imlib::create(int w, int h, bool shadow)
//-----------------------------------------------------------------------------
{
	Imlib_Image image = ::imlib_create_image( w, h );
	::imlib_context_set_image( image );
	::imlib_image_set_has_alpha(1);
	::imlib_context_set_blend(1);
	return image;
}

//-----------------------------------------------------------------------------
Imlib_Image Imlib::set(Imlib_Image image, const std::string& imgstr)
//-----------------------------------------------------------------------------
{
	struct stat buf;

	if (!::lstat( imgstr.c_str(), &buf )) {
		::imlib_context_set_image( image );
		return ::imlib_load_image_immediately_without_cache( imgstr.c_str() );
	}

	return image;
}

//-----------------------------------------------------------------------------
Imlib_Image Imlib::create(const std::string& imgstr, int w, int h, bool shadow, bool with_cache)
//-----------------------------------------------------------------------------
{
	struct stat buf;

	if (::stat( imgstr.c_str(), &buf ))
		return 0L;

	if ( with_cache )
		return ::imlib_load_image( imgstr.c_str() );

	return ::imlib_load_image_immediately_without_cache( imgstr.c_str() );
}

//-----------------------------------------------------------------------------
Imlib_Image Imlib::load(const std::string& path, bool with_cache)
//-----------------------------------------------------------------------------
{
	struct stat buf;
	//std::cerr << path << "\n";

	if (::lstat( path.c_str(), &buf ))
		return 0L;

	if ( with_cache )
		return ::imlib_load_image( path.c_str() );

	return ::imlib_load_image_immediately_without_cache( path.c_str() );
}

//-----------------------------------------------------------------------------
Imlib_Image Imlib::loadSized(const std::string& folder, const std::string& format)
//-----------------------------------------------------------------------------
{
	std::string themedir( folder );
	char path[256]={ '\0' };
	struct stat buf;

	std::sprintf(
		path, "%s/%dx%d.%s",
		themedir.c_str(),
		Screen::width(), Screen::height(),
		format.c_str()
	);

	//std::cerr << "loadSized: '" << path << "'\n";

	if (::lstat( path, &buf ) == 0)
		return Imlib::load( path );

	return 0L;
}

};
