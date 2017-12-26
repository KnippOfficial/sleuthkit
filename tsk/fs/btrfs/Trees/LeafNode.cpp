//! \file
//! \author Shujian Yang
//!
//! Implementation of class LeafNode.

#include <sstream>
#include "LeafNode.h"
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
    LeafNode::LeafNode(BTRFS_POOL *pool, const BtrfsHeader *header,
            TSK_ENDIAN_ENUM endian, uint64_t startOffset)
        :BtrfsNode(header)
    {
        char *diskArr;
        uint64_t itemOffset(0);
        uint32_t itemNum = header -> getNumOfItems();

        //TODO: can items (metadata in general) span across multiple stripes and thus multiple disks?!
        for(uint32_t i=0; i<itemNum; ++i){
            vector<char> diskArr;

            BTRFSPhyAddr temp = pool->getPhysicalAddress(startOffset + itemOffset);
            pool->readRawData(temp.device, temp.offset, ItemHead::SIZE_OF_ITEM_HEAD, diskArr);

            ItemHead *itemHead = new ItemHead(endian, (uint8_t*)diskArr.data(),
                                        startOffset, itemOffset);

            BtrfsItem *newItem = nullptr;
            vector<char> itmArr;
            uint64_t dataOffset = startOffset + itemHead->getDataOffset();
            temp = pool->getPhysicalAddress(dataOffset);
            pool->readRawData(temp.device, temp.offset, itemHead->getDataSize(), itmArr);

            switch(itemHead->key.itemType){
                case ItemType::INODE_ITEM:
                    newItem = new InodeItem(itemHead, TSK_LIT_ENDIAN, (uint8_t*)itmArr.data());
                    break;
                case ItemType::INODE_REF:
                    newItem = new InodeRef(itemHead, TSK_LIT_ENDIAN, (uint8_t*)itmArr.data());
                    break;
                case ItemType::DIR_ITEM: //Both types use the same structure.
                case ItemType::DIR_INDEX:
                    newItem = new DirItem(itemHead, TSK_LIT_ENDIAN, (uint8_t*)itmArr.data());
                    break;
                case ItemType::ROOT_ITEM:
                    newItem = new RootItem(itemHead, TSK_LIT_ENDIAN, (uint8_t*)itmArr.data());
                    break;
                case ItemType::ROOT_REF: //Both types use the same structure.
                case ItemType::ROOT_BACKREF:
                    newItem = new RootRef(itemHead, TSK_LIT_ENDIAN, (uint8_t*)itmArr.data());
                    break;
                case ItemType::CHUNK_ITEM:
                    newItem = new ChunkItem(itemHead, TSK_LIT_ENDIAN, (uint8_t*)itmArr.data());
                    break;
                case ItemType::EXTENT_DATA:
                    newItem = new ExtentData(itemHead, TSK_LIT_ENDIAN, (uint8_t*)itmArr.data(), dataOffset);
                    break;
                case ItemType::BLOCK_GROUP_ITEM:
                    newItem = new BlockGroupItem(itemHead, TSK_LIT_ENDIAN, (uint8_t*)itmArr.data());
                    break;
                case ItemType::EXTENT_ITEM:
                    newItem = new ExtentItem(itemHead, TSK_LIT_ENDIAN, (uint8_t*)itmArr.data());
                    break;
                default:
                    newItem = new UnknownItem(itemHead);
            }

            if(newItem != nullptr){
                itemList.push_back(newItem);
            }

            itemOffset += ItemHead::SIZE_OF_ITEM_HEAD;
        }
    }


    //! Destructor
    LeafNode::~LeafNode()
    {
        for(auto item : itemList)
            delete item;
    }


    //! Print info about this node.
    const std::string LeafNode::info() const
    {
        std::ostringstream oss;

        oss << *nodeHeader << "\n\n";
        oss << "Item list:" << '\n';
        oss << std::string(30, '=') << "\n\n";

        for(auto &item : itemList){
            oss << *item;
            oss << std::string(30, '=') << "\n\n";
        }

        return oss.str();
    }

}

