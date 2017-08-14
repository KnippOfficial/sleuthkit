/*
 * Uberblock.cpp
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file Uberblock.cpp
 * Describes the Uberblock object in ZFS
 */

#include "Uberblock.h"

using namespace std;

Uberblock::Uberblock(TSK_ENDIAN_ENUM endian, uint8_t *data, ZFS_POOL* pool) {
    this->endian = endian;
    this->pool = pool;
    this->ub_offset = 0;

    this->ub_magic = read64Bit(endian, data + 0);
    if (ub_magic != UB_MAGIC_BIG){
        throw(0);
    }

    this->ub_version = read64Bit(endian, data + 8);
    this->ub_txg = read64Bit(endian, data + 16);
    this->ub_timestamp = read64Bit(endian, data + 32);
    this->ub_rootbp = new Blkptr(endian, data + 40, this->pool);
}

Uberblock::~Uberblock() {
    delete this->ub_rootbp;
}
