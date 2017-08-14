/*
 * NVList.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file NVList.h
 * Describes the NVList object in ZFS
 */

#ifndef ZFS_FTK_NVLIST_H
#define ZFS_FTK_NVLIST_H

#include <iostream>
#include <tsk/libtsk.h>
#include <map>
#include "../../utils/ReadInt.h"

class NVList {

private:
    uint64_t beginningNV;
    std::map<std::string, uint64_t> elements_int;
    std::map<std::string, std::string> elements_string;
    std::map<std::string, NVList*> elements_nvlist;
    std::map<std::string, std::vector<NVList*>> elements_nvListArray;
    TSK_ENDIAN_ENUM endian;
    int nestedLevel;
    int size;

    void parseNVElement(uint8_t* data, int index, uint64_t length);

public:
    std::string getStringValue(std::string name);
    uint64_t getIntValue(std::string name);
    std::vector<NVList*> getNVListArray(std::string name);

    int getSize() { return size; };
    friend std::ostream& operator<<(std::ostream& os, const NVList& nvlist);
    NVList(TSK_ENDIAN_ENUM endian, uint8_t data[], int nestedLevel = 0);
    ~NVList();
};

#endif //ZFS_FTK_NVLIST_H
