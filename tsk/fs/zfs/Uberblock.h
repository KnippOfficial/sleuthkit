/*
 * Uberblock.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file Uberblock.h
 * Describes the Uberblock object in ZFS
 */

#ifndef ZFS_FTK_UBERBLOCK_H
#define ZFS_FTK_UBERBLOCK_H

#include <cstdint>
#include <iostream>
#include <memory>
#include <tsk/libtsk.h>
#include "../../utils/ReadInt.h"
#include "Blkptr.h"

class Uberblock{

private:
    uint64_t ub_magic;
    uint64_t ub_version;
    uint64_t ub_txg;
    uint64_t ub_timestamp;
    uint64_t ub_offset;
    Blkptr* ub_rootbp;
    ZFS_POOL* pool;
    TSK_ENDIAN_ENUM endian;

public:
    Uberblock(TSK_ENDIAN_ENUM endian, uint8_t data[], ZFS_POOL* pool);
    ~Uberblock();

    void setOffset(uint64_t offset) {this->ub_offset = offset; };
    uint64_t getTimestamp()const { return this->ub_timestamp; };
    uint64_t getTXG()const { return this->ub_txg; };
    uint64_t getOffset()const { return this->ub_offset; };
    Blkptr* getRootbp()const { return this->ub_rootbp; }

    static const int SIZE_UBERBLOCK= 0x400;
    static const int UB_MAGIC_BIG = 0x00bab10c;
};

#endif //ZFS_FTK_UBERBLOCK_H
