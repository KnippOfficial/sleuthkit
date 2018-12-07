/*
 * TSK_POOL.h
 * The Sleuth Kit
 *
 * This software is distributed under the Common Public License 1.0
 *
 */

/**
 * \file TSK_POOL.h
 * Abstract pool class used to create pool objects
 */

#ifndef THESLEUTHKIT_ZTK_TSK_POOL_H
#define THESLEUTHKIT_ZTK_TSK_POOL_H

#include <string>

using namespace std;

class TSK_POOL {

private:

public:
    virtual void fsstat(string str_dataset, int transaction) = 0;

    virtual void fls(string str_dataset, int transaction) = 0;

    virtual void istat(int object_number, string str_dataset, int transaction) = 0;

    virtual void icat(int object_number, string str_dataset, int transaction) = 0;

    virtual void print(std::ostream &os) const = 0;

    virtual void fwalk(string str_dataset, int transaction, string restorePath, bool debug) = 0;

    friend std::ostream &operator<<(std::ostream &os, const TSK_POOL &pool);

};

#endif //THESLEUTHKIT_ZTK_TSK_POOL_H
