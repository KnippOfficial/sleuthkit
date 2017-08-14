/*
 * UberblockArray.cpp
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file UberblockArray.cpp
 * Describes an array of Uberblocks in ZFS
 */

#include "UberblockArray.h"

using namespace std;

UberblockArray::UberblockArray(TSK_ENDIAN_ENUM endian, uint8_t *data, ZFS_POOL *pool) {
    int highestTXG = 0;

    //check all 128 possible uberblocks
    for (int i = 0; i < 128; i++) {
        try {
            uberblocks[i] = new Uberblock(endian, data + i * 0x400, pool);
            uberblocks[i]->setOffset(START_UBERBLOCKARRAY + i * 0x400);
            if (uberblocks[i]->getTXG() > highestTXG) {
                highestTXG = uberblocks[i]->getTXG();
                mostRecent = uberblocks[i];
            };
        }
        catch (...) {
            //TODO: keep uberblocks with corrupted block pointers for information
            uberblocks[i] = nullptr;
        }
    }
}

Uberblock *UberblockArray::getByTXG(int txg) const {
    for (int i = 0; i < 128; i++) {
        if ((uberblocks[i] != nullptr) && (uberblocks[i]->getTXG() == txg))
            return uberblocks[i];
    }

    return NULL;
}

ostream &operator<<(ostream &os, const UberblockArray &UberblockArray) {
    for (int i = 0; i < 128; i++) {
        if(UberblockArray.uberblocks[i] != NULL){
            os << setw(3) << i << ". Uberblock: " << hex << setw(10) << UberblockArray.uberblocks[i]->getOffset() <<  dec << "\t/\t"
                 << timestampToDateString(UberblockArray.uberblocks[i]->getTimestamp())
                 << "\t/\t TXG:" << UberblockArray.uberblocks[i]->getTXG() << endl;
        }
    }
    return os;
}

UberblockArray::~UberblockArray() {
    for (int i = 0; i < 128; i++) {
        delete uberblocks[i];
    }
}
