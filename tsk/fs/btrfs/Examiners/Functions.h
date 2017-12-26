//! \file
//! \author Shujian Yang
//!
//! Assistant functions defined here.

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#define BLOCK_FLAG_DATA         0x001
#define BLOCK_FLAG_SYSTEM       0x002
#define BLOCK_FLAG_METADATA     0x004
#define BLOCK_FLAG_RAID0        0x008
#define BLOCK_FLAG_RAID1        0x010
#define BLOCK_FLAG_DUPLICATE    0x020
#define BLOCK_FLAG_RAID10       0x040
#define BLOCK_FLAG_RAID5        0x080
#define BLOCK_FLAG_RAID6        0x100

#include <iostream>
#include <vector>
#include "Examiners.h"

namespace btrForensics {


    void printLeafDir(const LeafNode *, std::ostream &);

    bool searchForItem(const LeafNode *, uint64_t, ItemType, const BtrfsItem *&);

    bool filterItems(const LeafNode *, uint64_t, ItemType, vector<const BtrfsItem *> &);

    void filterItems(const LeafNode *, ItemType, vector<const BtrfsItem *> &);

    bool getPhyAddr(const LeafNode *leaf, uint64_t targetLogAddr, BTRFSPhyAddr &physAddr);

    BTRFSPhyAddr getChunkAddr(uint64_t logicalAddr,
                              const BtrfsKey *key, const ChunkData *chunkData);

    uint64_t getChunkLog(const LeafNode *, uint64_t, uint64_t &);

    std::ostream &operator<<(std::ostream &os, const DirItemType &type);
}

#endif
