AM_CPPFLAGS = -I../.. -I$(srcdir)/../.. -Wall -std=c++11
EXTRA_DIST = .indent.pro

noinst_LTLIBRARIES = libtskpool.la
# Note that the .h files are in the top-level Makefile
libtskpool_la_SOURCES  = TSK_POOL_INFO.cpp TSK_POOL_INFO.h \
    ZFS_POOL.cpp ZFS_POOL.h ZFS_VDEV.cpp ZFS_VDEV.h \
    TSK_POOL.h TSK_POOL.cpp BTRFS_POOL.cpp BTRFS_POOL.h \
    BTRFS_DEVICE.h BTRFS_DEVICE.cpp

indent:
	indent *.cpp *.h

clean-local:
	-rm -f *.cpp~ *.h~
