/*
 * BTRFS_POOL.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file BTRFS_POOL.h
 * Creates a BTRFS_POOL object, subclass of TSK_POOL object
 */

#ifndef ZFS_FTK_BTRFS_POOL_H
#define ZFS_FTK_BTRFS_POOL_H

#include <string.h>
#include "TSK_POOL_INFO.h"
#include "TSK_POOL.h"
#include "BTRFS_DEVICE.h"
#include "../../tsk/fs/btrfs/Basics/Basics.h"

namespace btrForensics {
    class SuperBlock;
    class TreeExaminer;
    class ChunkItem;
};

class BTRFS_POOL : public TSK_POOL {

private:
    TSK_POOL_INFO *pool;
    bool initialized;
    vector<BTRFS_DEVICE*> devices;
    uint16_t no_all_devices;
    uint16_t no_available_devices;
    //TODO: maybe change type back to UUID
    string pool_guid;
    btrForensics::SuperBlock* superblock;

public:
    BTRFS_POOL(TSK_POOL_INFO *pool);
    ~BTRFS_POOL();
    void readData(uint64_t, uint64_t, vector<char>&, bool = true);
    void readRawData(int dev, uint64_t offset, uint64_t size, vector<char>& buffer);
    virtual void print(std::ostream& os) const;
    BTRFS_DEVICE* getDeviceByID(uint64_t) const;

    void fsstat(string dataset = "", int uberblock = -1);
    void fls(string dataset = "", int uberblock = -1);
    void istat(int object_number, string dataset = "", int uberblock = -1);
    void icat(int object_number, string dataset = "", int uberblock = -1);
    void displayChunkInformation() const;
    bool isChunkDataAvailable (const btrForensics::ChunkItem*) const;
    //TODO: wrap function around that
    btrForensics::TreeExaminer* examiner;

    vector<BTRFSPhyAddr> getPhysicalAddress(uint64_t);
    uint64_t getChunkLogicalAddr(uint64_t);


    string getPoolName();

    btrForensics::SuperBlock* getSuperblock() { return superblock;};


};

#endif //ZFS_FTK_BTRFS_POOL_H

