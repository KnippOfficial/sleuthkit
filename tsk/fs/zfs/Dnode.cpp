/*
 * Dnode.cpp
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file Dnode.cpp
 * Describes the Dnode object in ZFS
 */

#include "Dnode.h"

Dnode::Dnode(TSK_ENDIAN_ENUM endian, uint8_t *data, ZFS_POOL *pool)
        : dn_bonus_blkptr(nullptr), dn_blkptr(), dn_bonuslen(0), dn_bonustype(0), dn_nblkptr(0), dn_nlevels(0),
          dn_type(DMU_OT_NONE),
          endian(endian), pool(pool),
          dn_bonus() {
    dn_type = static_cast<dmu_object_type_t>((uint8_t) data[0]);
    dn_nlevels = (uint8_t) data[2];
    dn_nblkptr = (uint8_t) data[3];
    dn_bonustype = (uint8_t) data[4];
    dn_bonuslen = read16Bit(endian, data + 10);

    //not a valid dnode => throw exception
    if ((this->dn_nlevels > 8) || (dn_nlevels == 0) || (dn_nblkptr > 3)) {
        throw (0);
    }

    //store all valid block pointers
    for (int i = 0; i < dn_nblkptr; ++i) {
        try {
            dn_blkptr.push_back(new Blkptr(endian, data + 64 + (i * 128), pool));
        }
        catch (...) {
            break;
        }
    }

    //check and create bonus member
    if (dn_bonuslen > 0) {
        generateBonus(endian, data + (dn_nblkptr * 128) + 64);
    }
}

void Dnode::getData(std::vector<char> &data) {
    if ((dn_type == DMU_OT_DSL_DATASET) && (dn_blkptr.size() == 0)) {
        if (dn_bonus_blkptr != nullptr) {
            dn_bonus_blkptr->getData(data);
        }
    } else {
        for (auto &it : dn_blkptr) {
            it->getData(data, dn_nlevels);
        }
    }
}

void Dnode::generateBonus(TSK_ENDIAN_ENUM endian, uint8_t *data) {
    switch (unsigned(dn_bonustype)) {
        case 16:
            this->dn_bonus["ds_dir_obj"] = read64Bit(endian, data);
            this->dn_bonus["ds_prev_snap_obj"] = read64Bit(endian, data + 8);
            this->dn_bonus["ds_prev_snap_txg"] = read64Bit(endian, data + 16);
            this->dn_bonus["ds_next_snap_obj"] = read64Bit(endian, data + 24);
            this->dn_bonus["ds_snapnames_zapobj"] = read64Bit(endian, data + 32);
            this->dn_bonus["ds_num_children"] = read64Bit(endian, data + 40);
            this->dn_bonus["ds_creation_time"] = read64Bit(endian, data + 48);
            this->dn_bonus["ds_creation_txg"] = read64Bit(endian, data + 56);
            this->dn_bonus["ds_deadlist_obj"] = read64Bit(endian, data + 64);
            this->dn_bonus["ds_used_bytes"] = read64Bit(endian, data + 72);
            this->dn_bonus["ds_compressed_bytes"] = read64Bit(endian, data + 80);
            this->dn_bonus["ds_uncompressed_bytes"] = read64Bit(endian, data + 88);
            this->dn_bonus["ds_unique_bytes"] = read64Bit(endian, data + 96);
            this->dn_bonus["ds_fsidguid"] = read64Bit(endian, data + 104);
            this->dn_bonus["ds_guid"] = read64Bit(endian, data + 112);
            try {
                dn_bonus_blkptr = new Blkptr(endian, data + 128, this->pool);
            }
            catch (...) {
                //in case of empty, it points to no object set
                dn_bonus_blkptr = NULL;
            }
            break;
        case 44:
            this->dn_bonus["zp_atime"] = read64Bit(endian, data + 64);
            this->dn_bonus["zp_mtime"] = read64Bit(endian, data + 80);
            this->dn_bonus["zp_ctime"] = read64Bit(endian, data + 96);
            this->dn_bonus["zp_crtime"] = read64Bit(endian, data + 112);
            this->dn_bonus["zp_size"] = read64Bit(endian, data + 16);
            this->dn_bonus["zp_parent"] = read64Bit(endian, data + 48);
            break;
        case 12:
            this->dn_bonus["dd_creation_time"] = read64Bit(endian, data);
            this->dn_bonus["dd_head_dataset"] = read64Bit(endian, data + 8);
            this->dn_bonus["dd_parent_obj"] = read64Bit(endian, data + 16);
            this->dn_bonus["dd_child_dir_zapobj"] = read64Bit(endian, data + 32);
            break;
        default:
            break;
    }
}

uint64_t Dnode::getBonusValue(string name) {
    if (dn_bonus.find(name) == dn_bonus.end())
        return FALSE;   // TODO: don't return false here
    else
        return dn_bonus[string(name)];
}

Blkptr *Dnode::getBonusBlkptr() {
    return dn_bonus_blkptr;
}

ostream &operator<<(ostream &os, const Dnode &dnode) {
    os << "type: " << dnode.dn_type << "\t|\t lvl: " << unsigned(dnode.dn_nlevels) << "\t|\t nblkptrs: "
       << unsigned(dnode.dn_nblkptr) << "\t|\t bonus type/len: " << unsigned(dnode.dn_bonustype) << "/"
       << dnode.dn_bonuslen << endl;
    cout << endl;

    if (dnode.dn_bonuslen > 0) {
        for (auto &it : dnode.dn_bonus) {
            if (it.first.find("time") != string::npos)
                cout << it.first << " = " << timestampToDateString(it.second) << endl;
            else
                cout << it.first << " = " << it.second << endl;
        }
        cout << endl;
    }

    if (dnode.dn_bonus_blkptr != NULL) {
        os << *dnode.dn_bonus_blkptr << endl;
    }

    //show only highest level block pointer
    for (int i = 0; i < dnode.dn_blkptr.size(); ++i) {
        os << *dnode.dn_blkptr[i];
    }

    //show level 0 block pointer
    os << endl << "Level 0 blocks:" << endl;
    for (int i = 0; i < dnode.dn_blkptr.size(); ++i) {
        os << dnode.dn_blkptr[i]->level0Blocks(dnode.dn_nlevels).str() << endl;
    }

    return os;
}

Dnode::~Dnode() {
    for (int i = 0; i < this->dn_blkptr.size(); ++i) {
        delete this->dn_blkptr[i];
    }

    delete this->dn_bonus_blkptr;
}
