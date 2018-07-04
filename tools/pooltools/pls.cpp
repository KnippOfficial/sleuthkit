/*
** pls
** The Sleuth Kit
**
** Given a directory of pool images, it analyzes and displays pool information
** and configuration.
**
** Brian Carrier [carrier <at> sleuthkit [dot] org]
** Copyright (c) 2006-2011 Brian Carrier, Basis Technology.  All Rights reserved
** Copyright (c) 2003-2005 Brian Carier.  All rights reserved
** Copyright (c) 2017 Jan-Niclas Hilgert.  All rights reserved
** TASK
** Copyright (c) 2002 @stake Inc.  All rights reserved
**
** TCTUTILs
** Copyright (c) 2001 Brian Carrier.  All rights reserved
**
**
** This software is distributed under the Common Public License 1.0
**
*/

#include "tsk/tsk_tools_i.h"
#include <locale.h>
#include <time.h>
#include "../../tsk/pool/TSK_POOL_INFO.h"
#include "../../tsk/fs/zfs/ZFS_Functions.h"
#include "../../tsk/pool/ZFS_POOL.h"

#include <iostream>
#include <tsk/libtsk.h>

static TSK_TCHAR *progname;

void usage() {
    TFPRINTF(stderr, _TSK_T ("usage: %s images\n"), progname);
    exit(1);
}

int main(int argc, char **argv1) {
    TSK_POOL_INFO *pool_info;
    extern int OPTIND;
    static TSK_TCHAR *macpre = NULL;
    TSK_TCHAR **argv;
    unsigned int ssize = 0;
    TSK_TCHAR *cp;

#ifdef TSK_WIN32
    // On Windows, get the wide arguments (mingw doesn't support wmain)
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == NULL) {
        fprintf(stderr, "Error getting wide arguments\n");
        exit(1);
    }
#else
    argv = (TSK_TCHAR **) argv1;
#endif
    progname = argv[0];
    setlocale(LC_ALL, "");

    /* We need at least one more argument */
    if (OPTIND == argc) {
        tsk_fprintf(stderr, "Missing path to directory containing images\n");
        usage();
    }
    //TODO: analyze only single device!!!
    pool_info = new TSK_POOL_INFO(TSK_LIT_ENDIAN, argv[1]);
    TSK_POOL* pool = pool_info->createPoolObject();
    cout << *pool << endl;
    //cout << *pool->getUberblockArray() << endl;

    delete pool;
    delete pool_info;

    exit(0);
}