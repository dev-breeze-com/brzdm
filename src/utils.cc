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
#include <uuid/uuid.h>
#include <nettle/md5.h>
#include <nettle/sha.h>
#include <nettle/base16.h>

#include "utils.h"
#include "config.h"
#include "infile.h"

namespace breeze {

//---------------------------------- Globals ----------------------------------

std::string Utils::_app;
std::string Utils::_version;
std::string Utils::_tmp_dir;
std::string Utils::_home_dir;
std::string Utils::_start_dir;
std::string Utils::_mountpoint;

//--------------------------------- Functions ---------------------------------

//-----------------------------------------------------------------------------
void Utils::init(const std::string& app, const std::string& version)
//-----------------------------------------------------------------------------
{
	char *ccwd = ::get_current_dir_name();
	ChronoTime now;

	_app = app;
	_version = version;
	_start_dir = ccwd;

	_tmp_dir = "/var/lib/brzdm/";
	_home_dir = "/var/lib/brzdm/";
	_mountpoint = "/var/mnt/media/";

	::gettimeofday( &now, 0L );

    ::srandom( now.tv_sec + now.tv_usec );

	::free( ccwd );
}

//-----------------------------------------------------------------------------
void Utils::strfree(char* str) { delete [] str; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
std::string Utils::strlower(const std::string& str)
//-----------------------------------------------------------------------------
{
	std::string lstr( str );

	for (auto& chr: lstr)
		chr = std::tolower( chr );

	return lstr;
}

//-----------------------------------------------------------------------------
char* Utils::strcat(const std::string& str1, const std::string& str2)
//-----------------------------------------------------------------------------
{
	std::string tmp( str1 );
	tmp.append( str2 );
	return strdupl( tmp.c_str(), tmp.length() );
}

//-----------------------------------------------------------------------------
char* Utils::strdupl(const char* const str, int sz)
//-----------------------------------------------------------------------------
{
	if ( !str ) { std::abort(); }
	sz = sz > 0 ? sz : std::strlen( str );
	char *new_str = ::new char[ sz+2 ];
	std::memcpy( new_str, str, sz );
	new_str[sz] = '\0';
	return new_str;
}

//-----------------------------------------------------------------------------
std::string Utils::dateString(const std::string& format, std::time_t now)
//-----------------------------------------------------------------------------
{
	char datestr[256]={0};
	struct tm *timerec = 0L;

	now = now ? now : std::time(0);
	timerec = ::localtime( &now );

	::strftime( datestr, 255, format.c_str(), timerec );

	return datestr;
}

//-----------------------------------------------------------------------------
std::string Utils::hash(const std::string& path, HashType algo)
//-----------------------------------------------------------------------------
{
	struct sha1_ctx ctxsha1;
	struct sha256_ctx ctx256;
	struct sha512_ctx ctx512;
	struct md5_ctx ctxmd5;
	::size_t sz = 0;

	Infile file( path );
	char digest[512]={0};
	char data[4097]={0};
	char hexstr[3]={0};
	std::string hash, hashed;

	if ( ! file.open() ) {
		perror( "error: " );
		return std::string();
	}

	if (algo == MD5_algo) {

		::md5_init( &ctxmd5 );

		while ((sz = file.read( data, 4096 )) > 0)
			::md5_update( &ctxmd5, sz, (uint8_t*) data );

		::md5_digest( &ctxmd5, MD5_DIGEST_SIZE, (uint8_t*) digest );

		hash.assign( digest, MD5_DIGEST_SIZE );

	} else if (algo == SHA1_algo) {

		::sha1_init( &ctxsha1 );

		while ((sz = file.read( data, 4096 )) > 0)
			::sha1_update( &ctxsha1, sz, (uint8_t*) data );

		::sha1_digest( &ctxsha1, SHA1_DIGEST_SIZE, (uint8_t*) digest );

		hash.assign( digest, SHA1_DIGEST_SIZE );

	} else if (algo == SHA256_algo) {

		::sha256_init( &ctx256 );

		while ((sz = file.read( data, 4096 )) > 0)
			::sha256_update( &ctx256, sz, (uint8_t*) data );

		::sha256_digest( &ctx256, SHA256_DIGEST_SIZE, (uint8_t*) digest );

		hash.assign( digest, SHA256_DIGEST_SIZE );

	} else if (algo == SHA512_algo) {

		::sha512_init( &ctx512 );

		while ((sz = file.read( data, 4096 )) > 0)
			::sha512_update( &ctx512, sz, (uint8_t*) data );

		::sha512_digest( &ctx512, SHA512_DIGEST_SIZE, (uint8_t*) digest );

		hash.assign( digest, SHA512_DIGEST_SIZE );
	}

	for (const auto& chr: hash) {
#if defined(NETTLE_LESSER)
		::base16_encode_single((uint8_t*)hexstr, chr );
#else
		::base16_encode_single(hexstr, chr );
#endif
		hashed.append( hexstr, 2 );
	}

	return hashed;
}

/*
 * Adds the given cookie to the specified Xauthority file.
 * Returns true on success, false on fault.
 */
//-----------------------------------------------------------------------------
bool Utils::add_mcookie(const std::string &mcookie, const std::string& display,
	const std::string& xauth_cmd, const std::string& authfile)
//-----------------------------------------------------------------------------
{
	std::string cmd( xauth_cmd + " -f " + authfile + " -v" );

	/*
	std::cout << "MCOOKIE='" << mcookie << "'\n";
	std::cout << "XAUTH_CMD='" << xauth_cmd << "'\n";
	std::cout << "DISPLAY='" << display << "'\n";
	std::cout << "AUTHFILE='" << authfile << "'\n";
	std::cout.flush();
	*/

	FILE *fptr = ::popen( cmd.c_str(), "w" );

	if (fptr) {
		fprintf(fptr, "remove %s\n", display.c_str());
		fprintf(fptr, "add %s %s %s\n", display.c_str(), ".", mcookie.c_str());
		fprintf(fptr, "exit\n");
		::pclose(fptr);
	}

	return fptr != 0L;
}

//-----------------------------------------------------------------------------
void Utils::srandom()
//-----------------------------------------------------------------------------
{
	::srandom( makeseed() );
}

//-----------------------------------------------------------------------------
long Utils::random()
//-----------------------------------------------------------------------------
{
	return ::random();
}

//-----------------------------------------------------------------------------
long Utils::makeseed()
//-----------------------------------------------------------------------------
{
	struct timespec ts;
	long pid = getpid();
	long tm = std::time(0);

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		ts.tv_sec = ts.tv_nsec = 0;

	return pid + tm + (ts.tv_sec ^ ts.tv_nsec);
}

//-----------------------------------------------------------------------------
std::string Utils::strrepl(const std::string& text, const std::string& pattern, const std::string& replstr)
//-----------------------------------------------------------------------------
{
	std::string::size_type pos = 0;
	std::string s( text );
	int len = pattern.size();

	while ((pos = s.find(pattern, pos)) != std::string::npos)
		s = s.substr( 0, pos ) + replstr + s.substr( pos+len );

	return s;
}

//-----------------------------------------------------------------------------
void Utils::strzap(std::string& s)
//-----------------------------------------------------------------------------
{
	for (auto& chr: s) { chr = '0'; }
	s.clear();
}

//-----------------------------------------------------------------------------
void Utils::strdel(std::string& s, const std::string& text)
//-----------------------------------------------------------------------------
{
	std::string::size_type offset = s.find( text, 0 );
	if (offset != std::string::npos)
		s.erase( offset, text.length() );
}

//-----------------------------------------------------------------------------
void Utils::strtrim(std::string& s)
//-----------------------------------------------------------------------------
{
	if ( ! s.empty() ) {

		int len = s.size();
		int pos = 0;

		while (pos < len && std::isspace( s[pos] ))
			++pos;

		s.erase( 0, pos );

		pos = s.length()-1;

		while (pos > -1 && std::isspace( s[pos] ))
			--pos;

		if (pos != -1) {
			s.erase( pos+1 );
		}
	}
}

//-----------------------------------------------------------------------------
std::string Utils::resolve(const std::string& path, const std::string& folder)
//-----------------------------------------------------------------------------
{
	std::string url( path );

	if (path.find( '/' ) != 0) {
		url.insert( 0, "/" );
		url.insert( 0, folder );
	}
	return url;
}

//-----------------------------------------------------------------------------
uint Utils::elapsedMSecs(const struct timeval& then)
//-----------------------------------------------------------------------------
{
	struct timeval now;
	uint elapsed;

	::gettimeofday( &now, 0L ); 

	elapsed = (now.tv_sec - then.tv_sec) * 1000000;
	elapsed += (now.tv_usec - then.tv_usec);

	elapsed /= 1000;

	return elapsed;
}

//-----------------------------------------------------------------------------
std::string Utils::get_uuid(bool macaddr)
//-----------------------------------------------------------------------------
{
	char buffer[ 128 ]={ '\0' };

	::uuid_t uuid = { 0 };

	if ( macaddr ) {
		::uuid_generate_time( uuid );
	} else {
		::uuid_generate( uuid );
	}

	::uuid_unparse_lower( uuid, buffer );

	return std::string( buffer );
}

};

//==================================== EOF ====================================
