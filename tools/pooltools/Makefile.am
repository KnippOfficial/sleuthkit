AM_CPPFLAGS = -I../.. -I$(srcdir)/../.. -Wall -std=c++11
LDADD = ../../tsk/libtsk.la
LDFLAGS += -static
EXTRA_DIST = .indent.pro


bin_PROGRAMS = pls
pls_SOURCES = pls.cpp

indent:
	indent *.cpp

clean-local:
	-rm -f *.cpp~
