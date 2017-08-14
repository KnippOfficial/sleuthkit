/*
 * ObjectSet.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file ObjectSet.h
 * Describes the ObjectSet object in ZFS
 */

#ifndef ZFS_FTK_OBJECTSET_H
#define ZFS_FTK_OBJECTSET_H

#include <cstdint>
#include <iostream>
#include <tsk/libtsk.h>
#include "../../pool/ZFS_POOL.h"

class Dnode;

class ObjectSet{

private:
    Dnode* metadnode;
    std::vector<Dnode*> dnodes;

public:
    ObjectSet(TSK_ENDIAN_ENUM endian, uint8_t data[], ZFS_POOL* pool);
    ~ObjectSet();

    Dnode* getDnode(uint64_t i)const;
};


#endif //ZFS_FTK_OBJECTSET_H
