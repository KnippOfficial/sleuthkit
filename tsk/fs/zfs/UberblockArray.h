/*
 * UberblockArray.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file UberblockArray.h
 * Describes an array of Uberblocks in ZFS
 */

#ifndef ZFS_FTK_UBERBLOCKARRAY_H
#define ZFS_FTK_UBERBLOCKARRAY_H

#include <iostream>
#include <iomanip>
#include <tsk/libtsk.h>
#include "Uberblock.h"

class ZFS_POOL;

class UberblockArray{

private:
    Uberblock* uberblocks[128];
    Uberblock* mostRecent;

public:
    UberblockArray(TSK_ENDIAN_ENUM endian, uint8_t data[], ZFS_POOL* pool);
    ~UberblockArray();
    Uberblock* getMostrecent()const { return mostRecent; }
    Uberblock* getByTXG(int txg)const;

    friend std::ostream& operator<<(std::ostream& os, const UberblockArray& uberblockarray);
    static const int START_UBERBLOCKARRAY= 0x20000;
    static const int SIZE_UBERBLOCKARRAY= 0x400 * 128;

};


#endif //ZFS_FTK_UBERBLOCKARRAY_H
