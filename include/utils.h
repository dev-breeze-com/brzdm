// Brzdm - Breeze::OS Login/Display Manager
//
// @author Pierre Innocent <dev@breezeos.com>
// Copyright (C) Tsert.Inc, All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
#ifndef _BRZDM_Utils_H_
#define _BRZDM_Utils_H_

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <clocale>
#include <cctype>
#include <cstring>
#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <climits>
#include <ctime>
#include <csignal>

#include <istream>
#include <ostream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <streambuf>

#include <string>
#include <set>
#include <vector>
#include <map>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>

#include "const.h"

namespace breeze {

typedef struct timeval ChronoTime;

class Utils {
public:
	enum HashType {
		MD5_algo,
		SHA1_algo,
		SHA256_algo,
		SHA512_algo
	};

	enum DateFormat {
		SHORT_format,
		ISO_format,
		ISOCLOCK_format,
		FULL_format,
		CLOCK_format,
		RFC822_format,
		RFC2822_format
	};

private:
	Utils() {}
	~Utils() {}

public:
	static std::string _app;
	static std::string _version;
	static std::string _tmp_dir;
	static std::string _root_dir;
	static std::string _home_dir;
	static std::string _start_dir;
	static std::string _mountpoint;

public:
	static void init(const std::string& app, const std::string& version);

	static uint elapsedMSecs(const struct timeval&);

	static char* strcat(const std::string&, const std::string&);
	static char* strdupl(const char* const str, int sz=0);

	static void strfree(char* str);
	static void strzap(std::string& s);
	static void strtrim(std::string& s);
	static void strdel(std::string& s, const std::string& pattern);

	static std::string strlower(const std::string& s);
	static std::string strrepl(const std::string& s,
		const std::string& pattern, const std::string& repl);

	static std::string resolve(const std::string& url, const std::string& dir);

	static std::string hash(const std::string& path, HashType algo);

	static std::string dateString(const std::string& format, std::time_t=0);

	static std::string get_uuid(bool macaddr=false);

	static bool add_mcookie(const std::string &mcookie,
		const std::string& display, const std::string &xauth_cmd,
		const std::string &authfile);

	static void srandom();
	static long random(void);
	static long makeseed(void);

};

};

#endif

//==================================== EOF ====================================
