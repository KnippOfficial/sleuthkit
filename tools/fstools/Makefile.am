AM_CPPFLAGS = -I../.. -I$(srcdir)/../.. -Wall -std=c++11
LDADD = ../../tsk/libtsk.la
LDFLAGS += -static
EXTRA_DIST = .indent.pro fscheck.cpp

bin_PROGRAMS = blkcalc blkcat blkls blkstat ffind fls fcat fsstat icat ifind ils \
    istat jcat jls usnjls btrfs_fls btrfs_fsstat btrfs_istat btrfs_icat
blkcalc_SOURCES = blkcalc.cpp
blkcat_SOURCES = blkcat.cpp
blkls_SOURCES = blkls.cpp
blkstat_SOURCES = blkstat.cpp
ffind_SOURCES = ffind.cpp
fls_SOURCES = fls.cpp
fcat_SOURCES = fcat.cpp
fsstat_SOURCES = fsstat.cpp
icat_SOURCES = icat.cpp
ifind_SOURCES = ifind.cpp
ils_SOURCES = ils.cpp
istat_SOURCES = istat.cpp
jcat_SOURCES = jcat.cpp
jls_SOURCES = jls.cpp
usnjls_SOURCES = usnjls.cpp
btrfs_fls_SOURCES = btrfs_fls.cpp
btrfs_fsstat_SOURCES = btrfs_fsstat.cpp
btrfs_icat_SOURCES = btrfs_icat.cpp
btrfs_istat_SOURCES = btrfs_istat.cpp


indent:
	indent *.cpp

clean-local:
	-rm -f *.cpp~
