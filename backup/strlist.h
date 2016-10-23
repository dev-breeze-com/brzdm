// Brzdm - Breeze::OS Login/Display Manager
//
// Copyright (C) 2015 Tsert.Com <dev@breezeos.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
#ifndef _Brzdm_StrList_H_
#define _Brzdm_StrList_H_

#include <list>
#include <string>
#include <algorithm>

//--------------------------------- Classes -----------------------------------

namespace breeze {

class StringList : public std::list<std::string> {
public:
	enum SearchMode { MATCH_search=0, FULL_search, APPROX_search };

	void split(const char* const vector[]);
	void split(const std::string& text, const int, bool skipempty);

public:
	StringList();
	StringList(const std::string&);
   ~StringList();

	inline int count() const { return std::list<std::string>::size(); }

	inline const std::string& first() const { return front(); }
	inline const std::string& last() const { return back(); }

	inline void prepend(const std::string& str) { push_front( str ); }
	inline void dequeue() { pop_front(); }

public:
	bool contains(const std::string& str) const;

	//const std::string& find(const std::string& str) const;
	const std::string& get(const std::string& str, SearchMode=MATCH_search) const;

	int indexOf(const std::string& str, SearchMode=MATCH_search) const;

	const std::string& operator[](int idx) const;

	StringList& operator<<(const std::string& path);
	StringList& operator>>(const std::string& path);

	StringList& operator+=(const std::string& str);
	StringList& operator+=(const StringList& wrklst);

};

};

#endif

//==================================== EOF ====================================
