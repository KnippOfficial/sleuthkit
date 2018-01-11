//! \file
//! \author Shujian Yang
//!
//! Assistant functions defined here.

#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#include <iostream>
#include <vector>
#include "Examiners.h"
#include "../Basics/Basics.h"

namespace btrForensics {


    void printLeafDir(const LeafNode *, std::ostream &);

    bool searchForItem(const LeafNode *, uint64_t, ItemType, const BtrfsItem *&);

    bool filterItems(const LeafNode *, uint64_t, ItemType, vector<const BtrfsItem *> &);

    void filterItems(const LeafNode *, ItemType, vector<const BtrfsItem *> &);

    bool getPhyAddr(const LeafNode *leaf, uint64_t targetLogAddr, vector<BTRFSPhyAddr>& physAddr);

    vector<BTRFSPhyAddr> getChunkAddr(uint64_t logicalAddr,
                              const BtrfsKey *key, const ChunkData *chunkData);

    uint64_t getChunkLog(const LeafNode *, uint64_t, uint64_t &);

    std::ostream &operator<<(std::ostream &os, const DirItemType &type);
}

#endif
