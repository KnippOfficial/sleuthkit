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
    cerr << "ObjectSet::ObjectSet: metaDnode created: " << std::endl;
    cerr << *metadnode << std::endl;

    std::vector<char> dataDnode;
    metadnode->getData(dataDnode);


    cerr << "ObjectSet::ObjectSet: dataNode Size: " << dataDnode.size() << std::endl;
    cerr << "ObjectSet::ObjectSet: i: " <<int(dataDnode.size() / 512) << std::endl;
    // cerr << "ObjectSet::ObjectSet: dataDnode: " << dataDnode.data() << std::endl;

    for (int i = 0; i < int(dataDnode.size() / 512); ++i) {
        try {
            dnodes.push_back(new Dnode(endian, (uint8_t *) dataDnode.data() + (i * 512), pool));
        }
        catch (...) {
            //   cerr << "ObjectSet::ObjectSet: Error on i " << i << std::endl;
            dnodes.push_back(nullptr);
        }
    }
}

ObjectSet::ObjectSet(TSK_ENDIAN_ENUM endian, Dnode *metadnode, ZFS_POOL *pool) {

    std::vector<char> dataDnode;
    metadnode->getData(dataDnode);

    cerr << "ObjectSet::ObjectSet: dataNode Size: " << dataDnode.size() << std::endl;
    cerr << "ObjectSet::ObjectSet: i: " <<int(dataDnode.size() / 512) << std::endl;
    // cerr << "ObjectSet::ObjectSet: dataDnode: " << dataDnode.data() << std::endl;

    for (int i = 0; i < int(dataDnode.size() / 512); ++i) {
        Dnode* dnode = nullptr;
        ZAP *zap;
        try {
            dnode = new Dnode(endian, (uint8_t *) dataDnode.data() + (i * 512), pool);
            // cerr << "ObjectSet::ObjectSet: data was " << (uint8_t *) dataDnode.data() + (i * 512)<< std::endl;
            dnodes.push_back(dnode);
        }
        catch (...) {
            // cerr << "ObjectSet::ObjectSet: Error on i " << i << std::endl;
            // cerr << "ObjectSet::ObjectSet: data was " << (uint8_t *) dataDnode.data() + (i * 512) << std::endl;
            dnodes.push_back(nullptr);
        }
        // if (dnode != nullptr) {
            // cerr << "======================================================================================" << std::endl;
            // cerr << "ObjectSet::ObjectSet: Dnode in Objectset with ID  " << i << std::endl;
            // cerr << *dnode << std::endl;

            // cerr << "ObjectSet::ObjectSet: ZAP for Dnode in Objectset with ID: " << i << std::endl;
            // zap = getZAPFromDnode(dnode);
            // if (zap == nullptr) {
            //     cerr << "ObjectSet::ObjectSet: zap for directory from Dnode with id  " << i << " could not be created " << std::endl;
            //     return;
            // }
            // // cerr << "ObjectSet::ObjectSet: created zap for directory from Dnode with id  " << dnodeID << std::endl;
            // cerr << *zap << std::endl;
            // delete zap;
        // }
    }
}

Dnode* ObjectSet::getDnode(uint64_t i) const {
    if(i < dnodes.size()) {
        return dnodes.at(i);
    } else
        return nullptr;
}

int ObjectSet::getDnodesSize() {
    return dnodes.size();
}

ObjectSet::~ObjectSet() {
    for (int j = 0; j < dnodes.size(); ++j) {
        delete dnodes.at(j);
    }

    delete this->metadnode;
}
