AM_CPPFLAGS = -I../.. -I$(srcdir)/../.. -Wall -std=c++11
EXTRA_DIST = .indent.pro

noinst_LTLIBRARIES = libtskutils.la
# Note that the .h files are in the top-level Makefile
libtskutils_la_SOURCES  = lz4.h lz4.cpp ReadInt.h ReadInt.cpp Tools.h Tools.cpp

indent:
	indent *.cpp *.h

clean-local:
	-rm -f *.cpp~ *.h~
