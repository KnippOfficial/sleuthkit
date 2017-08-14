/*
 * Dnode.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file Dnode.h
 * Describes the Dnode object in ZFS
 */

#ifndef ZFS_FTK_DNODE_H
#define ZFS_FTK_DNODE_H

#include <iostream>
#include <map>
#include <tsk/libtsk.h>
#include <fstream>
#include "Blkptr.h"
#include "../../utils/ReadInt.h"
#include "ZFS_Structures.h"
#include "../../pool/ZFS_POOL.h"

class Dnode{

private:
    dmu_object_type_t dn_type;
    uint8_t dn_nlevels;
    uint8_t dn_nblkptr;
    uint8_t dn_bonustype;
    uint16_t dn_bonuslen;
    std::vector<Blkptr*> dn_blkptr;
    Blkptr* dn_bonus_blkptr;
    TSK_ENDIAN_ENUM endian;
    ZFS_POOL* pool;

    void generateBonus(TSK_ENDIAN_ENUM endian, uint8_t * data);

public:
    Dnode(TSK_ENDIAN_ENUM endian, uint8_t data[], ZFS_POOL* pool);
    ~Dnode();

    void getData(std::vector<char>& data);
    dmu_object_type_t getType() {return dn_type;};
    uint64_t getBonusValue(string name);
    Blkptr* getBonusBlkptr();
    std::map<string, uint64_t> dn_bonus;

    friend std::ostream& operator<<(std::ostream& os, const Dnode& dnode);
};

#endif //ZFS_FTK_DNODE_H
