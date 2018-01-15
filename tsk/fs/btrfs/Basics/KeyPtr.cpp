//! \file
//! \author Shujian Yang
//!
//! Implementation of class KeyPtr

#include "KeyPtr.h"
#include "../../../utils/ReadInt.h"
#include "../Trees/BtrfsNode.h"
#include "../Trees/LeafNode.h"
#include "../../../pool/BTRFS_POOL.h"


namespace btrForensics{

    //! Constructor of Btrfs key pointer.
    //!
    //! \param endian The endianess of the array.
    //! \param arr Byte array storing key pointer data.
    //!
    KeyPtr::KeyPtr(BTRFS_POOL* pool, TSK_ENDIAN_ENUM endian, uint8_t arr[])
        :key(endian, arr), childNode(nullptr)
    {
        int arIndex(BtrfsKey::SIZE_OF_KEY); //Key initialized already.
        blkNum = read64Bit(endian, arr + arIndex);
        arIndex += 0x08;

        generation = read64Bit(endian, arr + arIndex);
        arIndex += 0x08;

        vector<char> headerArr;
        pool->readData(blkNum, BtrfsHeader::SIZE_OF_HEADER, headerArr);
        BtrfsHeader *header = new BtrfsHeader(TSK_LIT_ENDIAN, (uint8_t*)headerArr.data());

        uint64_t itemOffset = getBlkNum() + BtrfsHeader::SIZE_OF_HEADER;

        if(header->isLeafNode()){
            childNode = new LeafNode(pool, header, endian, itemOffset);
        }
        else {
            childNode = new InternalNode(pool, header, endian, itemOffset);
        }
    }


    //!< Destructor
    KeyPtr::~KeyPtr()
    {
        if(childNode != nullptr)
            delete childNode;
    }


    //! Overloaded stream operator.
    std::ostream &operator<<(std::ostream &os, const KeyPtr &keyPtr)
    {
        os << keyPtr.key;
        os << "Block number: " << keyPtr.blkNum << '\n';
        os << "Generation: " << keyPtr.generation << '\n';

        return os;
    }

}
