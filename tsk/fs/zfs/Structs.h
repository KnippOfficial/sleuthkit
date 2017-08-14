/*
 * Structs.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file Structs.h
 * Structs used for ZFS
 */

#ifndef ZFS_FTK_STRUCTS_H
#define ZFS_FTK_STRUCTS_H
#include <cstdint>

typedef struct dva{
    uint64_t offset;
    uint32_t vdev;
} dva;

#endif //ZFS_FTK_STRUCTS_H
