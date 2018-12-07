/*
** pwalk
** The Sleuth Kit
**
** Given a directory of pool images, it analyzes and displays pool information
** and configuration.
**
** If you
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
    TFPRINTF(stderr, _TSK_T ("usage: %s [-d] [-S sub_file_system] [-T transaction] [-R restorePath] images\n"), progname);
    tsk_fprintf(stderr,
                "\t-d: print debug output\n");
    tsk_fprintf(stderr,
                "\t-S: Specify sub file system (only used for pools)\n");
    tsk_fprintf(stderr,
                "\t-T: Specify transaction or generation number to use\n");
    tsk_fprintf(stderr,
                "\t-R: Specify path where contents should be restored\n");
    tsk_fprintf(stderr,
                "\n\tNote: if -R is not specified, pwalk will only list files\n");           
    exit(1);
}

int main(int argc, char **argv1) {
    TSK_POOL_INFO *pool_info;
    extern int OPTIND;
    TSK_TCHAR **argv;
    int ch;

    int transaction = -1;
    string restorePath = "";
    string subFileSystem = "";
    bool debug = false;

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

    while ((ch =
            GETOPT(argc, argv, _TSK_T("dR:S:T:"))) > 0) {
        switch (ch) {
        case _TSK_T('?'):
        default:
            TFPRINTF(stderr, _TSK_T("Invalid argument: %s\n"),
                argv[OPTIND]);
            usage();
        case _TSK_T('d'):
            debug = true;
            break;
        case _TSK_T('R'):
            restorePath = OPTARG;
            break;
        case _TSK_T('S'):
            subFileSystem = OPTARG;
            break;
        case _TSK_T('T'):
            transaction = TATOI(OPTARG);
            break;
        }
    }

    /* We need at least one more argument */
    if (OPTIND == argc) {
        tsk_fprintf(stderr, "Missing path to directory containing images\n");
        usage();
    }

    //TODO: analyze only single device!!!
    pool_info = new TSK_POOL_INFO(TSK_LIT_ENDIAN, argv[OPTIND]);
    TSK_POOL* pool = pool_info->createPoolObject();
    pool->fwalk(subFileSystem, transaction, restorePath, debug);
    //cout << *pool->getUberblockArray() << endl;

    delete pool;
    delete pool_info;

    exit(0);
}