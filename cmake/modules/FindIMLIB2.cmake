# - Try to find the IMLIB2 libraries
# Once done this will define
#
#  IMLIB2_FOUND - system has pam
#  IMLIB2_INCLUDE_DIR - the pam include directory
#  IMLIB2_LIBRARIES - libpam library

if (IMLIB2_INCLUDE_DIR AND IMLIB2_LIBRARY)
	# Already in cache, be silent
	set(IMLIB2_FIND_QUIETLY TRUE)
endif (IMLIB2_INCLUDE_DIR AND IMLIB2_LIBRARY)

find_path(IMLIB2_INCLUDE_DIR NAMES Imlib2.h)
find_library(IMLIB2_LIBRARY Imlib2)
find_library(DL_LIBRARY dl)

if (IMLIB2_INCLUDE_DIR AND IMLIB2_LIBRARY)
	set(IMLIB2_FOUND TRUE)
	if (DL_LIBRARY)
		set(IMLIB2_LIBRARIES ${IMLIB2_LIBRARY} ${DL_LIBRARY})
	else (DL_LIBRARY)
		set(IMLIB2_LIBRARIES ${IMLIB2_LIBRARY})
	endif (DL_LIBRARY)

	if (EXISTS ${IMLIB2_INCLUDE_DIR}/Imlib/imlib2.h)
		# darwin claims to be something special
		set(HAVE_IMLIB2_IMLIB2_APPL_H 1)
	endif (EXISTS ${IMLIB2_INCLUDE_DIR}/Imlib/imlib2.h)
endif (IMLIB2_INCLUDE_DIR AND IMLIB2_LIBRARY)

if (IMLIB2_FOUND)
	if (NOT IMLIB2_FIND_QUIETLY)
		message(STATUS "Found Imlib2: ${IMLIB2_LIBRARIES}")
	endif (NOT IMLIB2_FIND_QUIETLY)
else (IMLIB2_FOUND)
	if (IMLIB2_FIND_REQUIRED)
		message(FATAL_ERROR "Imlib2 was not found")
	endif(IMLIB2_FIND_REQUIRED)
endif (IMLIB2_FOUND)

mark_as_advanced(IMLIB2_INCLUDE_DIR IMLIB2_LIBRARY DL_LIBRARY IMLIB2_MESSAGE_CONST)
