FILE (GLOB project_SRCS *.cpp *.cxx *.cc *.C *.c *.h *.hpp EXCLUDE keysec.cc)

SET (project_BIN ${PROJECT_NAME})

IF (USE_KEYUTILS)
	SET(${project_SRCS} keysec.cc)
ENDIF ()

IF (GLIB2 STREQUAL "ON" OR QT4 STREQUAL "YES")
	PKG_CHECK_MODULES(GLIB2  "glib-2.0>=2.10")
ENDIF ()

ADD_EXECUTABLE(${project_BIN} ${project_SRCS})

INSTALL(TARGETS ${project_BIN} DESTINATION sbin)
