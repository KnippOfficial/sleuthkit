//! \file
//! \author Shujian Yang
//!
//! Implementation of class FilesystemTree.

#include <map>
#include <string>
#include <sstream>
#include <iomanip>
#include <functional>
#include <vector>
#include <iterator>
#include "FilesystemTree.h"
#include "../Examiners/Functions.h"

using namespace std;
//using namespace std::placeholders;

namespace btrForensics {
    //! Constructor of tree analyzer.
    //!
    //! \param rootNode Root node of the root tree to be analyzed.
    //! \param rootItemId Root item id of the file system tree.
    //! \param treeExaminer Tree examiner used to analyze file system tree.
    //!
    FilesystemTree::FilesystemTree(const BtrfsNode* rootNode,
            uint64_t rootItemId, const TreeExaminer* treeExaminer)
            :examiner(treeExaminer)
    {
        const BtrfsItem* foundItem;
        const RootItem* rootItm;
        if(examiner->treeSearchById(rootNode, rootItemId,
                [&foundItem](const LeafNode* leaf, uint64_t targetId)
                { return searchForItem(leaf, targetId, ItemType::ROOT_ITEM, foundItem); })) {
            rootItm = static_cast<const RootItem*>(foundItem);
        }
        else {
            ostringstream oss;
            oss << "Filesystem tree pointed by root item " << rootItemId << " is not found.";
            throw FsDamagedException(oss.str());
        }

        uint64_t offset = rootItm->getBlockNumber();
        rootDirId = rootItm->getRootObjId();
        vector<char> headerArr;
        examiner->pool->readData(offset, BtrfsHeader::SIZE_OF_HEADER, headerArr);

        const BtrfsHeader *fileTreeHeader =
            new BtrfsHeader(TSK_LIT_ENDIAN, (uint8_t*)headerArr.data());

        //cout << "DBG: Created Filesystem Tree Header" << endl;

        uint64_t itemOffset = offset + BtrfsHeader::SIZE_OF_HEADER;

        if(fileTreeHeader->isLeafNode()){
            fileTreeRoot = new LeafNode(examiner->pool, fileTreeHeader, examiner->endian, itemOffset);
        }
        else {
            fileTreeRoot = new InternalNode(examiner->pool, fileTreeHeader, examiner->endian, itemOffset);
        }

        //cout << "DBG: Created Filesystem Tree Root Node" << endl;
    }


    //!< Destructor
    FilesystemTree::~FilesystemTree()
    {
        if(fileTreeRoot != nullptr)
            delete fileTreeRoot;
    }


    //! List all dir items in this tree.
    //!
    //! \param os Output stream where the infomation is printed.
    //!
    const void FilesystemTree::listDirItems(ostream &os) const
    {
        //treeTraverse(fileTreeRoot, bind(printLeafDir, _1, _2, ref(os)));

        //Choose Lamba over std::bind.
        //See "Effective Modern C++" Item 34.
        examiner->treeTraverse(fileTreeRoot,
                [&os](const LeafNode *leaf) { printLeafDir(leaf, os); });
    }


    //! List all directory items in a directory with given id.
    //!
    //! \param id Id of the target directory.
    //! \param dirFlag Whether to list directories.
    //! \param fileFlag Whether to list regular files.
    //! \param recursive Whether list items recursively in subdirectories.
    //! \param level Level used to determine number of "+"s, usually set as 0.
    //! \param os Output stream where the infomation is printed.
    //!
    const void FilesystemTree::listDirItemsById(uint64_t id, bool dirFlag, bool fileFlag,
        bool recursive, int level, std::ostream& os) const
    {

        //cout << "DBG: ListingDirItemsByID... " << endl;

        DirContent* dir = getDirContent(id);

        //cout << "DBG: Got DirContent... " << endl;

        if(dir == nullptr) {
            return;
        }

        for(auto child : dir->children) {
            if(child->getTargetType() != ItemType::INODE_ITEM)
                continue;
            if((fileFlag && child->type == DirItemType::REGULAR_FILE) 
                    || (dirFlag && child->type == DirItemType::DIRECTORY)) {
                if(level!=0) os << string(level, '+') << " ";
                os << child->type << '/' << child->type << " ";
                ostringstream oss;
                oss << dec << child->getTargetInode() << ':';
                os << setfill(' ') << setw(9) << left << oss.str();
                os << " " << child->getDirName() << endl;
            }
            if(recursive && child->type == DirItemType::DIRECTORY) {
                uint64_t newId = child->targetKey.objId;
                listDirItemsById(newId, dirFlag, fileFlag, recursive, level+1, os);
            }
        }
    }


    //! Locate the directory with give inode number.
    //!
    //! \param id Inode number of directory.
    //!
    DirContent* FilesystemTree::getDirContent(uint64_t id) const
    {
        const BtrfsItem* foundItem;
        if(examiner->treeSearchById(fileTreeRoot, id,
                [&foundItem](const LeafNode* leaf, uint64_t targetId)
                { return searchForItem(leaf, targetId, ItemType::INODE_ITEM, foundItem); })) {
            const InodeItem* rootInode = static_cast<const InodeItem*>(foundItem);

            examiner->treeSearchById(fileTreeRoot, id,
                [&foundItem](const LeafNode* leaf, uint64_t targetId)
                { return searchForItem(leaf, targetId, ItemType::INODE_REF, foundItem); });
            const InodeRef* rootRef = static_cast<const InodeRef*>(foundItem);

            vector<const BtrfsItem*> foundItems;
            examiner->treeSearchById(fileTreeRoot, id,
                [&foundItems](const LeafNode* leaf, uint64_t targetId)
                { return filterItems(leaf, targetId, ItemType::DIR_INDEX, foundItems); });
            
            return new DirContent(rootInode, rootRef, foundItems);
        }
        return nullptr;
    }


    //! List files in a directory and navigate to subdirectory.
    //!
    //! \param os Output stream where the infomation is printed.
    //! \param is Input stream telling which directory is the one to be read.
    //!
    const void FilesystemTree::explorFiles(std::ostream& os, istream& is) const
    {
        DirContent* dir = getDirContent(rootDirId);
        if(dir == nullptr) {
            os << "Root directory not found." << endl;
            return;
        }

        os << "Root directory content:\n" <<endl;
        os << *dir;

        os << "The following items are subvolumes or snapshots:" << endl;
        for(auto dirItem : dir->children) {
            if(dirItem->getTargetType() == ItemType::ROOT_ITEM) {
                ostringstream oss;
                os << "  \e(0\x74\x71\e(B" << dec;
                oss << "[" << dirItem->getTargetInode() << "]";
                os << setfill(' ') << setw(9) << oss.str();
                os << "  " << dirItem->getDirName() << '\n';
            }
        }
        os << endl;

        while(true) {
            uint64_t targetInode;
            map<int, int> dirList;
            string input;
            int inputId;

            int count(0);
            for(int i = 0; i < dir->children.size(); ++i) {
                if(dir->children[i]->type == DirItemType::DIRECTORY 
                        && dir->children[i]->getTargetType() == ItemType::INODE_ITEM)
                    dirList[++count] = i;
            }
            if(count == 0) {
                while(true) {
                    os << "No child directory is found.\n";
                    os << "(Enter 'r' to return to previous directory or 'q' to quit.)" << endl;
                    is >> input;
                    if(input == "q") return;
                    if(input == "r") break;
                }
                targetInode = dir->ref->itemHead->key.offset;
            }
            else{
                while(true) {
                    os << "Child directory with following inode numbers are found." << endl;
                    for(auto entry : dirList) {
                        DirItem* item = dir->children[entry.second];
                        os << "[" << dec << setfill(' ') << setw(2) << entry.first << "] "
                            << setw(7) << item->getTargetInode() << "   " << item->getDirName() << '\n';
                    }
                    os << endl;
                    os << "To visit a child directory, please enter its index in the list:\n";
                    os << "(Enter 'r' to return to previous directory or 'q' to quit.)" << endl;
                    is >> input;
                    
                    if(input == "q") return;
                    if(input == "r") {
                        targetInode = dir->ref->itemHead->key.offset;
                        break;
                    }
                    stringstream(input) >> inputId;
                    if(dirList.find(inputId) != dirList.end()) {
                        int target = dirList[inputId];
                        DirItem* targetItem = dir->children[target];
                        targetInode = targetItem->getTargetInode();
                        break;
                    }
                    os << "Wrong index, please enter a correct one.\n" << endl;
                } 
                os << endl;
            }

            DirContent* newDir = getDirContent(targetInode);
            delete dir;
            dir = newDir;

            if(newDir == nullptr) {
                os << "Error, Directory not found." << endl;
                return;
            }
            os << std::string(60, '=') << "\n\n";
            os << "Directory content:\n" <<endl;
            os << *dir;
        }
    }

    
    //! Read file content with given inode and save to current directory.
    //!
    //! \param id Inode number of the file to read.
    //! \return True if file is all successfully written.
    //!
    const bool FilesystemTree::readFile(uint64_t id) const
    {
        const BtrfsItem* foundItem;
        if(!examiner->treeSearchById(fileTreeRoot, id,
                [&foundItem](const LeafNode* leaf, uint64_t targetId)
                { return searchForItem(leaf, targetId, ItemType::INODE_ITEM, foundItem); }))
            return false;
        const InodeItem* inode = static_cast<const InodeItem*>(foundItem);
        uint64_t fileSize = inode->getSize();
            
        if(!examiner->treeSearchById(fileTreeRoot, id,
                [&foundItem](const LeafNode* leaf, uint64_t targetId)
                { return searchForItem(leaf, targetId, ItemType::INODE_REF, foundItem); }))
            return false;
        const InodeRef* inodeRef = static_cast<const InodeRef*>(foundItem);
        string fileName = inodeRef->getDirName();
            

        vector<const BtrfsItem*> foundExtents;
        examiner->treeSearchById(fileTreeRoot, id,
                [&foundExtents](const LeafNode* leaf, uint64_t targetId)
                { return filterItems(leaf, targetId, ItemType::EXTENT_DATA, foundExtents); });
        if(foundExtents.size() < 1)
            return false;
            
        uint64_t unreadSize = fileSize;
        //cout << "DBG: FILE SIZE: " << unreadSize << endl;
        BTRFSPhyAddr phyAddr;
        for(auto extent : foundExtents) {
            const ExtentData* data = static_cast<const ExtentData*>(extent);
            if(data->compression + data->encryption + data->otherEncoding != 0)
                return false;

            //ofstream ofs(fileName, ofstream::app | ofstream::binary);
            //if(!ofs) return false;
            if(data->type == 0) { //Is inline file.
                vector<char> dataArr;
                examiner->pool->readData(data->dataAddress, fileSize, dataArr);
                std::copy(dataArr.begin(), dataArr.end(), ostream_iterator<char>(std::cout));
                //ofs.write(dataArr.data(), fileSize);
                //ofs.close();
                return true;
            }
            else {
                vector<char> dataArr;
                if(unreadSize > data->numOfBytes) {
                    examiner->pool->readData(data->logicalAddress + data->extentOffset, data->numOfBytes, dataArr);
                    std::copy(dataArr.begin(), dataArr.end(), ostream_iterator<char>(std::cout));
                    //ofs.write(dataArr.data(), data->numOfBytes);
                    unreadSize -= data->numOfBytes;
                }
                else {
                    examiner->pool->readData(data->logicalAddress + data->extentOffset, unreadSize, dataArr);
                    std::copy(dataArr.begin(), dataArr.end(), ostream_iterator<char>(std::cout));
                    //ofs.write(dataArr.data(), unreadSize);
                    unreadSize = 0;
                }
                //ofs.close();
            }
        }
        if(unreadSize != 0) {
            throw FsDamagedException("File extents missing. Written file is incomplete.");
        }
        return true;
    }


    //! Print infomation about target inode.
    //!
    //! \param id Id of the target inode.
    //! \param os Output stream where the infomation is printed.
    //! \return True if inode is found.
    //! 
    const bool FilesystemTree::showInodeInfo(uint64_t id, std::ostream& os) const
    {
        const BtrfsItem* foundItem;
        if(!examiner->treeSearchById(fileTreeRoot, id,
                [&foundItem](const LeafNode* leaf, uint64_t targetId)
                { return searchForItem(leaf, targetId, ItemType::INODE_ITEM, foundItem); }))
            return false;
        const InodeItem* inode = static_cast<const InodeItem*>(foundItem);
        uint64_t size = inode->getSize();
            
        if(!examiner->treeSearchById(fileTreeRoot, id,
                [&foundItem](const LeafNode* leaf, uint64_t targetId)
                { return searchForItem(leaf, targetId, ItemType::INODE_REF, foundItem); }))
            return false;
        const InodeRef* inodeRef = static_cast<const InodeRef*>(foundItem);
        string name = inodeRef->getDirName();
/*
        os << dec;
        os << "Inode number: " << id << endl;
        os << "Size: " << size << endl;
        os << "Name: " << name << endl;

        os << "\nDirectory Entry Times(local);" << endl;
        os << inode->printTime() << endl;*/

        vector<const BtrfsItem*> foundExtents;
        examiner->treeSearchById(fileTreeRoot, id,
                                 [&foundExtents](const LeafNode* leaf, uint64_t targetId)
                                 { return filterItems(leaf, targetId, ItemType::EXTENT_DATA, foundExtents); });

        //TODO: get access to key for correct offset in file?
        uint64_t file_offset = 0;

        for(auto extent : foundExtents) {
            const ExtentData *data = static_cast<const ExtentData *>(extent);
            //cout << setw(10) << file_offset << " -" << setw(10) << (file_offset + data->extentSize) << " at logical offset 0x" << hex << (data->logicalAddress + data->extentOffset) << dec << endl;
            //file_offset += data->extentSize;
            cout << data->extentSize << endl;
        }
        
        return true;
    }
}
