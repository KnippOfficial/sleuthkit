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

#include "BTRFS_DEVICE.h"


BTRFS_DEVICE::BTRFS_DEVICE(uint64_t id, string guid, TSK_IMG_INFO *img, uint64_t offset, bool available)
        : id(id), guid(guid), image(img), offset(offset), available(available) {
}


BTRFS_DEVICE::~BTRFS_DEVICE() {

}

void BTRFS_DEVICE::setAvailable(bool av) {
    available = av;
}

void BTRFS_DEVICE::setImage(TSK_IMG_INFO* img) {
    image = img;
}