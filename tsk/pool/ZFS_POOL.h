/*
 * ZFS_POOL.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file ZFS_POOL.h
 * Creates a ZFS_POOL object, subclass of TSK_POOL object
 */

#ifndef ZFS_FTK_ZFS_POOL_H
#define ZFS_FTK_ZFS_POOL_H

#include <tsk/libtsk.h>
#include <sys/stat.h>
#include <map>
#include <iterator>
#include "../fs/zfs/NVList.h"
#include "ZFS_VDEV.h"
#include "TSK_POOL_INFO.h"
#include "TSK_POOL.h"
#include "../fs/zfs/UberblockArray.h"
#include "../fs/zfs/ZFS_Functions.h"

class Blkptr;
class Uberblock;

class ZFS_POOL : public TSK_POOL {

private:
    vector<ZFS_VDEV*> vdevs;
    vector<uint64_t> availableIDs;
    uint64_t no_all_vdevs;
    uint64_t pool_guid;
    string name;
    bool reconstructable;
    TSK_POOL_INFO *pool;
    UberblockArray* uberblock_array;

public:
    ZFS_POOL(TSK_POOL_INFO *pool);
    ~ZFS_POOL();

    ZFS_VDEV* getVdevByID(uint64_t i);

    void checkReconstructable();
    void readData(Blkptr*, vector<char>& buffer, bool decompress=true);
    void readData(int, uint64_t, uint64_t, vector<char>& buffer);
    void readRawData(int dev, uint64_t offset, uint64_t size, vector<char>& buffer);
    Uberblock* getMostrecentUberblock() const { return uberblock_array->getMostrecent(); }
    UberblockArray* getUberblockArray() const { return uberblock_array;};
    friend std::ostream& operator<<(std::ostream& os, const ZFS_POOL& pool);
    virtual void print(std::ostream& os) const;
    void fsstat(string dataset = "", int uberblock = -1);
    void fls(string dataset = "", int uberblock = -1);
    void istat(int object_number, string dataset = "", int uberblock = -1);
    void icat(int object_number, string dataset = "", int uberblock = -1);
    void fwalk(string dataset = "", int uberblock = -1, string restorePath = "", bool debug = false);

    ObjectSet* getMOS(Uberblock* uberblock);
    std::map<string, uint64_t> getDatasets(ObjectSet* MOS);
    string getPoolName();
    ObjectSet* getObjectSetFromDnode(Dnode* dnode);
    void listFiles(ObjectSet* os, uint64_t dnodeID,  std::map<string, uint64_t> datasets, bool isDirectory, string path, int tabLevel=0);
    void fileWalk(ObjectSet *os, uint64_t dnodeID, std::map<string, uint64_t> datasets,
                string path, string restorepath = "", bool debug = false);
    void printDirectory(ObjectSet *fileSystem, string path, std::map<string, uint64_t> datasets, 
                uint64_t dnodeID, int tabLevel, bool debug = false);
    void restoreDirectory(ObjectSet *fileSystem, string path, std::map<string, uint64_t> datasets, 
                uint64_t dnodeID, string restorePath, bool debug);
    void retrieveFile(ObjectSet *fileSystem, uint64_t dnodeID, std::vector<char> &fileContent) ;

};

#endif //ZFS_FTK_ZFS_POOL_H

