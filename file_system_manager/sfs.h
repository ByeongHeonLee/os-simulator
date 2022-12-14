#ifndef _SFS_H_
#define _SFS_H_

#define SFS_MAGIC         0xabadf001    /* magic number identifying us */
#define SFS_BLOCKSIZE     512           /* size of our blocks */
#define SFS_VOLNAME_SIZE  32            /* max length of volume name */
#define SFS_NDIRECT       15            /* # of direct blocks in inode */
#define SFS_DBPERIDB      128           /* # direct blks per indirect blk */
#define SFS_NAMELEN       60            /* max length of filename */
#define SFS_SB_LOCATION    0            /* block the superblock lives in */
#define SFS_ROOT_LOCATION  1            /* loc'n of the root dir inode */
#define SFS_MAP_LOCATION   2            /* 1st block of the freemap */
#define SFS_NOINO          0            /* inode # for free dir entry */

/* Number of directory entry in a block */
#define SFS_DENTRYPERBLOCK (SFS_BLOCKSIZE/sizeof(struct sfs_dir))

/* Number of bits in a block */
#define SFS_BLOCKBITS (SFS_BLOCKSIZE * CHAR_BIT)

/* Utility macro */
#define SFS_ROUNDUP(a,b)       ((((a)+(b)-1)/(b))*b)	// ((a)+(b)-1)/(b) --> ROUNDUP()

/* Size of bitmap (in bits) */
#define SFS_BITMAPSIZE(nblocks) SFS_ROUNDUP(nblocks, SFS_BLOCKBITS)

/* Size of bitmap (in blocks) */
#define SFS_BITBLOCKS(nblocks)  (SFS_BITMAPSIZE(nblocks)/SFS_BLOCKBITS)

/* File types for dfi_type */
#define SFS_TYPE_INVAL    0       /* Should not appear on disk */
#define SFS_TYPE_FILE     1       /* Is file */
#define SFS_TYPE_DIR      2       /* Is directory */

/*
 * On-disk superblock
 */
struct sfs_super {
	u_int32_t sp_magic;                 /* Magic number, should be SFS_MAGIC */
	u_int32_t sp_nblocks;               /* Number of blocks in fs */
	char sp_volname[SFS_VOLNAME_SIZE];  /* Name of this volume */
	u_int32_t reserved[118];            /* Dummy */
};

/*
 * On-disk inode
 */
struct sfs_inode {
	u_int32_t sfi_size;        /* Size of this file (bytes) */
	u_int16_t sfi_type;        /* One of SFS_TYPE_* above */
	u_int16_t sfi_linkcount;   /* Number of hard links to this file */ /* UNUSED in our hw */
	u_int32_t sfi_direct[SFS_NDIRECT];	    /* Direct blocks 60Byte */
	u_int32_t sfi_indirect;			        /* Indirect block, Single indirect pointer, 128 direct pointers */
	u_int32_t sfi_waste[128-3-SFS_NDIRECT]; /* Dummy */
};

/*
 * On-disk directory entry
 */
struct sfs_dir {
	u_int32_t sfd_ino;           /* Inode number,  4 Byte */
	char sfd_name[SFS_NAMELEN];  /* Filename,     60 Byte */
};

#endif /* _SFS_H_ */
