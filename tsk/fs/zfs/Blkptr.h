/*
 * Blkptr.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file Blkptr.h
 * Describes the Blkptr object in ZFS
 */

#ifndef ZFS_FTK_BLKPTR_H
#define ZFS_FTK_BLKPTR_H

#include <iostream>
#include <tsk/libtsk.h>
#include <string>
#include <bitset>
#include <fstream>
#include <sstream>
#include <memory>
#include "utils/ReadInt.h"
#include "utils/Tools.h"
#include "utils/lz4.h"
#include "Structs.h"

class ZFS_POOL;
class Blkptr{

private:
    dva blk_dva[3];
    uint64_t blk_lsize;
    uint64_t blk_psize;
    uint64_t blk_asize;
    uint8_t blk_compression;
    uint64_t blk_checksum[4];
    uint8_t blk_checksum_type;
    uint64_t embedded;
    TSK_ENDIAN_ENUM endian;
    ZFS_POOL* pool;
    uint64_t blk_calculated_checksum[4];
    std::vector<char> ownData;
    bool dataAvailable;

    bool checkChecksum();

public:
    Blkptr(TSK_ENDIAN_ENUM endian, uint8_t data[], ZFS_POOL* pool);
    ~Blkptr() = default;

    void getData(std::vector<char>& dataVector, int level=1, bool decompress=true);
    std::stringstream level0Blocks(int level);

    friend std::ostream& operator<<(std::ostream& os, const Blkptr& blkptr);
    dva getDVA(int i)const {return this->blk_dva[i];}
    uint64_t getLsize()const {return this->blk_lsize;}
    uint64_t getPsize()const {return this->blk_psize;}

};

#endif //ZFS_FTK_BLKPTR_H
