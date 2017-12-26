//! \file
//! \author Shujian Yang
//!
//! Header file of class ChunkTree

#include <iostream>
#include "ChunkTree.h"
#include "../Examiners/Functions.h"

namespace btrForensics {
    //! Constructor of tree analyzer.
    //!
    //! \param superBlk Pointer to btrfs super block.
    //! \param treeExaminer Pointer to a tree examiner.
    //!
    ChunkTree::ChunkTree(const SuperBlock* superBlk, const TreeExaminer* treeExaminer)
            :examiner(treeExaminer)
    {
        //cerr << "DBG: Creating ChunkTree" << endl;
        BTRFSPhyAddr chunkTreePhyAddr = superBlk->getChunkPhyAddr();
        //cerr << "DBG: device " << chunkTreePhyAddr.device << " @ " << chunkTreePhyAddr.offset << endl;
        vector<char> diskArr;
        examiner->pool->readRawData(chunkTreePhyAddr.device, chunkTreePhyAddr.offset, BtrfsHeader::SIZE_OF_HEADER, diskArr);
        BtrfsHeader *chunkHeader = new BtrfsHeader(examiner->endian, (uint8_t*)diskArr.data());

        uint64_t itemListStart = superBlk->getChunkLogAddr() + BtrfsHeader::SIZE_OF_HEADER;

        //TODO: pool for internal node
        const BtrfsNode* chunkTree;
        if(chunkHeader->isLeafNode())
            chunkRoot = new LeafNode(examiner->pool, chunkHeader, examiner->endian, itemListStart);
        else
            chunkRoot = new InternalNode(examiner->pool, chunkHeader, examiner->endian, itemListStart);

    }


    //!< Destructor
    ChunkTree::~ChunkTree()
    {
        if(chunkRoot != nullptr)
            delete chunkRoot;
    }


    //! Convert logical address to physical address.
    //!
    //! \param logicalAddr 64-bit logial address.
    //! \return 64-bit physical address.
    //!
    BTRFSPhyAddr ChunkTree::getPhysicalAddr(uint64_t logicalAddr)
    {
        BTRFSPhyAddr physAddr;

        //std::cout << chunkRoot->info() << std::endl;
        examiner->treeSearch(chunkRoot,
                [logicalAddr, &physAddr](const LeafNode* leaf)
                { return getPhyAddr(leaf, logicalAddr, physAddr); });

        return physAddr;
    }

    uint64_t ChunkTree::getChunkLogical(uint64_t logicalAddr) {

        uint64_t chunkLogical(0);

        examiner->treeSearch(chunkRoot,
                             [logicalAddr, &chunkLogical](const LeafNode* leaf)
                             { return getChunkLog(leaf, logicalAddr, chunkLogical); });

        return chunkLogical;
    }
}

