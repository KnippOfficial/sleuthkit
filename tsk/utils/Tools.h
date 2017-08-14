/*
 * Tools.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file Tools.h
 * Includes some tools for ZFS
 */

#ifndef TSK_TOOLS_H
#define TSK_TOOLS_H

#include <iostream>

const std::string timestampToDateString(uint64_t timestamp);
void fletcher_4_native(const uint8_t* buf, uint64_t size, uint64_t* output);

#endif