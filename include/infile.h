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
#ifndef _BRZDM_INFILE_H_
#define _BRZDM_INFILE_H_

#include <string>
#include <istream>
#include <iostream>

//--------------------------------- Classes -----------------------------------

namespace breeze {

class Infile {
protected:
	std::FILE *_fptr;
	std::string _path;
	char *_buffer;

public:
	Infile(const std::string& path="");
    virtual ~Infile();

public:
	bool isOpened() { return _fptr != NULL; }
	long getOffset() { return ::ftell( _fptr ); }
	bool isEof() const { return ::feof( _fptr ); }

	int getSize() const;

	int read(char*, ::size_t);
	bool readLine(std::string& line, bool& wslash); 

	bool open(const std::string& path="");
	void setPath(const std::string& path);
	void close();

};

};

#endif

//=================================== EOF =====================================
