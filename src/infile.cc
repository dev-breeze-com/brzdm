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
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "infile.h"

namespace breeze {

//-----------------------------------------------------------------------------
Infile::Infile(const std::string& path)
//-----------------------------------------------------------------------------
{
	_fptr = 0L;
	_path = path;
	_buffer = new char[ 32768 ];
}

//-----------------------------------------------------------------------------
Infile::~Infile()
//-----------------------------------------------------------------------------
{
	Infile::close();
	delete [] _buffer;
}

//-----------------------------------------------------------------------------
void Infile::close()
//-----------------------------------------------------------------------------
{
	if ( _fptr ) {
		std::fclose( _fptr );
		_fptr = NULL;
	}
}

//-----------------------------------------------------------------------------
int Infile::getSize() const
//-----------------------------------------------------------------------------
{
	struct stat buf;
	if (::lstat( _path.c_str(), &buf ) == 0)
		return buf.st_size;
	return (-1);
}

//-----------------------------------------------------------------------------
bool Infile::open(const std::string& path)
//-----------------------------------------------------------------------------
{
	if ( !path.empty() )
		Infile::setPath( path );

	_fptr = std::fopen( _path.c_str(), "r" );
	return _fptr != NULL;
}

//-----------------------------------------------------------------------------
void Infile::setPath(const std::string& path)
//-----------------------------------------------------------------------------
{
	_path = path;
	Infile::close();
}

//-----------------------------------------------------------------------------
int Infile::read(char *buf, ::size_t sz)
//-----------------------------------------------------------------------------
{
	return std::fread( buf, 1, sz, _fptr );
}

//-----------------------------------------------------------------------------
bool Infile::readLine(std::string& line, bool& has_slash)
//-----------------------------------------------------------------------------
{
	char *bufptr = _buffer;
	size_t sz = 16384;
	int total = 0;

	has_slash = false;
	line.clear();

	for (;;) {
		int nread = ::getline( &bufptr, &sz, _fptr ); 

		if ( nread < 1 )
			return false;

		//std::cout << bufptr;

		bufptr += nread-1;
		total += nread-1;

		if (*bufptr == '/') {
			has_slash = true;
			bufptr--;
			total--;
		}
		else {
			break;
		}
		sz -= nread;
	}

	if (total > 0) {
		line.assign( _buffer, total );
	}
	return true;
}

};

//=================================== EOF =====================================
