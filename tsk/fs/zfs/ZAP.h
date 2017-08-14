/*
 * ZAP.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file ZAP.h
 * Describes a ZFS ZAP object
 */

#ifndef ZFS_FTK_ZAP_H
#define ZFS_FTK_ZAP_H
#include <iostream>
#include <map>
#include <string>
#include <tsk/libtsk.h>
#include <fstream>
#include <cstdint>
#include "utils/ReadInt.h"

class ZAP{

private:
    bool isDirectory;
    TSK_ENDIAN_ENUM endian;

    void parseFatLeaf(uint8_t* data);
    void parseFatLeafChunkEntry(uint8_t* data, int chunkOffset);
    void getChunkData(uint8_t* data, int offset, std::vector<char>* chunkData);

public:
    ZAP(TSK_ENDIAN_ENUM endian, uint8_t data[], size_t length, bool isDirectory=FALSE);
    ~ZAP() = default;

    std::map<std::string, uint64_t> entries;
    uint64_t getValue(std::string name);
    friend std::ostream& operator<<(std::ostream& os, const ZAP& microzap);
};


#endif //ZFS_FTK_ZAP_H
