/*
 * ZFS_Functions.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file ZFS_Functions.h
 * Contains helper functions used for ZFS support
 */

#ifndef ZFS_FTK_ZFS_FUNCTIONS_H
#define ZFS_FTK_ZFS_FUNCTIONS_H
#include <iostream>
#include <memory>
#include <map>
#include <string>

class ObjectSet;
class ZAP;
class Dnode;

uint64_t getHeaddatasetID(ObjectSet* MOS, uint64_t directoryID);

uint64_t getRootdatasetDirectoryID(ObjectSet* MOS);

uint64_t getRootdatasetID(ObjectSet* MOS);

ZAP* getZAPFromDnode(Dnode* dnode);

auto getZapEntriesFromChildZapObject(Dnode *childZAPObject) -> std::map<std::string, uint64_t>;

void getChildDatasets(ObjectSet* MOS, uint64_t datasetDirectoryID, std::map<std::string, uint64_t>* datasets, std::string path);

void getSnapshots(ObjectSet* MOS, uint64_t datasetDirectoryID, std::map<std::string, uint64_t>* snapshots);

#endif //ZFS_FTK_ZFS_FUNCTIONS_H
