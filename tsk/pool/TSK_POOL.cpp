/*
 * TSK_POOL.cpp
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

#include "TSK_POOL.h"

std::ostream& operator<<(std::ostream& os, const TSK_POOL& pool) {
    pool.print(os);
    return os;
};
