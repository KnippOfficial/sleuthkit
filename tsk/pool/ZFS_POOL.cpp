/*
 * ZFS_POOL.cpp
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file ZFS_POOL.cpp
 * Creates a ZFS_POOL object, subclass of TSK_POOL object
 */

#include <string.h>
#include "ZFS_POOL.h"
#include "../fs/zfs/ObjectSet.h"
#include "../fs/zfs/Dnode.h"
#include "../fs/zfs/ZAP.h"

ZFS_POOL::ZFS_POOL(TSK_POOL_INFO *pool)
        : vdevs(), availableIDs(), no_all_vdevs(0), pool_guid(0), name(""), reconstructable(true), pool(pool),
          uberblock_array(nullptr) {
    NVList *list = nullptr;
    NVList *vdev_tree = nullptr; //part of the list containing information about subtree
    std::vector<char> diskData(112 * 1024);
    uint64_t current_guid = 0;
    bool initialized = false;

    //init for ZFS pool
    for (auto &it : pool->members) {
        tsk_img_read(it.second, 0x4004, diskData.data(), (112 * 1024));
        try {
            list = new NVList(TSK_BIG_ENDIAN, (uint8_t *) diskData.data());
        }
        catch (...) {
            throw "No valid ZFS Pool";
        }

        current_guid = list->getIntValue("guid");
        vdev_tree = list->getNVListArray("vdev_tree").at(0);

        //initialize using first image (take its pool_guid and number of top-level vdevs)
        if (!initialized) {
            no_all_vdevs = list->getIntValue("vdev_children");
            pool_guid = list->getIntValue("pool_guid");
            name = list->getStringValue("name");
            initialized = true;
        }

        //if vdev not yet created
        auto temp = getVdevByID(vdev_tree->getIntValue("id"));
        if (temp == nullptr) {
            temp = new ZFS_VDEV(list);
            vdevs.push_back(temp);
        }
        delete list;

        if (!temp->addDevice(current_guid, it)) {
            cerr << it.first << " with GUID " << current_guid << " is already in the pool!" << endl;
        }
        else {
            // cerr << "ZFS_POOL::ZFS_POOL: " << it.first << " with GUID " << current_guid << " added to pool!" << endl;
        }
    }
    checkReconstructable();

    //get uberblock array
    //TODO: multiple uberblock arrays from available disks
    //until then: get the array with the most recent block
    //get any child that is available
    int highestTXG = 0;
    UberblockArray* temp_array = nullptr;
    for (int i = 0; i < vdevs.size(); i++){
        pool->readData(vdevs.at(i)->getID(), UberblockArray::START_UBERBLOCKARRAY, diskData, UberblockArray::SIZE_UBERBLOCKARRAY);
        uberblock_array = new UberblockArray(TSK_LIT_ENDIAN, (uint8_t *) diskData.data(), this);
        if(uberblock_array->getMostrecent()->getTXG() > highestTXG){
            delete temp_array;
            temp_array = uberblock_array;
            highestTXG = uberblock_array->getMostrecent()->getTXG();
        }
    }
    uberblock_array = temp_array;
}

ZFS_POOL::~ZFS_POOL() {
    for (auto it : vdevs) {
        delete it;
    }
    delete uberblock_array;
}

void ZFS_POOL::checkReconstructable() {
    //first: check if all top-level vdevs are usable
    for (int i = 0; i < vdevs.size(); i++) {
        if (!vdevs.at(i)->isUsable()) {
            reconstructable = false;
            cout << vdevs.at(i)->getType() << " top-level vdev with ID " << vdevs.at(i)->getID() << " is unavailable!"
                 << endl;
        } else {
            availableIDs.push_back(vdevs.at(i)->getID());
        }
    }

    //second: check if all top-level vdevs are present (e.g. missing single disk/file)
    if (no_all_vdevs > vdevs.size()) {
        reconstructable = false;
        cerr << "Missing " << (no_all_vdevs - vdevs.size()) << " top-level vdev(s)!" << endl;
    }
}

ZFS_VDEV *ZFS_POOL::getVdevByID(uint64_t id) {
    for (int i = 0; i < vdevs.size(); i++) {
        if (vdevs.at(i)->getID() == id)
            return vdevs.at(i);
    }
    return nullptr;
}

void ZFS_POOL::readRawData(int dev, uint64_t offset, uint64_t size, vector<char> &buffer) {
    pool->readData(dev, offset, buffer, size);
}

void ZFS_POOL::readData(int tvdev_id, uint64_t offset, uint64_t length, vector<char> &buffer) {
    //get corresponding tvdev
    ZFS_VDEV *vdev = getVdevByID(tvdev_id);
    string vdev_type = vdev->getType();

    buffer.resize(length);

    if (vdev_type == "file" || vdev_type == "disk") {
        int s = tsk_img_read(vdev->getChild(0).img, (offset+0x400000), buffer.data(), length);
    } else if (vdev_type == "mirror") {
        //get any child that is available
        ZFS_DEVICE temp;
        for (int i = 0; i < vdev->getNoChildren(); i++) {
            temp = vdev->getChild(i);
            if (temp.available) {
                tsk_img_read(temp.img, (offset+0x400000), buffer.data(), length);
                break;
            }
        }
    } else if (vdev_type == "raidz") {
        // algorithm based on: https://github.com/zfsonlinux/zfs/blob/master/module/zfs/vdev_raidz.c
        std::vector<RAIDZ_COL> dataCols;
        RAIDZ_COL temp;
        uint8_t unit_shift = 9;
        uint64_t dcols = vdev->getNoChildren();
        uint64_t nparity = vdev->getParity();

        uint64_t b = offset >> unit_shift;
        uint64_t s = length >> unit_shift;
        uint64_t f = b % dcols;
        uint64_t o = (b / dcols) << unit_shift;
        uint64_t q, r, c, bc, col, acols, scols, coff, devidx, tot, rc_size;
        uint64_t off = 0;
        uint64_t asize = 0;

        q = s / (dcols - nparity);
        r = s - q * (dcols - nparity);
        bc = (r == 0 ? 0 : r + nparity);
        tot = s + nparity * (q + (r == 0 ? 0 : 1));

        if (q == 0) {
            acols = bc;
            scols = MIN(dcols, roundup(bc, nparity + 1));
        } else {
            acols = dcols;
            scols = dcols;
        }

        uint64_t rm_cols = acols;
        uint64_t rm_scols = scols;
        uint64_t rm_bigcols = bc;
        uint64_t rm_skipstart = bc;
        uint64_t rm_missingdata = 0;
        uint64_t rm_missingparity = 0;
        uint64_t rm_firstdatacol = nparity;
        uint64_t rm_datacopy = 0;

        for (c = 0; c < scols; c++) {
            col = f + c;
            coff = o;
            if (col >= dcols) {
                col -= dcols;
                coff += 1ULL << unit_shift;
            }

            if (c >= acols)
                rc_size = 0;
            else if (c < bc)
                rc_size = (q + 1) << unit_shift;
            else
                rc_size = q << unit_shift;

            temp.devid = col;
            temp.offset = coff;
            temp.size = rc_size;
            dataCols.push_back(temp);

            asize += rc_size;
        }

        if (rm_firstdatacol == 1 && (offset & (1ULL << 20))) {
            devidx = dataCols.at(0).devid;
            o = dataCols.at(0).offset;
            dataCols.at(0).devid= dataCols.at(1).devid;
            dataCols.at(0).offset= dataCols.at(1).offset;
            dataCols.at(1).devid  = devidx;
            dataCols.at(1).offset = o;

            if (rm_skipstart == 0)
                rm_skipstart = 1;
        }

        buffer.resize(0);
        vector<char> diskData;
        for (c = rm_firstdatacol; c < dataCols.size(); c++){
            diskData.resize(dataCols.at(c).size);
            tsk_img_read(vdev->getChild(dataCols.at(c).devid).img, (dataCols.at(c).offset+0x400000),
                                 diskData.data(), dataCols.at(c).size);
            buffer.insert(buffer.end(), diskData.data(), diskData.data() + dataCols.at(c).size);
        }
    } else {
        cerr << "vdev type '" << vdev_type << "' is not supported!" << endl;
    }
}

void ZFS_POOL::readData(Blkptr *blkptr, vector<char> &buffer, bool decompress) {
    ZFS_VDEV* vdev = nullptr;
    dva goodDVA;

    for (int i = 0; i < 3 && vdev == nullptr; i++) {
        vdev = this->getVdevByID(blkptr->getDVA(i).vdev);
        goodDVA = blkptr->getDVA(i);
    }

    if(vdev == nullptr){
        cerr << "No top-level vdev of DVA available!" << endl;
        uint64_t size = blkptr->getPsize();
        uint64_t size_decompressed = blkptr->getLsize();
        buffer.resize(size);
        if (buffer.size() != size_decompressed && decompress) {
            buffer.resize(size_decompressed);
        }
        //fill missing blocks with zeros
        std::fill(buffer.begin(), buffer.end(), 0);
        return;
    }

    uint64_t size = blkptr->getPsize();
    uint64_t size_decompressed = blkptr->getLsize();
    string vdev_type = vdev->getType();

    this->readData(goodDVA.vdev, goodDVA.offset, size, buffer);

    //decompress
    if (buffer.size() != size_decompressed && decompress) {
        vector<char> v(buffer);
        buffer.resize(size_decompressed);
        uint32_t bufferLength = read32Bit(TSK_BIG_ENDIAN, (uint8_t *) v.data());
        LZ4_decompress_safe(v.data() + 4, buffer.data(), bufferLength, size_decompressed);
    }
}

void ZFS_POOL::print(std::ostream &os) const {
    os << "Name: " << name << endl;
    os << "Pool GUID: " << pool_guid << endl;
    os << "Number of top-level vdevs: " << no_all_vdevs << " (" << vdevs.size() << " detected)" << endl;
    //print toplevel vdevs
    for (int i = 0; i < vdevs.size(); i++) {
        os << *vdevs.at(i);
    }

    os << *uberblock_array << endl;
}

ostream &operator<<(ostream &os, const ZFS_POOL &pool) {
    os << "Name: " << pool.name << endl;
    os << "Pool GUID: " << pool.pool_guid << endl;
    os << "Number of top-level vdevs: " << pool.no_all_vdevs << " (" << pool.vdevs.size() << " detected)" << endl;
    //print toplevel vdevs
    for (int i = 0; i < pool.vdevs.size(); i++) {
        os << *pool.vdevs.at(i);
    }

    return os;
}

//util functions for handling zpools

ObjectSet* ZFS_POOL::getMOS(Uberblock *uberblock) {
    //read MOS block pointer
    vector<char> diskData;;
    Blkptr *mos_ptr = uberblock->getRootbp();
    this->readData(mos_ptr, diskData);
    return new ObjectSet(TSK_LIT_ENDIAN, (uint8_t *) diskData.data(), this);
}

std::map<string, uint64_t> ZFS_POOL::getDatasets(ObjectSet *MOS) {
    std::map<string, uint64_t> datasets;

    auto rootDatasetDirectory = MOS->getDnode(getRootdatasetDirectoryID(MOS));
    auto childDirID = rootDatasetDirectory->getBonusValue("dd_child_dir_zapobj");
    auto entries = getZapEntriesFromChildZapObject(MOS->getDnode(childDirID));
    auto path = this->getPoolName();
    datasets.insert(std::pair<string, uint64_t>(path, getRootdatasetDirectoryID(MOS)));

    for (auto& it : entries) {
        if (it.first[0] != '$') {
            datasets.insert(std::pair<string, uint64_t>((path + "/" + it.first), it.second));
            getChildDatasets(MOS, it.second, &datasets, (path + "/" + it.first));
        }
    }

    return datasets;
}

string ZFS_POOL::getPoolName() {
    // extracts the pool name from the NV list at the beginning
    vector<char> diskData;
    this->readRawData(0, 0x4004, (112 * 1024), diskData);
    NVList list(TSK_BIG_ENDIAN, (uint8_t *) diskData.data());

    return list.getStringValue("name");
}

ObjectSet* ZFS_POOL::getObjectSetFromDnode(Dnode *dnode) {
    std::vector<char> datasetData;
    dnode->getData(datasetData);
    cerr << "ZFS_POOL::getObjectSetFromDnode: Dnode data size: " << datasetData.size() << std::endl;
    return new ObjectSet(TSK_LIT_ENDIAN, (uint8_t *) datasetData.data(), this);
    // return new ObjectSet(TSK_LIT_ENDIAN, dnode, this);
}

//TODO: get img pointer from object set
void ZFS_POOL::listFiles(ObjectSet *os, uint64_t dnodeID, std::map<string, uint64_t> datasets, bool isDirectory,
               string path, int tabLevel) {
    ZAP *zap;
    ObjectSet *MOS = os;
    ObjectSet *nextObjectSet = NULL;
    char tab = ' ';

    cerr << "ZFS_POOL::listFiles: processing path " << path << " which is directory: " << isDirectory << std::endl;

    if (!isDirectory) {
        Dnode *dataset = os->getDnode(dnodeID);
        
        cerr << "ZFS_POOL::listFiles: found Dnode dataset with id " << dnodeID << " and blkptr_size " << dataset->getBlkptrSize() << std::endl;
          cerr << *dataset << std::endl;

        nextObjectSet = this->getObjectSetFromDnode(dataset);
        cerr << "ZFS_POOL::listFiles: created ObjectSet with size " << nextObjectSet->getDnodesSize() << std::endl;

        Dnode *masterNode = nextObjectSet->getDnode(1);
        cerr << "ZFS_POOL::listFiles: found Dnode masterNode with id 1 and blkptr_size " << masterNode->getBlkptrSize() << std::endl;
        cerr << *masterNode << std::endl;
        
        zap = getZAPFromDnode(masterNode);

        cerr << "ZFS_POOL::listFiles: created zap from masternode" << std::endl;
        cerr << *zap << std::endl;
        
        uint64_t root_directory_id = zap->getValue("ROOT");
        delete zap;

        Dnode *rootDirectory = nextObjectSet->getDnode(root_directory_id);
        zap = getZAPFromDnode(rootDirectory);

        cerr << "ZFS_POOL::listFiles: created zap from ROOT" << std::endl;
        cerr << *zap << std::endl;
        

    } else {
        Dnode *dnode = os->getDnode(dnodeID);
        cerr << "ZFS_POOL::listFiles: found Dnode with id " << dnodeID << " and blkptr_size " << dnode->getBlkptrSize() << std::endl;
        // cerr << *dnode << std::endl;

        zap = getZAPFromDnode(dnode);
        
        if (zap == nullptr) {
            cerr << "ZFS_POOL::listFiles: zap for directory from Dnode with id  " << dnodeID << " could not be created " << std::endl;
            return;
        }
        cerr << "ZFS_POOL::listFiles: created zap for directory from Dnode with id  " << dnodeID << std::endl;
        cerr << *zap << std::endl;
        nextObjectSet = os;
    }

    delete zap;
    return;

    for (auto& it : zap->entries) {
        uint64_t value = it.second;
        uint8_t type = *(((uint8_t *) &(it.second)) + 7);

        // Type = file
        if (type == 0x80) {
            cout << std::string(tabLevel * 4, tab) << "|---" << it.first << " : " << unsigned(value) << std::endl;
        }
        
        // Type = dataset / snapshot or directory
        else if (type == 0x40) {

            //for snapshots, this will never be the case (good to avoid inconsistencies)
            if (datasets.find(path + "/" + it.first) == datasets.end()) {
                cout << std::string(tabLevel * 4, tab) << "|---" << it.first << " : " << unsigned(value)
                     << " (Directory)" << std::endl;
                this->listFiles(nextObjectSet, unsigned(value), datasets, true, (path + "/" + it.first),
                          tabLevel + 1);
            } else {
                cout << std::string(tabLevel * 4, tab) << "|---" << it.first << " : " << unsigned(value)
                     << " (Dataset)" << std::endl;
                this->listFiles(MOS, getHeaddatasetID(MOS, datasets[path + "/" + it.first]), datasets, FALSE,
                          (path + "/" + it.first), tabLevel + 1);
            }
        } 
        
        // Type = anything else
        else {
            cout << it.first << " : " << unsigned(value) << "(" << unsigned(type) << ")" << std::endl;
        }
    }
    if(!isDirectory) {
        delete nextObjectSet;
    }
    delete zap;
}

void ZFS_POOL::printDirectory(ObjectSet *fileSystem, string path, std::map<string, uint64_t> datasets, 
            uint64_t dnodeID, int tabLevel, bool debug) {

    Dnode *directory = fileSystem->getDnode(dnodeID);
    if (directory == nullptr) {
        cerr << "Dnode with ID " << dnodeID << " not found in Object-Set" << std::endl;
        return;
    }

    if (debug) cerr << *directory << std::endl;
    ZAP* zap = getZAPFromDnode(directory);

    if (zap == nullptr) {
        cerr << "Could not create ZAP for Dnode with id " << dnodeID << std::endl;
        return;
    }
    char tab = ' ';

    for (auto& it : zap->entries) {

        // Important: split in value and type
        uint64_t value = unsigned(it.second);
        uint8_t type = *(((uint8_t *) &(it.second)) + 7);

        // Type = file
        if (type == 0x80) {
            cout << std::string(tabLevel * 4, tab) << "|---" << it.first << " : " << value << std::endl;
        }
        
        // Type = dataset / snapshot or directory
        else if (type == 0x40) {
             //for snapshots, this will never be the case (good to avoid inconsistencies)
            if (datasets.find(path + "/" + it.first) == datasets.end()) {
                cout << std::string(tabLevel * 4, tab) << "|---" << it.first << " (Directory) : " << unsigned(value) << std::endl;
                this->printDirectory(fileSystem, path, datasets, value, tabLevel + 1);
            } else {
                cout << std::string(tabLevel * 4, tab) << "|---" << it.first << " (Dataset) : " << unsigned(value) << std::endl;
            }
        } 
        
        // Type = anything else
        else {
            cout << it.first << " : " << value << "(" << unsigned(type) << ")" << std::endl;
        }
    }
    delete zap;
}

void ZFS_POOL::restoreDirectory(ObjectSet *fileSystem, string path, std::map<string, uint64_t> datasets, 
        uint64_t dnodeID, string restorePath, bool debug) {

    Dnode *directory = fileSystem->getDnode(dnodeID);
    if (directory == nullptr) {
        cerr << "Dnode with ID " << dnodeID << " not found in Object-Set" << std::endl;
        return;
    }
    // cerr << *directory << std::endl;
    ZAP* zap = getZAPFromDnode(directory);
    char tab = ' ';

    // cerr << *zap << std::endl;

    
    if (zap == nullptr) {
        cerr << "ZFS_POOL::restoreDirectory: Could not create ZAP for Dnode with id " << dnodeID << std::endl;
        return;
    }
    

    // Create restore directory
    struct stat st;
    if(stat(restorePath.c_str(),&st) == 0) {
        if( st.st_mode & (S_IFDIR != 0))
            cerr << "ZFS_POOL::restoreDirectory: " << restorePath << " already exists!" << endl;
            return;
    }
    else if(mkdir(restorePath.c_str(), 0777) == -1) {
         cerr << "ZFS_POOL::restoreDirectory: Could not create restoreDirectory " << restorePath << std::endl;
         return;
    }
    cout << "ZFS_POOL::restoreDirectory: Created directory " << restorePath << std::endl;

    for (auto& it : zap->entries) {

        // Important: split in value and type
        uint64_t value = unsigned(it.second);
        uint8_t type = *(((uint8_t *) &(it.second)) + 7);

        // Type = file
        if (type == 0x80) {
            cout << "ZFS_POOL::restoreDirectory:  Restoring file " << it.first << " to directory " << restorePath << std::endl;
            std::vector<char> fileContent;
            this->retrieveFile(fileSystem, value, fileContent);
            ofstream write ((restorePath + "/" + it.first));
            std::copy(fileContent.begin(), fileContent.end(), ostream_iterator<char>(write));
            write.close();
            // std::copy(fileContent.begin(), fileContent.end(), ostream_iterator<char>(std::cout));
        }
        
        // Type = dataset / snapshot or directory
        else if (type == 0x40) {
            restorePath = restorePath + "/" + it.first;
             //for snapshots, this will never be the case (good to avoid inconsistencies)
            if (datasets.find(path + "/" + it.first) == datasets.end()) {
               this->restoreDirectory(fileSystem, path, datasets, value, restorePath, debug);
            } else {
                // create dir
                cout << "ZFS_POOL::restoreDirectory:  Created directory for dataset " << it.first << std::endl;       
                if(mkdir(restorePath.c_str(), 0777) == -1) {
                    cerr << "ZFS_POOL::restoreDirectory: Could not create Directory " << restorePath << std::endl;
                }
            }
        } 
        
        // Type = anything else
        else {
            // do nothing
        }
    }
    delete zap;
}

void ZFS_POOL::retrieveFile(ObjectSet *fileSystem, uint64_t dnodeID, std::vector<char> &fileContent) {
    Dnode* file = fileSystem->getDnode(dnodeID);
    if (file == nullptr) {
        cerr << "object with number " << dnodeID << " does not exist!" << endl;
        return;
    } else {
        std::vector<char> data;
        file->getData(data);

        auto it = file->dn_bonus.find("zp_size");
        if(it != file->dn_bonus.end() and file->getType() != DMU_OT_DIRECTORY_CONTENTS)
            fileContent.insert(fileContent.end(), data.begin(), (data.begin() + file->dn_bonus["zp_size"]));
        else
            fileContent.insert(fileContent.end(), data.begin(), data.end());
    }
}




void ZFS_POOL::fileWalk(ObjectSet *os, uint64_t dnodeID, std::map<string, uint64_t> datasets,
               string path, string restorepath, bool debug) {
    ZAP *zap;
    ObjectSet *MOS = os;
    ObjectSet *nextObjectSet = NULL;
    
    if (debug) cerr << "ZFS_POOL::fileWalk: processing path " << path << std::endl;

    // DATASET Dnode
    Dnode *dataset = os->getDnode(dnodeID);
    
    if (debug) cerr << "======================================================================================" << std::endl;
    if (debug) cerr << "ZFS_POOL::fileWalk: found Dnode dataset with id " << dnodeID << " and blkptr_size " << dataset->getBlkptrSize() << std::endl;
    if (debug) cerr << *dataset << std::endl;

    std::vector<char> datasetData;
    dataset->getData(datasetData);

    // MetaDnode Dnode
    Dnode* metadnode = new Dnode(TSK_LIT_ENDIAN, (uint8_t *) datasetData.data(), this);
    
    if (debug) cerr << "======================================================================================" << std::endl;
    if (debug) cerr << "ZFS_POOL::fileWalk: metaDnode created: " << std::endl;
    if (debug) cerr << *metadnode << std::endl;

    // ObjectSet for fileSystem
    ObjectSet* fileSystem = new ObjectSet(TSK_LIT_ENDIAN, metadnode, this);

    if (debug) cerr << "ZFS_POOL::fileWalk: created fileSystem ObjectSet" << std::endl;
    
    // MasterNode of filesystem
    Dnode *masterNode = fileSystem->getDnode(1);
    if (debug) cerr << "======================================================================================" << std::endl;
    if (debug) cerr << "ZFS_POOL::fileWalk: found Dnode masterNode" << std::endl;
    if (debug) cerr << *masterNode << std::endl;
    
    zap = getZAPFromDnode(masterNode);

    if (zap == nullptr) {
        cerr << "ZFS_POOL::fileWalk: cannot create zap from masterNode" << std::endl;
        return;
    }
    else {
        if (debug) cerr << "======================================================================================" << std::endl;
        if (debug) cerr << "ZFS_POOL::fileWalk: created zap from masternode" << std::endl;
        if (debug) cerr << *zap << std::endl;
    }

    // Follow MasterNode -> ROOT directory
    uint64_t root_directory_id = zap->getValue("ROOT");
    delete zap;

    if (restorepath == "") {
        this->printDirectory(fileSystem, path, datasets, root_directory_id, 0, debug);
    }
    else {
        this->restoreDirectory(fileSystem, path, datasets, root_directory_id, restorepath, debug);
    }
}


//file system specific functions

void ZFS_POOL::fsstat(string str_dataset, int uberblock) {

    Uberblock *usedUberblock;

    if(uberblock == -1) {
        usedUberblock = this->getMostrecentUberblock();
    } else {
        usedUberblock = this->getUberblockArray()->getByTXG(uberblock);
        cerr << "Using TXG: " << uberblock << endl;
    }

    ObjectSet *MOS = this->getMOS(usedUberblock);
    //parse object set and create MOS
    std::map<string, uint64_t> datasets = this->getDatasets(MOS);
    uint64_t rootDatasetDirectoryID = getRootdatasetDirectoryID(MOS);

    if (str_dataset != "") {
        bool snapshot = false;
        std::string str_snapshot = "";

        if(string(str_dataset).find('@') != string::npos){
            snapshot = true;
            str_snapshot = string(str_dataset).substr(string(str_dataset).find('@')+1);
            str_dataset = string(str_dataset).substr(0, string(str_dataset).find('@'));
        }

        if (datasets.find(str_dataset) == datasets.end())
            cerr << "No dataset named " << str_dataset << " found!" << endl;
        else {
            if(not snapshot){
                Dnode *dataset = MOS->getDnode(getHeaddatasetID(MOS,datasets.find(str_dataset)->second));
                cout << "Creation Time: \t" << timestampToDateString(dataset->getBonusValue("ds_creation_time")) << endl;
                cout << "Creation TXG: \t" << dataset->getBonusValue("ds_creation_txg") << endl;
                cout << "FSID GUID: \t" << dataset->getBonusValue("ds_fsid_guid") << endl;
                cout << "Parent Dir.: \t" << dataset->getBonusValue("ds_dir_obj") << endl;
                //cout << "Number of Children: \t" << dataset->getBonusValue("ds_num_children") << endl;
                cout << "Used Bytes: \t" << dataset->getBonusValue("ds_used_bytes") << endl;

                std::map<string, uint64_t> snapshots;
                getSnapshots(MOS, datasets.find(str_dataset)->second, &snapshots);
                if(snapshots.size() > 0){
                    cout << endl << "Snapshots of dataset: " << str_dataset << endl;
                    cout << "-----------------------------------------------" << endl;
                    for ( auto it = snapshots.begin(); it != snapshots.end(); it++ )
                    {
                        cout << str_dataset << "@" << it->first << " ---> " << it->second << endl;
                    }
                }
                cout << endl;

            }
            else if(snapshot){
                std::map<string, uint64_t> snapshots;
                getSnapshots(MOS, datasets.find(str_dataset)->second, &snapshots);
                if (snapshots.find(str_snapshot) == snapshots.end())
                    cout << "Dataset " << str_dataset << " does not have a snapshot named " << str_snapshot << "!" << endl;
                else{
                    Dnode *dataset = MOS->getDnode(snapshots.find(str_snapshot)->second);
                    cout << "Creation Time: \t" << timestampToDateString(dataset->getBonusValue("ds_creation_time")) << endl;
                    cout << "Creation TXG: \t" << dataset->getBonusValue("ds_creation_txg") << endl;
                    cout << "FSID GUID: \t" << dataset->getBonusValue("ds_fsid_guid") << endl;
                    //cout << "ID of most recent snapshot: \t" << dataset->getBonusValue("ds_next_snap_obj") << endl;
                    cout << "Used Bytes: \t" << dataset->getBonusValue("ds_used_bytes") << endl;
                }
            }
        }
    } else {
        std::map<string, uint64_t>::const_iterator it;
        for (it = datasets.begin(); it != datasets.end(); it++) {
            cout << it->first << " / " << it->second << " (" << getHeaddatasetID(MOS, it->second) << ")" << endl;
        }
    };
}

void ZFS_POOL::fls(string str_dataset, int uberblock){
    Uberblock *usedUberblock;

    if(uberblock == -1) {
        usedUberblock = this->getMostrecentUberblock();
        cerr << "Using TXG: " << usedUberblock << endl;
    } else {
        usedUberblock = this->getUberblockArray()->getByTXG(uberblock);
        cerr << "Using TXG: " << uberblock << endl;
    }

    ObjectSet *MOS = this->getMOS(usedUberblock);
    std::map<string, uint64_t> datasets = this->getDatasets(MOS);
    uint64_t rootDatasetDirectoryID = getRootdatasetDirectoryID(MOS);

    if (str_dataset != "") {
        bool snapshot = false;
        std::string str_snapshot = "";

        if(string(str_dataset).find('@') != string::npos){
            snapshot = true;
            str_snapshot = string(str_dataset).substr(string(str_dataset).find('@')+1);
            str_dataset = string(str_dataset).substr(0, string(str_dataset).find('@'));
        }

        if (datasets.find(str_dataset) == datasets.end()) {
            cerr << "No dataset named " << str_dataset << " found!" << endl;
        } else {
            if (!snapshot) {
                listFiles(MOS, getHeaddatasetID(MOS, datasets.find(str_dataset)->second), datasets, FALSE,
                           str_dataset);
            }
            if (snapshot) {
                std::map<string, uint64_t> snapshots;
                getSnapshots(MOS, datasets.find(str_dataset)->second, &snapshots);
                if (snapshots.find(str_snapshot) == snapshots.end()) {
                    cerr << "Dataset " << str_dataset << " does not have a snapshot named " << str_snapshot << "!"
                         << endl;
                } else {
                   listFiles(MOS, snapshots.find(str_snapshot)->second, datasets, FALSE, str_dataset);
                }
            }
        }
    } else {
        fileWalk(MOS, getHeaddatasetID(MOS, rootDatasetDirectoryID), datasets, this->getPoolName(), "");
    }

    delete MOS;
}

void ZFS_POOL::fwalk(string str_dataset, int uberblock, string restorePath, bool debug){
    Uberblock *usedUberblock;

    if(uberblock == -1) {
        usedUberblock = this->getMostrecentUberblock();
        if (debug) cerr << "Using TXG: " << usedUberblock << endl;
    } else {
        usedUberblock = this->getUberblockArray()->getByTXG(uberblock);
         if (debug) cerr << "Using TXG: " << uberblock << endl;
    }

     if (debug) cerr << "Using restorePath: " << restorePath << endl;

    ObjectSet *MOS = this->getMOS(usedUberblock);
    std::map<string, uint64_t> datasets = this->getDatasets(MOS);
    uint64_t rootDatasetDirectoryID = getRootdatasetDirectoryID(MOS);

    if (str_dataset != "") {
        bool snapshot = false;
        std::string str_snapshot = "";

        if(string(str_dataset).find('@') != string::npos){
            snapshot = true;
            str_snapshot = string(str_dataset).substr(string(str_dataset).find('@')+1);
            str_dataset = string(str_dataset).substr(0, string(str_dataset).find('@'));
        }

        if (datasets.find(str_dataset) == datasets.end()) {
            cerr << "No dataset named " << str_dataset << " found!" << endl;
        } else {
            if (!snapshot) {
                fileWalk(MOS, getHeaddatasetID(MOS, datasets.find(str_dataset)->second), datasets, 
                            str_dataset, restorePath, debug);
            }
            if (snapshot) {
                std::map<string, uint64_t> snapshots;
                getSnapshots(MOS, datasets.find(str_dataset)->second, &snapshots);
                if (snapshots.find(str_snapshot) == snapshots.end()) {
                    cerr << "Dataset " << str_dataset << " does not have a snapshot named " << str_snapshot << "!"
                         << endl;
                } else {
                    // TODO currenly not tested for snapshots
                    // fileWalk(MOS, snapshots.find(str_snapshot)->second, datasets, 
                    //         str_dataset, restorePath);
                }
            }
        }
    } else {
        fileWalk(MOS, getHeaddatasetID(MOS, rootDatasetDirectoryID), datasets, this->getPoolName(), restorePath, debug);
    }

    delete MOS;
}

void ZFS_POOL::istat(int object_number, string str_dataset, int uberblock) {
    Uberblock *usedUberblock;

    if(uberblock == -1) {
        usedUberblock = this->getMostrecentUberblock();
    } else {
        usedUberblock = this->getUberblockArray()->getByTXG(uberblock);
        cerr << "Using TXG: " << uberblock << endl;
    }

    ObjectSet *MOS = this->getMOS(usedUberblock);
    std::map<string, uint64_t> datasets = this->getDatasets(MOS);

    if (str_dataset == ""){
        str_dataset = this->name;
    }

    std::string str_snapshot = "";
    Dnode *dnode = nullptr;

    if(str_dataset == "MOS"){
        dnode = MOS->getDnode(object_number);
    }
    else{
        bool snapshot = false;
        if (str_dataset.find('@') != string::npos) {
            snapshot = true;
            str_snapshot = string(str_dataset).substr(string(str_dataset).find('@')+1);
            str_dataset = string(str_dataset).substr(0, string(str_dataset).find('@'));
        }

        if (datasets.find(str_dataset) == datasets.end()) {
            cerr << "No dataset named " << str_dataset << " found!" << endl;
            return;
        } else {
            if (!snapshot) {
                Dnode *datasetDnode = MOS->getDnode(getHeaddatasetID(MOS, datasets.find(str_dataset)->second));
                ObjectSet *dataset = this->getObjectSetFromDnode(datasetDnode);
                dnode = dataset->getDnode(object_number);
            }
            if (snapshot) {
                std::map<string, uint64_t> snapshots;
                getSnapshots(MOS, datasets.find(str_dataset)->second, &snapshots);
                if (snapshots.find(str_snapshot) == snapshots.end()) {
                    cerr << "Dataset " << str_dataset << " does not have a snapshot named " << str_snapshot << "!"
                         << endl;
                    return;
                } else {
                    Dnode *datasetDnode = MOS->getDnode(snapshots.find(str_snapshot)->second);
                    ObjectSet *dataset = this->getObjectSetFromDnode(datasetDnode);
                    dnode = dataset->getDnode(object_number);
                }
            }
        }
    }

    if (dnode == nullptr) {
        cerr << "object with number " << object_number << " does not exist!" << endl;
    } else {
        cout << *dnode << endl;
    }

    delete MOS;
}

void ZFS_POOL::icat(int object_number, string str_dataset, int uberblock) {
    Uberblock *usedUberblock;

    if(uberblock == -1) {
        usedUberblock = this->getMostrecentUberblock();
    } else {
        usedUberblock = this->getUberblockArray()->getByTXG(uberblock);
        cerr << "Using TXG: " << uberblock << endl;
    }

    ObjectSet *MOS = this->getMOS(usedUberblock);
    std::map<string, uint64_t> datasets = this->getDatasets(MOS);
    uint64_t rootDatasetDirectoryID = getRootdatasetDirectoryID(MOS);

    if (str_dataset == ""){
        str_dataset = this->name;
    }

    std::string str_snapshot = "";
    Dnode *dnode = nullptr;

    if(str_dataset == "MOS"){
        dnode = MOS->getDnode(object_number);
    }
    else{
        bool snapshot = false;
        if (str_dataset.find('@') != string::npos) {
            snapshot = true;
            str_snapshot = string(str_dataset).substr(string(str_dataset).find('@')+1);
            str_dataset = string(str_dataset).substr(0, string(str_dataset).find('@'));
        }

        if (datasets.find(str_dataset) == datasets.end()) {
            cerr << "No dataset named " << str_dataset << " found!" << endl;
            return;
        } else {
            if (!snapshot) {
                Dnode *datasetDnode = MOS->getDnode(getHeaddatasetID(MOS, datasets.find(str_dataset)->second));
                ObjectSet *dataset = this->getObjectSetFromDnode(datasetDnode);
                dnode = dataset->getDnode(object_number);
            }
            if (snapshot) {
                std::map<string, uint64_t> snapshots;
                getSnapshots(MOS, datasets.find(str_dataset)->second, &snapshots);
                if (snapshots.find(str_snapshot) == snapshots.end()) {
                    cerr << "Dataset " << str_dataset << " does not have a snapshot named " << str_snapshot << "!"
                         << endl;
                    return;
                } else {
                    Dnode *datasetDnode = MOS->getDnode(snapshots.find(str_snapshot)->second);
                    ObjectSet *dataset = this->getObjectSetFromDnode(datasetDnode);
                    dnode = dataset->getDnode(object_number);
                }
            }
        }
    }

    if (dnode == nullptr) {
        cerr << "object with number " << object_number << " does not exist!" << endl;
        return;
    } else {
        std::vector<char> data;
        dnode->getData(data);

        auto it = dnode->dn_bonus.find("zp_size");
        if(it != dnode->dn_bonus.end() and dnode->getType() != DMU_OT_DIRECTORY_CONTENTS)
            std::copy(data.begin(), (data.begin() + dnode->dn_bonus["zp_size"]), ostream_iterator<char>(std::cout));
        else
            std::copy(data.begin(), data.end(), ostream_iterator<char>(std::cout));
    }
}
