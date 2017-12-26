/*
 * BTRFS_POOL.cpp
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file BTRFS_POOL.cpp
 * Creates a BTRFS_POOL object, subclass of TSK_POOL object
 */

#include <string.h>
#include <cmath>
#include <iomanip>
#include "BTRFS_POOL.h"
#include "../../tsk/utils/ReadInt.h"
#include "../../tsk/utils/Uuid.h"
#include "../../tsk/fs/btrfs/Trees/SuperBlock.h"
#include "../../tsk/fs/btrfs/Trees/Trees.h"
#include "../../tsk/fs/btrfs/Examiners/Examiners.h"

using namespace btrForensics;

using namespace std;

BTRFS_POOL::BTRFS_POOL(TSK_POOL_INFO *pool)
        : initialized(false), no_all_devices(0), no_available_devices(0), pool(pool) {

    std::vector<char> diskData(SuperBlock::SIZE_OF_SPR_BLK);
    SuperBlock *supblk = nullptr;
    BTRFS_DEVICE *dev = nullptr;
    examiner = nullptr;

    for (auto &it : pool->members) {
        //cout << it.first << endl;
        tsk_img_read(it.second, SuperBlock::ADDR_OF_SPR_BLK, diskData.data(), SuperBlock::SIZE_OF_SPR_BLK);
        //TODO: Try, catch
        supblk = new SuperBlock(TSK_LIT_ENDIAN, (uint8_t *) diskData.data());

        if (!initialized) {
            no_all_devices = supblk->getNumDevices();
            pool_guid = supblk->getDevData().getFSUUID().encode();
            superblock = new SuperBlock(TSK_LIT_ENDIAN, (uint8_t *) diskData.data());
            initialized = true;
        }

        //TODO: check if device is part of pool
        dev = new BTRFS_DEVICE(supblk->getDevData().getID(), supblk->getDevData().getUUID().encode(),
                               it.second, 0, true);
        //cerr << "DBG: Added device with id " << dev->getID() << endl;
        devices.push_back(dev);
        no_available_devices++;
        delete supblk;
    }

    //initialize global treeexaminer for BTRFS_POOL
    //TODO: rootFsId is currently fixed
    //cout << "DBG: Creating TreeExaminer" << endl;
    uint64_t rootFsId = 0;
    if (rootFsId == 0)
        examiner = new TreeExaminer(this, TSK_LIT_ENDIAN, superblock);
    else
        examiner = new TreeExaminer(this, TSK_LIT_ENDIAN, superblock, rootFsId);

    //cout << "DBG: Created TreeExaminer" << endl;
    examiner->initializeRootTree(superblock);
    examiner->initializeFSTree();
}


BTRFS_POOL::~BTRFS_POOL() {
    for (auto it : devices) {
        delete it;
    }
    delete superblock;
}

void BTRFS_POOL::readData(uint64_t logical_addr, uint64_t size, vector<char> &buffer) {
    BTRFSPhyAddr physicalAddr;
    uint64_t offset = logical_addr;
    vector<char> stripeData;
    buffer.resize(0);
    //TODO: variable stripe_length !!!
    uint64_t stripe_length = 65536;
    uint64_t num_stripes = (uint64_t) ceil(size / stripe_length);

    uint64_t size_left = size;
    uint64_t size_for_stripe = 0;
    uint64_t chunk_logical = examiner->getChunkLogicalAddr(logical_addr);

    //printf("input_log %d | chunk_log %d | size %d | str_len %d | num_stripes %d\n", offset, chunk_logical, size,
    //       stripe_length, num_stripes);

    for (int i = 0; i <= num_stripes; i++) {
        size_for_stripe = stripe_length - ((offset - chunk_logical) % stripe_length);
        if (size_for_stripe > size_left) {
            size_for_stripe = size_left;
        }
        //cout << "DBG: SIZE FOR STRIPE: " << size_for_stripe << endl;
        physicalAddr = examiner->getPhysicalAddr(offset);
        readRawData(physicalAddr.device, physicalAddr.offset, size_for_stripe, stripeData);
        buffer.insert(buffer.end(), stripeData.data(), stripeData.data() + size_for_stripe);
        size_left -= size_for_stripe;
        offset += size_for_stripe;
    }
}

void BTRFS_POOL::readRawData(int dev_id, uint64_t offset, uint64_t size, vector<char> &buffer) {
    BTRFS_DEVICE *dev = getDeviceByID(dev_id);
    //cerr << "DBG (readRawData):: Looking for ID " << dev_id << endl;
    if (dev == nullptr) {
        cout << dev_id << endl;
        throw "Device with ID does not exist!";
    }
    //cout << dev->getID() << " | " << dev->getGUID() << endl;
    buffer.resize(size);
    //cout << dev->getImg()->size << endl;
    tsk_img_read(dev->getImg(), offset, buffer.data(), size);
}

BTRFSPhyAddr BTRFS_POOL::getPhysicalAddress(uint64_t logical_address) {
    //TODO: initialized check function for examiner
    if (examiner == nullptr) {
        BtrfsKey chunkKey = superblock->getChunkKey();
        ChunkData chunkData = superblock->getChunkData();
        return getChunkAddr(logical_address, &chunkKey, &chunkData);
    } else {
        return examiner->getPhysicalAddr(logical_address);
    }
}

BTRFS_DEVICE *BTRFS_POOL::getDeviceByID(uint64_t id) {
    for (int i = 0; i < devices.size(); i++) {
        if (devices.at(i)->getID() == id)
            return devices.at(i);
    }
    return nullptr;
}

void BTRFS_POOL::print(std::ostream &os) const {
    os << "FSID: " << pool_guid << endl;
    os << "Number of devices/stripes: " << no_all_devices << " (" << no_available_devices << " detected)" << endl;
    os << "-------------------------------------------------" << endl;
    for (auto &it : devices) {
        os << "ID: " << it->getID() << (it->getAvailable() ? " [X]" : " [O]") << endl;
        os << "GUID: " << it->getGUID() << endl;
        os << endl;
    }
}


string BTRFS_POOL::getPoolName() {

    return "N/A";
}

//file system specific functions

void BTRFS_POOL::fsstat(string str_dataset, int uberblock) {
    cout << *superblock << endl;
    cout << superblock->printSpace() << endl;
    cout << endl;
    cout << "Label: " << superblock->printLabel() << endl;

    vector<const BtrfsItem *> foundRootRefs;
    examiner->treeTraverse(examiner->rootTree, [&foundRootRefs](const LeafNode *leaf) {
        filterItems(leaf, ItemType::ROOT_BACKREF, foundRootRefs);
    });

    if (foundRootRefs.size() == 0) {
        cout << "\nNo subvolumes or snapshots are found.\n" << endl;
    }

    cout << "The following subvolumes or snapshots are found:" << endl;
    for (auto item : foundRootRefs) {
        const RootRef *ref = static_cast<const RootRef *>(item);
        cout << dec << setfill(' ') << setw(7);
        cout << ref->getId() << "   " << ref->getDirName() << '\n';
    }
}

void BTRFS_POOL::fls(string str_dataset, int uberblock) {
    uint64_t fsTreeID = 0;
    if (str_dataset != "") {
        vector<const BtrfsItem *> foundRootRefs;
        examiner->treeTraverse(examiner->rootTree, [&foundRootRefs](const LeafNode *leaf) {
            filterItems(leaf, ItemType::ROOT_BACKREF, foundRootRefs);
        });

        for (auto item : foundRootRefs) {
            const RootRef *ref = static_cast<const RootRef *>(item);
            if (ref->getDirName() == str_dataset) {
                fsTreeID = ref->getId();
            }
        }

        if (fsTreeID == 0) {
            cerr << "Could not find subvolume/snapshot named " << str_dataset << " using root instead!" << endl;
        } else {
            examiner->reInitializeFSTree(fsTreeID);
        }
    }

    uint64_t targetId = examiner->fsTree->rootDirId;
    examiner->fsTree->listDirItemsById(targetId, true, true, true, 0, cout);
}


void BTRFS_POOL::istat(int object_number, string str_dataset, int uberblock) {
    uint64_t fsTreeID = 0;
    if (str_dataset != "") {
        vector<const BtrfsItem *> foundRootRefs;
        examiner->treeTraverse(examiner->rootTree, [&foundRootRefs](const LeafNode *leaf) {
            filterItems(leaf, ItemType::ROOT_BACKREF, foundRootRefs);
        });

        for (auto item : foundRootRefs) {
            const RootRef *ref = static_cast<const RootRef *>(item);
            if (ref->getDirName() == str_dataset) {
                fsTreeID = ref->getId();
            }
        }

        if (fsTreeID == 0) {
            cerr << "Could not find subvolume/snapshot named " << str_dataset << " using root instead!" << endl;
        } else {
            examiner->reInitializeFSTree(fsTreeID);
        }
    }

    uint64_t targetId = examiner->fsTree->rootDirId;

    examiner->fsTree->showInodeInfo(object_number, cout);
}

void BTRFS_POOL::icat(int object_number, string str_dataset, int uberblock) {
    uint64_t fsTreeID = 0;
    if (str_dataset != "") {
        vector<const BtrfsItem *> foundRootRefs;
        examiner->treeTraverse(examiner->rootTree, [&foundRootRefs](const LeafNode *leaf) {
            filterItems(leaf, ItemType::ROOT_BACKREF, foundRootRefs);
        });

        for (auto item : foundRootRefs) {
            const RootRef *ref = static_cast<const RootRef *>(item);
            if (ref->getDirName() == str_dataset) {
                fsTreeID = ref->getId();
            }
        }

        if (fsTreeID == 0) {
            cerr << "Could not find subvolume/snapshot named " << str_dataset << " using root instead!" << endl;
        } else {
            examiner->reInitializeFSTree(fsTreeID);
        }
    }

    uint64_t targetId = examiner->fsTree->rootDirId;

    examiner->fsTree->readFile(object_number);
}