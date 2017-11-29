AM_CPPFLAGS = -I../.. -I$(srcdir)/../.. -Wall -std=c++11
EXTRA_DIST = .indent.pro
AUTOMAKE_OPTIONS = subdir-objects

noinst_LTLIBRARIES = libtskfs.la
# Note that the .h files are in the top-level Makefile
libtskfs_la_SOURCES  = tsk_fs_i.h fs_inode.c fs_io.c fs_block.c fs_open.c \
    fs_name.c fs_dir.c fs_types.c fs_attr.c fs_attrlist.c fs_load.c \
    fs_parse.c fs_file.c \
    unix_misc.c nofs_misc.c \
    ffs.c ffs_dent.c ext2fs.c ext2fs_dent.c ext2fs_journal.c \
    fatfs.c fatfs_meta.c fatfs_dent.cpp \
    fatxxfs.c fatxxfs_meta.c fatxxfs_dent.c \
    exfatfs.c exfatfs_meta.c exfatfs_dent.c \
    fatfs_utils.c \
    ntfs.c ntfs_dent.cpp swapfs.c rawfs.c \
    iso9660.c iso9660_dent.c \
    hfs.c hfs_dent.c hfs_journal.c hfs_unicompare.c \
    dcalc_lib.c dcat_lib.c dls_lib.c dstat_lib.c ffind_lib.c \
    fls_lib.c icat_lib.c ifind_lib.c ils_lib.c usn_journal.c usnjls_lib.c \
    walk_cpp.cpp yaffs.cpp \
    btrfs/Basics/Basics.h \
    btrfs/Basics/BlockGroupItem.cpp btrfs/Basics/BlockGroupItem.h \
    btrfs/Basics/BtrfsHeader.h btrfs/Basics/BtrfsHeader.cpp \
    btrfs/Basics/BtrfsItem.h \
    btrfs/Basics/BtrfsKey.h btrfs/Basics/BtrfsKey.cpp \
    btrfs/Basics/ChunkData.h btrfs/Basics/ChunkData.cpp \
    btrfs/Basics/ChunkItem.h btrfs/Basics/ChunkItem.cpp \
    btrfs/Basics/DevData.h btrfs/Basics/DevData.cpp \
    btrfs/Basics/DirItem.h btrfs/Basics/DirItem.cpp \
    btrfs/Basics/Enums.h \
    btrfs/Basics/Exceptions.h \
    btrfs/Basics/ExtentData.h btrfs/Basics/ExtentData.cpp \
    btrfs/Basics/ExtentItem.h btrfs/Basics/ExtentItem.cpp \
    btrfs/Basics/InodeData.h btrfs/Basics/InodeData.cpp \
    btrfs/Basics/InodeItem.h btrfs/Basics/InodeItem.cpp \
    btrfs/Basics/InodeRef.h btrfs/Basics/InodeRef.cpp \
    btrfs/Basics/ItemHead.h btrfs/Basics/ItemHead.cpp \
    btrfs/Basics/KeyPtr.h btrfs/Basics/KeyPtr.cpp \
    btrfs/Basics/RootItem.h btrfs/Basics/RootItem.cpp \
    btrfs/Basics/RootRef.h btrfs/Basics/RootRef.cpp \
    btrfs/Basics/UnknownItem.h btrfs/Examiners/Examiners.h \
    btrfs/Examiners/Functions.h btrfs/Examiners/Functions.cpp \
    btrfs/Examiners/TreeExaminer.h btrfs/Examiners/TreeExaminer.cpp \
    btrfs/Trees/BtrfsNode.h btrfs/Trees/BtrfsNode.cpp \
    btrfs/Trees/ChunkTree.h btrfs/Trees/ChunkTree.cpp \
    btrfs/Trees/DirContent.h btrfs/Trees/DirContent.cpp \
    btrfs/Trees/FilesystemTree.h btrfs/Trees/FilesystemTree.cpp \
    btrfs/Trees/InternalNode.h btrfs/Trees/InternalNode.cpp \
    btrfs/Trees/LeadNode.h btrfs/Trees/LeafNode.cpp \
    btrfs/Trees/SuperBlock.h btrfs/Trees/SuperBlock.cpp \
    btrfs/Trees/Trees.h \
    zfs/Blkptr.cpp zfs/Blkptr.h \
    zfs/Dnode.cpp zfs/Dnode.h \
    zfs/NVList.cpp zfs/NVList.h \
    zfs/ObjectSet.cpp zfs/ObjectSet.h \
    zfs/Uberblock.cpp zfs/Uberblock.h \
    zfs/UberblockArray.cpp zfs/UberblockArray.h \
    zfs/ZAP.cpp zfs/ZAP.h \
    zfs/ZFS_Functions.cpp zfs/ZFS_Functions.h

indent:
	indent *.cpp*.c *.h

clean-local:
	-rm -f *.cpp *.c~ *.h~
