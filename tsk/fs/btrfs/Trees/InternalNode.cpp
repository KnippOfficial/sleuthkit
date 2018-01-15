//! \file
//! \author Shujian Yang
//!
//! Implementation of class InternalNode.

#include <sstream>
#include "InternalNode.h"
#include "../Examiners/Functions.h"
#include "../Basics/ChunkData.h"

namespace btrForensics{
    //! Constructor of btrfs leaf node.
    //!
    //! \param img Image file.
    //! \param header Pointer to header of a node.
    //! \param endian The endianess of the array.
    //! \param startOffset Offset of the node, right after header.
    //!
    InternalNode::InternalNode(BTRFS_POOL *pool, const BtrfsHeader *header,
            TSK_ENDIAN_ENUM endian, uint64_t startOffset)
        :BtrfsNode(header)
    {
        vector<char> diskArr;
        BTRFSPhyAddr physAddr;
        uint64_t itemOffset(0);
        uint32_t itemNum = header->getNumOfItems();

        //cerr << "DBG: Creating InternalNode" << endl;

        for(uint32_t i=0; i<itemNum; ++i){
            /*physAddr = getChunkAddr(startOffset + itemOffset, &chunkKey, &chunkData).at(0);
            pool->readRawData(physAddr.device, physAddr.offset, KeyPtr::SIZE_OF_KEY_PTR, diskArr);*/

            pool->readData(startOffset + itemOffset, KeyPtr::SIZE_OF_KEY_PTR, diskArr);

            keyPointers.push_back(new KeyPtr(pool, endian, (uint8_t*)diskArr.data()));

            itemOffset += KeyPtr::SIZE_OF_KEY_PTR;
        }
    }


    //! Destructor
    InternalNode::~InternalNode()
    {
        for(auto ptr : keyPointers)
            delete ptr;
    }


    //! Print info about this node.
    const std::string InternalNode::info() const
    {
        std::ostringstream oss;

        oss << *nodeHeader << "\n\n";
        oss << "Item list:" << '\n';
        oss << std::string(30, '=') << "\n\n";

        for(auto ptr : keyPointers){
            oss << *ptr;
            oss << std::string(30, '=') << "\n\n";
        }

        return oss.str();
    }

}
