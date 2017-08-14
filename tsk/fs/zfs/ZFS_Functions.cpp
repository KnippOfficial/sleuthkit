/*
 * ZFS_Functions.cpp
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file ZFS_Functions.cpp
 * Contains helper functions used for ZFS support
 */

#include "ZFS_Functions.h"
#include "ObjectSet.h"
#include "Dnode.h"
#include "ZAP.h"

using namespace std;

uint64_t getHeaddatasetID(ObjectSet *MOS, uint64_t directoryID) {
    Dnode *rootDatasetDirectory = MOS->getDnode(directoryID);
    return rootDatasetDirectory->getBonusValue("dd_head_dataset");
}

uint64_t getRootdatasetDirectoryID(ObjectSet *MOS) {
    std::vector<char> objectDirectoryData;
    Dnode *objectDirectory = MOS->getDnode(1);
    objectDirectory->getData(objectDirectoryData);
    ZAP zap(TSK_LIT_ENDIAN, (uint8_t *) &objectDirectoryData[0], objectDirectoryData.size());
    return zap.getValue("root_dataset");

}

uint64_t getRootdatasetID(ObjectSet *MOS) {
    //object directory
    Dnode *objectDirectory = MOS->getDnode(1);
    ZAP *zap = getZAPFromDnode(objectDirectory);
    uint64_t root_dataset_directory_id = zap->getValue("root_dataset");
    return getHeaddatasetID(MOS, root_dataset_directory_id);
}

ZAP *getZAPFromDnode(Dnode *dnode) {
    std::vector<char> data;
    dnode->getData(data);
    return new ZAP(TSK_LIT_ENDIAN, (uint8_t *) data.data(), data.size());
}


void getChildDatasets(ObjectSet *MOS, uint64_t datasetDirectoryID, std::map<string, uint64_t> *datasets, string path) {
    Dnode *datasetDirectory = MOS->getDnode(datasetDirectoryID);
    uint64_t childDirID = datasetDirectory->getBonusValue("dd_child_dir_zapobj");
    auto childZAPObject = MOS->getDnode(childDirID);

    std::vector<char> childZAPObjectData;
    childZAPObject->getData(childZAPObjectData);
    char *start = &childZAPObjectData[0];
    ZAP zap = ZAP(TSK_LIT_ENDIAN, (uint8_t *) start, childZAPObjectData.size());
    auto entries = zap.entries;

    for (auto& it : entries) {
        if (it.first[0] != '$') {
            datasets->insert(std::pair<string, uint64_t>((path + "/" + it.first), it.second));
            getChildDatasets(MOS, it.second, datasets, (path + "/" + it.first));
        }
    }
}

auto getZapEntriesFromChildZapObject(Dnode *childZAPObject) -> std::map<string, uint64_t> {
    std::vector<char> childZAPObjectData;
    childZAPObject->getData(childZAPObjectData);
    auto start = childZAPObjectData.data();
    ZAP zap = ZAP(TSK_LIT_ENDIAN, (uint8_t *) start, childZAPObjectData.size());
    return zap.entries;
}

void getSnapshots(ObjectSet *MOS, uint64_t datasetDirectoryID, std::map<string, uint64_t> *snapshots) {
    uint64_t datasetID = getHeaddatasetID(MOS, datasetDirectoryID);
    auto dataset = MOS->getDnode(datasetID);
    auto snapnames = dataset->getBonusValue("ds_snapnames_zapobj");
    auto snapnamesZAPObject = MOS->getDnode(snapnames);

    std::vector<char> snapnamesZAPObjectData;
    snapnamesZAPObject->getData(snapnamesZAPObjectData);
    auto start = snapnamesZAPObjectData.data();
    ZAP zap = ZAP(TSK_LIT_ENDIAN, (uint8_t *) start, snapnamesZAPObjectData.size());

    for (auto& it : zap.entries) {
        snapshots->insert(std::pair<string, uint64_t>(it.first, it.second));
        //TODO: Recursive snapshots?!
        //getChildDatasets(MOS, it->second, datasets, (path + "/" + it->first));
    }
}
