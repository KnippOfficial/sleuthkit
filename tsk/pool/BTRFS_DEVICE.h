/*
 * BTRFS_DEVICE.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file BTRFS_DEVICE.h
 * Creates a BTRFS_DEVICE object
 */

#ifndef CODE_BTRFS_DEVICE_H
#define CODE_BTRFS_DEVICE_H

#include <string.h>
#include <tsk/libtsk.h>
#include "../utils/Uuid.h"
#include "../../tsk/fs/btrfs/Basics/Basics.h"

using namespace std;

class BTRFS_DEVICE{

private:
    uint64_t id;
    string guid;
    TSK_IMG_INFO *image;
    uint64_t offset;
    bool available;

public:
    BTRFS_DEVICE(uint64_t, string, TSK_IMG_INFO*, uint64_t, bool);
    ~BTRFS_DEVICE();

    uint64_t getID() const { return id;};
    uint64_t getOffset() const { return offset;};
    string getGUID() const { return guid;};
    TSK_IMG_INFO* getImg() const { return image;};
    bool getAvailable() const { return available;};
    void setAvailable(bool);
    void setImage(TSK_IMG_INFO*);

};




#endif //CODE_BTRFS_DEVICE_H
