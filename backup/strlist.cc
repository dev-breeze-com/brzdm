// Brzdm - Breeze::OS Login/Display Manager
//
// Copyright (C) 2015 Tsert.Com <dev@breezeos.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
#include "utils.h"
#include "infile.h"
#include "strlist.h"

namespace breeze {

//-----------------------------------------------------------------------------
StringList::~StringList() { clear(); }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
StringList::StringList() {}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
StringList::StringList(const std::string& path)
//-----------------------------------------------------------------------------
{
	StringList::operator<<( path );
}

/*
//-----------------------------------------------------------------------------
const std::string& StringList::find(const std::string& key) const
//-----------------------------------------------------------------------------
{
	if ( key.empty() ) { return String::null; }
	const_iterator it = std::lower_bound( begin(), end(), key );
	if ( it != end() )
		return (*it) == key ? key : String::null;
	return String::null;
}
*/

//-----------------------------------------------------------------------------
bool StringList::contains(const std::string& key) const
//-----------------------------------------------------------------------------
{
	if ( key.empty() ) { return false; }
	return std::binary_search( begin(), end(), key );
}

//-----------------------------------------------------------------------------
const std::string& StringList::get(const std::string& key, SearchMode mode) const
//-----------------------------------------------------------------------------
{
	static std::string null;
	const_iterator it;
	const_iterator iend;

	if ( key.empty() )
		return null;

	it = std::lower_bound( begin(), end(), key );

	if ( it != end() )
		return (*it) == key ? key : null;

	it = StringList::begin();
	iend = StringList::end();

	for (; it != iend; it++) {
//COUT_2 << "StringList::get '" << key << "' '" << (*it) << "'\n";

		switch ( mode ) {
			case APPROX_search:
				if ((*it).find( key ) >= 0)
					return (*it);
			break;
			case FULL_search:
				if ((*it).find( key ) == 0)
					return (*it);
			break;
			default:
				if (key == (*it))
					return (*it);
			break;
		}
	}
	return null;
}

//-----------------------------------------------------------------------------
int StringList::indexOf(const std::string& key, SearchMode mode) const
//-----------------------------------------------------------------------------
{
	const_iterator it = StringList::begin();
	const_iterator iend = StringList::end();

	int i = 0;

	for (; it != iend; it++, i++) {
		switch ( mode ) {
			case APPROX_search:
				if ((*it).find( key ) >= 0)
					return i;
			break;
			case FULL_search:
				if ((*it).find( key ) == 0)
					return i;
			break;
			default:
				if (key == (*it))
					return i;
			break;
		}
	}
	return (-1);
}

//-----------------------------------------------------------------------------
StringList& StringList::operator+=(const std::string& str)
//-----------------------------------------------------------------------------
{
	std::list<std::string>::push_back( str );
	return *this;
}

//-----------------------------------------------------------------------------
StringList& StringList::operator+=(const StringList& entries)
//-----------------------------------------------------------------------------
{
	StringList::const_iterator it = entries.begin();
	StringList::const_iterator iend = entries.end();

	for (; it != iend; it++)
		std::list<std::string>::push_back( (*it) );

	return *this;
}

#if 0
//-----------------------------------------------------------------------------
StringList& StringList::operator+=(const Config& entries)
//-----------------------------------------------------------------------------
{
	Config::const_iterator it = entries.begin();
	Config::const_iterator iend = entries.end();

	for (; it != iend; it++)
		std::list<std::string>::push_back( (*it).first );

	return *this;
}
#endif

//-----------------------------------------------------------------------------
StringList& StringList::operator<<(const std::string& path)
//-----------------------------------------------------------------------------
{
	Infile infile( path );

	if ( infile.open() ) {
		bool slashes = false;
		int start = 0;
		std::string suffix( path );
		std::string line;

		if ((start = path.rfind( '.' )) > 0)
			suffix.erase( 0, start+1 );

		bool nolstmap = suffix != "lst" && suffix != "cfg";

		while ( !infile.isEof() && infile.readLine( line, slashes )) {
			if ( nolstmap )
				push_back( line );
			else
			if (line.find( '#' ) != 0 && 0 != line.find( "//" )) {
				push_back( line );
			}
		}
	}

	infile.close();

#if 0
	std::ifstream infile;

	infile.open( path.utf8() );

	if ( infile.is_open() ) {
	if ( infile.open() ) {

		IO_Buffer *buffer = ::new IO_Buffer( BUFSIZE );
		std::string line, suffix( path );
		int start = 0;

		if ((start = path.rfind( '.' )) > 0)
			suffix.remove( 0, start+1 );

		for (int nread = 1; nread > 0; buffer->reset()) {

			infile.getline( buffer->begin(), buffer->avail() );

			nread = infile.gcount();

			if ((nread = infile.gcount()) < 1)
				nread = infile.fail() ? 0 : 1;
			else {
				const char chr = buffer->ref( nread-1 );

				if (chr == '\n')
					buffer->setLength( nread-1 );
				else {
					buffer->setLength( nread );
				}

				buffer->debug(32);

COUT_2 << "StringList::operator<< nread=" << nread << "\n";
COUT_2 << "StringList::operator<< '" << line << "' LENGTH=" << line.length() << "\n";

				line.assign( buffer->utf8(), buffer->length() );
				Utils::strTrim( line );

				if ( line.empty() )
					continue;

COUT_2 << "StringList::operator<< '" << line << "' LENGTH=" << line.length() << "\n";

				if (suffix != "lst" && suffix != "cfg")
					push_back( line );
				else
				if (line.find( '#' ) != 0 && 0 != line.find( "//" )) {
					push_back( line );
				}
			}
		}
		infile.close();
		infile.clear();
		::delete buffer;
	}
#endif
	return (*this);
}

//-----------------------------------------------------------------------------
void StringList::split(const std::string& text, const int chr, bool skip_empty)
//-----------------------------------------------------------------------------
{
	std::string entry;

	// There should be no '*' wildcards at the end of the delimiter
	//
	int textlen = text.length();
	int start = 0;
	int end = 0;

	while (start < textlen) {
		if ((end = text.find( chr, start )) >= 0) {
			entry = text.substr( start, end-start );
			start = end + 1;
		}
		else {
			entry = text.substr( start, textlen-start );
			start = textlen;
		}

		if ( skip_empty ) {
			Utils::strTrim( entry );
			if ( entry.empty() )
				continue;
		}
		push_back( entry );
	}
}

};

//================================== EOF ======================================
