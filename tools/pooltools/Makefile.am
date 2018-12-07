AM_CPPFLAGS = -I../.. -I$(srcdir)/../.. -Wall -std=c++11
LDADD = ../../tsk/libtsk.la
LDFLAGS += -static
EXTRA_DIST = .indent.pro


bin_PROGRAMS = pls pwalk
pls_SOURCES = pls.cpp
pwalk_SOURCES = pwalk.cpp

indent:
	indent *.cpp

clean-local:
	-rm -f *.cpp~
