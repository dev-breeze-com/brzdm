# - Try to find the NETTLE libraries
# Once done this will define
#
#  NETTLE_FOUND - system has nettle
#  NETTLE_INCLUDE_DIR - the nettle include directory
#  NETTLE_LIBRARIES - libnettle library

if (NETTLE_INCLUDE_DIR AND NETTLE_LIBRARY)
	# Already in cache, be silent
	set(NETTLE_FIND_QUIETLY TRUE)
endif (NETTLE_INCLUDE_DIR AND NETTLE_LIBRARY)

find_path(NETTLE_INCLUDE_DIR NAMES nettle.h)
find_library(NETTLE_LIBRARY nettle)
find_library(DL_LIBRARY dl)

if (NETTLE_INCLUDE_DIR AND NETTLE_LIBRARY)
	set(NETTLE_FOUND TRUE)
	if (DL_LIBRARY)
		set(NETTLE_LIBRARIES ${NETTLE_LIBRARY} ${DL_LIBRARY})
	else (DL_LIBRARY)
		set(NETTLE_LIBRARIES ${NETTLE_LIBRARY})
	endif (DL_LIBRARY)

	if (EXISTS ${NETTLE_INCLUDE_DIR}/nettle.h)
		# darwin claims to be something special
		set(HAVE_NETTLE_NETTLE_APPL_H 1)
	endif (EXISTS ${NETTLE_INCLUDE_DIR}/nettle.h)
endif (NETTLE_INCLUDE_DIR AND NETTLE_LIBRARY)

if (NETTLE_FOUND)
	if (NOT NETTLE_FIND_QUIETLY)
		message(STATUS "Found Nettle: ${NETTLE_LIBRARIES}")
	endif (NOT NETTLE_FIND_QUIETLY)
else (NETTLE_FOUND)
	if (NETTLE_FIND_REQUIRED)
		message(FATAL_ERROR "Nettle was not found")
	endif(NETTLE_FIND_REQUIRED)
endif (NETTLE_FOUND)

mark_as_advanced(NETTLE_INCLUDE_DIR NETTLE_LIBRARY DL_LIBRARY NETTLE_MESSAGE_CONST)
