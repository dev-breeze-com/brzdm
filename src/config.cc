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
#include "utils.h"
#include "infile.h"
#include "config.h"

namespace breeze {

//-----------------------------------------------------------------------------
Config::~Config() { clear(); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
Config::Config()
//-----------------------------------------------------------------------------
{
	_mode = Config::Normal;
}

//-----------------------------------------------------------------------------
Config::Config(const std::string& path)
//-----------------------------------------------------------------------------
{
	_mode = Config::Normal;
	Config::load( path );
}

//-----------------------------------------------------------------------------
void Config::set(const std::string& key, const std::string& value)
//-----------------------------------------------------------------------------
{
	operator[]( key ) = value;
}

//-----------------------------------------------------------------------------
void Config::set(const std::string& prefix, const Config& entries)
//-----------------------------------------------------------------------------
{
	for (const auto& tuple: entries) {
		std::string key( prefix + "/" + tuple.first );
		Utils::strrepl( key, "//", "/" );
		operator[]( key ) = tuple.second;
	}
}

//-----------------------------------------------------------------------------
bool Config::getBool(const std::string& key, bool flag) const
//-----------------------------------------------------------------------------
{
	std::string value( get( key ));
	return value.empty() ? flag : string2bool( value );
}

//-----------------------------------------------------------------------------
int Config::getInt(const std::string& key, int value) const
//-----------------------------------------------------------------------------
{
	bool err = 0;
	std::string val( get( key ));
	return val.empty() ? value : string2int( val, &err );
}

//-----------------------------------------------------------------------------
uint Config::getUInt(const std::string& key, uint value) const
//-----------------------------------------------------------------------------
{
	bool err = 0;
	std::string val( get( key ));
	return val.empty() ? value : string2int( val, &err );
}

//-----------------------------------------------------------------------------
uint Config::getHexa(const std::string& key, uint value) const
//-----------------------------------------------------------------------------
{
	bool err = 0;
	std::string val( get( key ));
	return val.empty() ? value : string2int( val, &err );
}

//-----------------------------------------------------------------------------
float Config::getFloat(const std::string& key, float value) const
//-----------------------------------------------------------------------------
{
	bool err = 0;
	std::string val( get( key ));
	return val.empty() ? value : string2float( val, &err );
}

//-----------------------------------------------------------------------------
void Config::zap()
//-----------------------------------------------------------------------------
{
	for (auto& tuple: (*this)) {
		Utils::strzap( tuple.second );
	}
}

//-----------------------------------------------------------------------------
void Config::clearValues()
//-----------------------------------------------------------------------------
{
	for (auto& tuple: (*this)) {
		tuple.second.clear();
	}
}

//-----------------------------------------------------------------------------
bool Config::get(const std::string& key, Config& entries, bool strip) const
//-----------------------------------------------------------------------------
{
	std::string prefix( key );
	std::string::size_type offset = prefix.rfind( '/' );

	if (offset != prefix.length()-1)
		prefix += "/";

	for (const auto& tuple: (*this)) {
		std::string key( tuple.first );

		if (key.find( prefix ) == 0) {
			std::string value( tuple.second );
			if ( strip ) { Utils::strdel( key, prefix ); }
			entries.insert( std::make_pair( key, value ));
//std::cout << "Config::get [ '" << key << "','" << value << "' ]\n";
		}
	}

	return entries.size() > 0;
}

//-----------------------------------------------------------------------------
const std::string& Config::get(const std::string& key, const std::string& value, bool useval) const
//-----------------------------------------------------------------------------
{
	const_iterator it;

	if ( useval ) {
		for (const auto& tuple: (*this)) {
			if (key == tuple.second)
				return tuple.first;
		}
		return value;
	}

	it = std::map<std::string, std::string>::find( key );
	return it != end() ? (*it).second : value;
}

//-----------------------------------------------------------------------------
bool Config::contains(const std::string& key) const
//-----------------------------------------------------------------------------
{
	return std::map<std::string, std::string>::find( key ) != end();
}

//-----------------------------------------------------------------------------
Config& Config::operator+=(const Config& entries)
//-----------------------------------------------------------------------------
{
	const std::list<std::string>& keys = entries.getKeys();

	if (keys.size() > 0) {
		_keys.insert( _keys.begin(), keys.begin(), keys.end() );
	}

	for (const auto& tuple: entries) {
		std::string key( tuple.first );
		std::string value( tuple.second );
		operator[]( key ) = value;
	}

	return (*this);
}

//-----------------------------------------------------------------------------
Config& Config::operator<<(const std::string& url)
//-----------------------------------------------------------------------------
{
	Config::load( url );
	return (*this);
}

//-----------------------------------------------------------------------------
bool Config::load(const std::string& url)
//-----------------------------------------------------------------------------
{
	Infile infile( url );
	std::string group, entry;
	bool wslash = false;
	int level = 0;

	if ( ! infile.open() )
		return false;

	_keys.clear();

	while ( ! infile.isEof() ) {

		if (! infile.readLine( entry, wslash ))
			continue;

		if ( entry.empty() )
			continue;

		if (std::strncmp( entry.c_str(), "#", 1 ) == 0 ||
			std::strncmp( entry.c_str(), "/*", 2 ) == 0 ||
			std::strncmp( entry.c_str(), "//", 2 ) == 0 ||
			std::strncmp( entry.c_str(), ".enc.", 5 ) == 0)
		{
			continue;
		}

//std::cout << "Config LINE='" << entry << "'\n";

		Utils::strtrim( entry );

		if ( entry.empty() )
			continue;

		getEntries( entry, group, level );
	}
	infile.close();
	return size() > 0;
}

//-----------------------------------------------------------------------------
void Config::getEntries(std::string& entry, std::string& group, int& level)
//-----------------------------------------------------------------------------
{
	const char *strbuf = entry.c_str();
	const char *delimReg = "\t=:,;";
	std::string::size_type start;
	std::string key, value;

	int len = entry.length();

	if (strbuf[0] == '[' && strbuf[len-1] == ']') {

		if (std::strncmp( strbuf, "[/", 2 ) == 0) {
			level -= 1;

			if ((start = group.rfind( '/' )) != std::string::npos) {
				group = group.substr( 0, start );
			} else {
				group.clear();
			}
		} else {
			Utils::strdel( entry, "[" );
			Utils::strdel( entry, "]" );
			Utils::strtrim( entry );

			_keys.push_back( entry );

			// Drop main/starting tag ...
			//
			if ( level++ > 0 ) {
			//std::cout << "Config GROUP '" << entry << "' LEVEL=" << level << "'\n";
				if ( group.empty() ) {
					group = entry;
				} else {
					group += "/";
					group += entry;
				}
			}
		}
		return;
	}

	char *strptr = (char*) std::strpbrk( strbuf, delimReg );

	if ( strptr ) {
		(*strptr) = '\0';
		key = strbuf;
		value = ++strptr;
	} else {
		key = entry;
		value = entry;
	}

	Utils::strtrim( key );
	Utils::strtrim( value );
	Utils::strrepl( value, "\\n", "\n" );

	if ( key.length() > 0 ) {

		if ( group.length() > 0 ) {
			key.insert( 0, "/" );
			key.insert( 0, group );
		}

		if ((_mode & Config::ValueAsKey) > 0) {
			operator[]( value ) = key;
		} else {
			operator[]( key ) = value;
//std::cout << "GROUP='" << group << "' KEY='" << key << "' VALUE='" << value << "'\n";
		}
	}
}

//-----------------------------------------------------------------------------
bool Config::string2bool(const std::string& str)
//-----------------------------------------------------------------------------
{
	if (str.empty() || str.find( '|' ) != std::string::npos)
		return false;

	std::string pattern( "|" + str + "|" );
	std::string yes( "|true|yes|on|enabled|active|1|" );

	if (yes.find( pattern ) != std::string::npos)
		return true;

	std::string YES( "|TRUE|YES|ON|ENABLED|ACTIVE|1|" );

	if (YES.find( pattern ) != std::string::npos)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
int Config::percent2int(const std::string& str, int total)
//-----------------------------------------------------------------------------
{
	std::string::size_type offset = str.find( '%' );
	std::string::size_type offset1 = str.find( '-' );
	std::string value( str );
	std::string minus;
	bool percent = false;
	char* err = 0;

	if (offset1 != std::string::npos) {
		minus.assign( str );
		minus.erase( 0, offset1+1 );
		value = value.substr( 0, offset1 );
	}

	if (offset != std::string::npos) {
		value.erase( offset, 1 );
		percent = true;
	}

	float intval = strtof( value.c_str(), &err );

//std::cerr << "percent '" << str << "'  '" << value << "' " << intval << " ERR=" << err << "\n";

	if (percent && *err == 0) {
		intval = intval * total / 100; 

		if ( !minus.empty() ) {
			intval -= (int) strtol( minus.c_str(), &err, 10 );
		}
	}

	return (*err == 0) ? intval : 0;
}

//-----------------------------------------------------------------------------
int Config::string2int(const std::string& str, bool* ok)
//-----------------------------------------------------------------------------
{
	char* err = 0;
	int val = (int) strtol( str.c_str(), &err, 10 );
	if (ok) { *ok = (*err == 0); }
	return (*err == 0) ? val : 0;
}

//-----------------------------------------------------------------------------
float Config::string2float(const std::string& str, bool* ok)
//-----------------------------------------------------------------------------
{
	char* err = 0;
	float val = (int)strtof( str.c_str(), &err );
	if (ok) { *ok = (*err == 0); }
	return (*err == 0) ? val : 0;
}

//-----------------------------------------------------------------------------
void Config::splitKey(std::vector<std::string>& v, const std::string& key, char chr)
//-----------------------------------------------------------------------------
{
	std::string text( get( key ));
	Config::split( v, text, chr  );
}

//-----------------------------------------------------------------------------
void Config::split(std::vector<std::string>& v, const std::string& text, char chr)// bool useEmpty)
//-----------------------------------------------------------------------------
{
	std::string::const_iterator it = text.begin();

	v.clear();

	//std::cout << text << "\n";

	while (true) {

		std::string::const_iterator start = it;

		while (*it != chr && it != text.end()) { ++it; }

		std::string tmp( std::string( start, it ));

		//if (useEmpty || tmp.size() > 0)
		if (tmp.size() > 0)
			v.push_back( tmp );

		if (it == text.end())
			break;

		if (++it == text.end()) {
			//if (useEmpty)
			//	v.push_back("");
			break;
		}
	}
}

};

//================================== EOF ======================================
