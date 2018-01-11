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

typedef struct BTRFSPhyAddr{
    uint64_t offset;
    uint16_t device;
} BTRFSPhyAddr;

#endif

