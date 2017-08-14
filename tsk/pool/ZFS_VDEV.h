/*
 * ZFS_VDEV.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file ZFS_VDEV.h
 * Used to handle top-level virtual devices of ZFS.
 */

#ifndef ZFS_FTK_ZFS_VDEV_H
#define ZFS_FTK_ZFS_VDEV_H


#include <tsk/libtsk.h>
#include <iostream>
#include <vector>
#include "../fs/zfs/NVList.h"

using namespace std;

typedef struct ZFS_DEVICE{
    string type;
    string path;
    string accessedVia;
    uint64_t id;
    uint64_t guid;
    uint64_t create_txg;
    TSK_IMG_INFO* img;
    bool available;
} ZFS_DEVICE;

typedef struct RAIDZ_COL{
    uint64_t devid;
    uint64_t offset;
    uint64_t size;
} RAIDZ_COL;

class ZFS_VDEV {

private:
    vector<ZFS_DEVICE> children;
    string type;
    uint64_t pool_guid;
    uint64_t guid;
    uint64_t id;
    uint64_t noActiveChildren;
    uint64_t nparity;
    bool usable;
    int create_txg;

public:
     ZFS_VDEV(NVList* list);
    ~ZFS_VDEV() = default;

    uint64_t getID() { return this->id; };
    uint64_t getNoChildren() { return children.size(); };
    uint64_t getNoActiveChildren() { return noActiveChildren; };
    uint64_t getParity() { return nparity; };
    string getType() { return this->type; };
    bool isUsable() { return this->usable; };
    friend std::ostream& operator<<(std::ostream& os, const ZFS_VDEV& vdev);
    bool addDevice(uint64_t, std::pair<string, TSK_IMG_INFO*>);
    ZFS_DEVICE getChild(uint64_t child);
    void checkUsable();
};


#endif //ZFS_FTK_ZFS_VDEV_H
