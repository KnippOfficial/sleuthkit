/*
 * ObjectSet.cpp
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file ObjectSet.cpp
 * Describes the ObjectSet object in ZFS
 */

#include "ObjectSet.h"
#include "Dnode.h"

using namespace std;

ObjectSet::ObjectSet(TSK_ENDIAN_ENUM endian, uint8_t *data, ZFS_POOL *pool) {

    metadnode = new Dnode(endian, data, pool);
    std::vector<char> dataDnode;
    metadnode->getData(dataDnode);

    for (int i = 0; i < int(dataDnode.size() / 512); ++i) {
        try {
            dnodes.push_back(new Dnode(endian, (uint8_t *) dataDnode.data() + (i * 512), pool));
        }
        catch (...) {
            dnodes.push_back(nullptr);
        }
    }
}

Dnode* ObjectSet::getDnode(uint64_t i) const {
    if(i < dnodes.size()) {
        return dnodes.at(i);
    } else
        return nullptr;
}

ObjectSet::~ObjectSet() {
    for (int j = 0; j < dnodes.size(); ++j) {
        delete dnodes.at(j);
    }

    delete this->metadnode;
}
