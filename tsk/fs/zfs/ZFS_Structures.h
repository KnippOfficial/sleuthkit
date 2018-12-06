/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2011, 2016 by Delphix. All rights reserved.
 * Copyright 2011 Nexenta Systems, Inc. All rights reserved.
 * Copyright (c) 2012, Joyent, Inc. All rights reserved.
 * Copyright 2013 DEY Storage Systems, Inc.
 * Copyright 2014 HybridCluster. All rights reserved.
 * Copyright (c) 2014 Spectra Logic Corporation, All rights reserved.
 * Copyright 2013 Saso Kiselkov. All rights reserved.
 * Copyright (c) 2014 Integros [integros.com]
 */

/* Portions Copyright 2010 Robert Milkowski */

#ifndef ZFS_FTK_ZFS_STRUCTURES_H
#define ZFS_FTK_ZFS_STRUCTURES_H

typedef enum dmu_object_type {
	DMU_OT_NONE,
	/* general: */
	DMU_OT_OBJECT_DIRECTORY,	/* ZAP */
	DMU_OT_OBJECT_ARRAY,		/* UINT64 */
	DMU_OT_PACKED_NVLIST,		/* UINT8 (XDR by nvlist_pack/unpack) */
	DMU_OT_PACKED_NVLIST_SIZE,	/* UINT64 */
	DMU_OT_BPOBJ,			/* UINT64 */
	DMU_OT_BPOBJ_HDR,		/* UINT64 */
	/* spa: */
	DMU_OT_SPACE_MAP_HEADER,	/* UINT64 */
	DMU_OT_SPACE_MAP,		/* UINT64 */
	/* zil: */
	DMU_OT_INTENT_LOG,		/* UINT64 */
	/* dmu: */
	DMU_OT_DNODE,			/* DNODE */
	DMU_OT_OBJSET,			/* OBJSET */
	/* dsl: */
	DMU_OT_DSL_DIR,			/* UINT64 */
	DMU_OT_DSL_DIR_CHILD_MAP,	/* ZAP */
	DMU_OT_DSL_DS_SNAP_MAP,		/* ZAP */
	DMU_OT_DSL_PROPS,		/* ZAP */
	DMU_OT_DSL_DATASET,		/* UINT64 */
	/* zpl: */
	DMU_OT_ZNODE,			/* ZNODE */
	DMU_OT_OLDACL,			/* Old ACL */
	DMU_OT_PLAIN_FILE_CONTENTS,	/* UINT8 */
	DMU_OT_DIRECTORY_CONTENTS,	/* ZAP */
	DMU_OT_MASTER_NODE,		/* ZAP */
	DMU_OT_UNLINKED_SET,		/* ZAP */
	/* zvol: */
	DMU_OT_ZVOL,			/* UINT8 */
	DMU_OT_ZVOL_PROP,		/* ZAP */
	/* other; for testing only! */
	DMU_OT_PLAIN_OTHER,		/* UINT8 */
	DMU_OT_UINT64_OTHER,		/* UINT64 */
	DMU_OT_ZAP_OTHER,		/* ZAP */
	/* new object types: */
	DMU_OT_ERROR_LOG,		/* ZAP */
	DMU_OT_SPA_HISTORY,		/* UINT8 */
	DMU_OT_SPA_HISTORY_OFFSETS,	/* spa_his_phys_t */
	DMU_OT_POOL_PROPS,		/* ZAP */
	DMU_OT_DSL_PERMS,		/* ZAP */
	DMU_OT_ACL,			/* ACL */
	DMU_OT_SYSACL,			/* SYSACL */
	DMU_OT_FUID,			/* FUID table (Packed NVLIST UINT8) */
	DMU_OT_FUID_SIZE,		/* FUID table size UINT64 */
	DMU_OT_NEXT_CLONES,		/* ZAP */
	DMU_OT_SCAN_QUEUE,		/* ZAP */
	DMU_OT_USERGROUP_USED,		/* ZAP */
	DMU_OT_USERGROUP_QUOTA,		/* ZAP */
	DMU_OT_USERREFS,		/* ZAP */
	DMU_OT_DDT_ZAP,			/* ZAP */
	DMU_OT_DDT_STATS,		/* ZAP */
	DMU_OT_SA,			/* System attr */
	DMU_OT_SA_MASTER_NODE,		/* ZAP */
	DMU_OT_SA_ATTR_REGISTRATION,	/* ZAP */
	DMU_OT_SA_ATTR_LAYOUTS,		/* ZAP */
	DMU_OT_SCAN_XLATE,		/* ZAP */
	DMU_OT_DEDUP,			/* fake dedup BP from ddt_bp_create() */
	DMU_OT_DEADLIST,		/* ZAP */
	DMU_OT_DEADLIST_HDR,		/* UINT64 */
	DMU_OT_DSL_CLONES,		/* ZAP */
	DMU_OT_BPOBJ_SUBOBJ,		/* UINT64 */
	/*
	 * Do not allocate new object types here. Doing so makes the on-disk
	 * format incompatible with any other format that uses the same object
	 * type number.
	 *
	 * When creating an object which does not have one of the above types
	 * use the DMU_OTN_* type with the correct byteswap and metadata
	 * values.
	 *
	 * The DMU_OTN_* types do not have entries in the dmu_ot table,
	 * use the DMU_OT_IS_METDATA() and DMU_OT_BYTESWAP() macros instead
	 * of indexing into dmu_ot directly (this works for both DMU_OT_* types
	 * and DMU_OTN_* types).
	 */
	DMU_OT_NUMTYPES,
} dmu_object_type_t;

inline std::ostream& operator<<(std::ostream& out, const dmu_object_type &value){
    const char* s = 0;
	
	#define PROCESS_VAL(p) case(p): s = #p; break;
    switch(value){
	
		PROCESS_VAL(DMU_OT_NONE);
			/* general: */
		PROCESS_VAL(DMU_OT_OBJECT_DIRECTORY);
		PROCESS_VAL(DMU_OT_OBJECT_ARRAY);
		PROCESS_VAL(DMU_OT_PACKED_NVLIST);
		PROCESS_VAL(DMU_OT_PACKED_NVLIST_SIZE);
		PROCESS_VAL(DMU_OT_BPOBJ);
		PROCESS_VAL(DMU_OT_BPOBJ_HDR);
			/* spa: */
		PROCESS_VAL(DMU_OT_SPACE_MAP_HEADER);
		PROCESS_VAL(DMU_OT_SPACE_MAP);
			/* zil: */
		PROCESS_VAL(DMU_OT_INTENT_LOG);
			/* dmu: */
		PROCESS_VAL(DMU_OT_DNODE);
		PROCESS_VAL(DMU_OT_OBJSET);
			/* dsl: */
		PROCESS_VAL(DMU_OT_DSL_DIR);
		PROCESS_VAL(DMU_OT_DSL_DIR_CHILD_MAP);
		PROCESS_VAL(DMU_OT_DSL_DS_SNAP_MAP);
		PROCESS_VAL(DMU_OT_DSL_PROPS);
		PROCESS_VAL(DMU_OT_DSL_DATASET);
			/* zpl: */
		PROCESS_VAL(DMU_OT_ZNODE);
		PROCESS_VAL(DMU_OT_OLDACL);
		PROCESS_VAL(DMU_OT_PLAIN_FILE_CONTENTS);
		PROCESS_VAL(DMU_OT_DIRECTORY_CONTENTS);
		PROCESS_VAL(DMU_OT_MASTER_NODE);
		PROCESS_VAL(DMU_OT_UNLINKED_SET);
			/* zvol: */
		PROCESS_VAL(DMU_OT_ZVOL);
		PROCESS_VAL(DMU_OT_ZVOL_PROP);
			/* other; for testing only! */
		PROCESS_VAL(DMU_OT_PLAIN_OTHER);
		PROCESS_VAL(DMU_OT_UINT64_OTHER);
		PROCESS_VAL(DMU_OT_ZAP_OTHER);
			/* new object types: */
		PROCESS_VAL(DMU_OT_ERROR_LOG);
		PROCESS_VAL(DMU_OT_SPA_HISTORY);
		PROCESS_VAL(DMU_OT_SPA_HISTORY_OFFSETS);
		PROCESS_VAL(DMU_OT_POOL_PROPS);
		PROCESS_VAL(DMU_OT_DSL_PERMS);
		PROCESS_VAL(DMU_OT_ACL);
		PROCESS_VAL(DMU_OT_SYSACL);
		PROCESS_VAL(DMU_OT_FUID);
		PROCESS_VAL(DMU_OT_FUID_SIZE);
		PROCESS_VAL(DMU_OT_NEXT_CLONES);
		PROCESS_VAL(DMU_OT_SCAN_QUEUE);
		PROCESS_VAL(DMU_OT_USERGROUP_USED);
		PROCESS_VAL(DMU_OT_USERGROUP_QUOTA);
		PROCESS_VAL(DMU_OT_USERREFS);
		PROCESS_VAL(DMU_OT_DDT_ZAP);
		PROCESS_VAL(DMU_OT_DDT_STATS);
		PROCESS_VAL(DMU_OT_SA);
		PROCESS_VAL(DMU_OT_SA_MASTER_NODE);
		PROCESS_VAL(DMU_OT_SA_ATTR_REGISTRATION);
		PROCESS_VAL(DMU_OT_SA_ATTR_LAYOUTS);
		PROCESS_VAL(DMU_OT_SCAN_XLATE);
		PROCESS_VAL(DMU_OT_DEDUP);
		PROCESS_VAL(DMU_OT_DEADLIST);
		PROCESS_VAL(DMU_OT_DEADLIST_HDR);
		PROCESS_VAL(DMU_OT_DSL_CLONES);
		PROCESS_VAL(DMU_OT_BPOBJ_SUBOBJ);
		PROCESS_VAL(DMU_OT_NUMTYPES);
		
	}
	#undef PROCESS_VAL

    return out << s ;
}


#endif //ZFS_FTK_ZFS_STRUCTURES_H
