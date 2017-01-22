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
#ifndef _Tsert_Config_H_
#define _Tsert_Config_H_

#include <map>
#include <list>
#include <vector>
#include <iostream>
#include <istream>
#include <string>
//#include "strlist.h"

//--------------------------------- Classes -----------------------------------

namespace breeze {

class Config : public std::map<std::string, std::string> {
public:
	enum NormalizeMode { Normal, ValueAsKey };

protected:
	std::string _group_key;
	NormalizeMode _mode;
	std::list<std::string> _keys;

public:
	Config();
	Config(const std::string& path);
	virtual ~Config();

	const std::list<std::string>& getKeys() const { return _keys; }

	bool getBool(const std::string& key, bool value=false) const;
	int getInt(const std::string& key, int value=0) const;
	uint getUInt(const std::string& key, uint value=0) const;
	uint getHexa(const std::string& key, uint value=0) const;
	float getFloat(const std::string& key, float value=0.0) const;

	const std::string& get(const std::string& key,
		const std::string& val="", bool useval=false) const;

	bool contains(const std::string& key) const;

	bool get(const std::string& key, Config&, bool strip=false) const;

	void set(const std::string& key, const std::string& value);
	void set(const std::string& prefix, const Config& entries);

	void split(std::vector<std::string>& v, const std::string& key, char c);

	bool load(const std::string& url);

	Config& operator+=(const Config&);
	Config& operator<<(const std::string& path);

	void zap();
	void clearValues();

public:
	static bool string2bool(const std::string& str);
	static float string2float(const std::string& str, bool* ok);
	static int string2int(const std::string& str, bool* ok);
	static int percent2int(const std::string& str, int total);

protected:
	void getEntries(std::string& entry, std::string& group, int& level);

};

};

#endif

//==================================== EOF ====================================
