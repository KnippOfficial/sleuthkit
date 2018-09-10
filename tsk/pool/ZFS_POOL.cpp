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
    return new ObjectSet(TSK_LIT_ENDIAN, (uint8_t *) datasetData.data(), this);
}

//TODO: get img pointer from object set
void ZFS_POOL::listFiles(ObjectSet *os, uint64_t dnodeID, std::map<string, uint64_t> datasets, bool isDirectory,
               string path, int tabLevel) {
    ZAP *zap;
    ObjectSet *MOS = os;
    ObjectSet *nextObjectSet = NULL;
    char tab = ' ';

    if (!isDirectory) {
        Dnode *dataset = os->getDnode(dnodeID);
        nextObjectSet = this->getObjectSetFromDnode(dataset);
        Dnode *masterNode = nextObjectSet->getDnode(1);
        zap = getZAPFromDnode(masterNode);
        uint64_t root_directory_id = zap->getValue("ROOT");
        delete zap;

        Dnode *rootDirectory = nextObjectSet->getDnode(root_directory_id);
        zap = getZAPFromDnode(rootDirectory);

    } else {
        Dnode *dnode = os->getDnode(dnodeID);
        zap = getZAPFromDnode(dnode);
        nextObjectSet = os;
    }

    for (auto& it : zap->entries) {
        uint64_t value = it.second;
        uint8_t type = *(((uint8_t *) &(it.second)) + 7);
        if (type == 0x80) {
            cout << std::string(tabLevel * 4, tab) << "|---" << it.first << " : " << unsigned(value) << std::endl;
        } else if (type == 0x40) {
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
        } else {
            cout << it.first << " : " << unsigned(value) << "(" << unsigned(type) << ")" << std::endl;
        }
    }
    if(!isDirectory) {
        delete nextObjectSet;
    }
    delete zap;

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
        listFiles(MOS, getHeaddatasetID(MOS, rootDatasetDirectoryID), datasets, FALSE, this->getPoolName());
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
