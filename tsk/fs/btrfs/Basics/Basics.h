#ifndef BASICS_H
#define BASICS_H

#define BLOCK_FLAG_DATA         0x001
#define BLOCK_FLAG_SYSTEM       0x002
#define BLOCK_FLAG_METADATA     0x004
#define BLOCK_FLAG_RAID0        0x008
#define BLOCK_FLAG_RAID1        0x010
#define BLOCK_FLAG_DUPLICATE    0x020
#define BLOCK_FLAG_RAID10       0x040
#define BLOCK_FLAG_RAID5        0x080
#define BLOCK_FLAG_RAID6        0x100
#define BTRFS_NUM_BACKUP_ROOTS 4

#include "Exceptions.h"
#include "Enums.h"
#include "BtrfsKey.h"
#include "BtrfsHeader.h"
#include "ItemHead.h"
#include "BtrfsItem.h"
#include "KeyPtr.h"

#include "InodeData.h"
#include "InodeItem.h"
#include "RootItem.h"
#include "RootRef.h"

#include "InodeRef.h"
#include "DevData.h"
#include "DirItem.h"

#include "ExtentData.h"
#include "ChunkItem.h"
#include "ExtentItem.h"
#include "BlockGroupItem.h"

#include "UnknownItem.h"
#include "../../../../tools/fiwalk/src/sha2.h"

typedef struct BTRFSPhyAddr{
    uint64_t offset;
    uint16_t device;
} BTRFSPhyAddr;

typedef struct btrfs_root_backup {
    uint64_t tree_root;
    uint64_t tree_root_gen;

    uint64_t chunk_root;
    uint64_t chunk_root_gen;

    uint64_t extent_root;
    uint64_t extent_root_gen;

    uint64_t fs_root;
    uint64_t fs_root_gen;

    uint64_t dev_root;
    uint64_t dev_root_gen;

    uint64_t csum_root;
    uint64_t csum_root_gen;

    uint64_t total_bytes;
    uint64_t bytes_used;
    uint64_t num_devices;
    uint64_t unused_64[4];

    uint8 tree_root_level;
    uint8 chunk_root_level;
    uint8 extent_root_level;
    uint8 fs_root_level;
    uint8 dev_root_level;
    uint8 csum_root_level;
} btrfs_root_backup;

#endif

