/*
 * TSK_POOL_INFO.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file TSK_POOL_INFO.h
 * Generic class containing information about pool objects
 */

#ifndef ZFS_FTK_TSK_POOL_INFO_H
#define ZFS_FTK_TSK_POOL_INFO_H
#include <tsk/libtsk.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <dirent.h>

enum TSK_POOL_TYPE {TSK_ZFS_POOL, TSK_BTRFS_POOL};

class TSK_POOL_INFO {

private:
    int numberMembers;
    std::string poolName;
    int poolGUID;
    TSK_POOL_TYPE type;
    std::vector<int> unavailableMembers;

public:
    TSK_POOL_INFO(TSK_ENDIAN_ENUM endian, std::string pathToFolder);
    ~TSK_POOL_INFO();

    std::map<std::string, TSK_IMG_INFO*> members;

    void detectPoolType();
    void displayAllDevices();
    void displayAllMembers();
    void readData(int device, TSK_OFF_T offset, std::vector<char>& buffer, size_t size);
};

#endif //ZFS_FTK_TSK_POOL_INFO_H
