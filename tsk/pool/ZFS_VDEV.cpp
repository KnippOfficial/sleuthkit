/*
 * ZFS_VDEV.cpp
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file ZFS_VDEV.cpp
 * Used to handle top-level virtual devices of ZFS.
 */

#include "ZFS_VDEV.h"

//TODO: rename to ZFS_TLVDEV
ZFS_VDEV::ZFS_VDEV(NVList *list) {
    pool_guid = list->getIntValue("pool_guid");

    NVList *vdev_tree = list->getNVListArray("vdev_tree").at(0);
    id = vdev_tree->getIntValue("id");
    guid = vdev_tree->getIntValue("guid");
    type = vdev_tree->getStringValue("type");

    if (type == "raidz") {
        nparity = vdev_tree->getIntValue("nparity");
    }

    usable = false;
    noActiveChildren = 0;

    if (type == "file" || type == "disk") {
        ZFS_DEVICE temp;
        temp.create_txg = vdev_tree->getIntValue("create_txg");
        temp.id = 0;
        temp.available = FALSE;
        temp.path = vdev_tree->getStringValue("path");
        temp.guid = vdev_tree->getIntValue("guid");
        this->children.push_back(temp);
    } else if (type == "mirror") {
        vector<NVList *> children = vdev_tree->getNVListArray("children");
        for (int i = 0; i < children.size(); i++) {
            ZFS_DEVICE temp;
            temp.create_txg = children.at(i)->getIntValue("create_txg");
            temp.id = children.at(i)->getIntValue("id");
            temp.type = children.at(i)->getStringValue("type");
            temp.available = FALSE;
            temp.path = children.at(i)->getStringValue("path");
            temp.guid = children.at(i)->getIntValue("guid");
            this->children.push_back(temp);
        }
    } else if (type == "raidz") {
        vector<NVList *> children = vdev_tree->getNVListArray("children");
        for (int i = 0; i < children.size(); i++) {
            ZFS_DEVICE temp;
            temp.create_txg = children.at(i)->getIntValue("create_txg");
            temp.id = children.at(i)->getIntValue("id");
            temp.type = children.at(i)->getStringValue("type");
            temp.available = FALSE;
            temp.path = children.at(i)->getStringValue("path");
            temp.guid = children.at(i)->getIntValue("guid");
            //TODO: order by ID
            this->children.push_back(temp);
        }
    }
}

bool ZFS_VDEV::addDevice(uint64_t guid, std::pair<string, TSK_IMG_INFO *> img) {
    for (auto &it : this->children) {
        if (it.guid == guid) {
            if (!it.available) {
                it.available = true;
                noActiveChildren++;
                checkUsable();
                it.img = img.second;
                it.accessedVia = img.first;
                return true;
            } else {
                return false;
            }
        }
    }
}

void ZFS_VDEV::checkUsable() {
    if (!usable) {
        if (type == "mirror") {
            if (noActiveChildren > 0)
                usable = true;
        } else if (type == "file" || type == "disk") {
            if (noActiveChildren == children.size())
                usable = true;
        } else if (type == "raidz") {
            if (noActiveChildren >= (this->children.size() - nparity))
                usable = true;
        }
    }
}

//TODO: get child by ID
ZFS_DEVICE ZFS_VDEV::getChild(uint64_t childID) {
    if (childID > children.size()) {
        cout << "Child " << childID << " is not existent in this vdev!" << endl;
    } else {
        for (auto it : children) {
            if(it.id == childID){
                return it;
            }
        }
    }
}

ostream &operator<<(ostream &os, const ZFS_VDEV &vdev) {
    os << vdev.type << " (ID:  " << vdev.id << ")" << endl;
    for (int i = 0; i < vdev.children.size(); i++) {
        os << "    " << vdev.children.at(i).path << " (ID: " << vdev.children.at(i).id << ") ";
        if (vdev.children.at(i).available) {
            os << "AVAILABLE" << endl;
            os << "\tACCESSED VIA: " << vdev.children.at(i).accessedVia << endl;
        } else
            os << "UNAVAILABLE" << endl;
    }

    return os;
}
