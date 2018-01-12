/*
 * TSK_POOL_INFO.cpp
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file TSK_POOL_INFO.cpp
 * Generic class containing information about pool objects
 */

#include "TSK_POOL_INFO.h"
#include "ZFS_POOL.h"
#include "BTRFS_POOL.h"

TSK_POOL_INFO::TSK_POOL_INFO(TSK_ENDIAN_ENUM endian, std::string pathToFolder)
        : numberMembers(0), poolName(""), poolGUID(0), type(), unavailableMembers() {
    //TODO: recursive directory search
    //read all given volumes
    auto dir = opendir(pathToFolder.c_str());
    struct dirent *ent = nullptr;
    if (dir != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_type != DT_DIR) {
                std::string pathToFile = "";
                if (pathToFolder.back() == '/') {
                    pathToFile = (pathToFolder + ent->d_name);
                } else {
                    pathToFile = (pathToFolder + "/" + ent->d_name);
                }

                members[pathToFile] = (tsk_img_open(1, (const TSK_TCHAR *const *) &pathToFile, TSK_IMG_TYPE_DETECT, 0));
                if (members[pathToFile] == nullptr) {
                    tsk_error_print(stderr);
                    std::cerr << "Cannot open image " << pathToFile << "." << std::endl;
                    exit(1);
                }
                numberMembers += 1;
            }
        }
        closedir(dir);
    }
}

void TSK_POOL_INFO::displayAllMembers() {
    //TODO: display all members of a pool

}

TSK_POOL *TSK_POOL_INFO::createPoolObject() {
    TSK_POOL *pool = nullptr;

    try {
        pool = new ZFS_POOL(this);
        type = TSK_ZFS_POOL;
    } catch (...) {
        //cerr << "Not a ZFS pool!" << endl;
    }

    try {
        pool = new BTRFS_POOL(this);
        type = TSK_BTRFS_POOL;
    } catch (...) {
        //cerr << "Not a BTRFS pool!" << endl;
    }

    return pool;
}

void TSK_POOL_INFO::displayAllDevices() {
    for (auto it : members) {
        std::cout << it.first << " (" << it.second->size << ")" << std::endl;
    }
}

void TSK_POOL_INFO::readData(int device, TSK_OFF_T offset, std::vector<char> &buffer, size_t length) {
    auto it = this->members.begin();
    for (int i = 0; i < device; i++) {
        it++;
    }
    buffer.resize(length);
    tsk_img_read(it->second, offset, buffer.data(), length);
}

TSK_POOL_INFO::~TSK_POOL_INFO() {
    for (auto &m : members) {
        tsk_img_close(m.second);
    }
}

