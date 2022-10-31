//
// Submission Year: 2020
// Subject : Operating System (101511-3)
// Simple FIle System
// Student Number : B684031
// Student Name : Lee Byeong Heon (이병헌)
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* optional */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
/***********/

#include "sfs_types.h"
#include "sfs_func.h"
#include "sfs_disk.h"
#include "sfs.h"

void dump_directory();

/* BIT operation Macros */
/* a=target variable, b=bit number to act upon 0-n */
#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))

#define SFS_DIR_MAX (SFS_NDIRECT * SFS_DENTRYPERBLOCK * sizeof(struct sfs_dir))
#define TRUE 1
#define FALSE 0

static struct sfs_super spb;	// superblock
static struct sfs_dir sd_cwd = { SFS_NOINO }; // i-node of current working directory

struct sfs_bitmap {
	u_int8_t bitmap[SFS_BLOCKSIZE];
};

void error_message(const char *message, const char *path, int error_code) {
	switch (error_code) {
	case -1:
		printf("%s: %s: No such file or directory\n",message, path); return;
	case -2:
		printf("%s: %s: Not a directory\n",message, path); return;
	case -3:
		printf("%s: %s: Directory full\n",message, path); return;
	case -4:
		printf("%s: %s: No block available\n",message, path); return;
	case -5:
		printf("%s: %s: Not a directory\n",message, path); return;
	case -6:
		printf("%s: %s: Already exists\n",message, path); return;
	case -7:
		printf("%s: %s: Directory not empty\n",message, path); return;
	case -8:
		printf("%s: %s: Invalid argument\n",message, path); return;
	case -9:
		printf("%s: %s: Is a directory\n",message, path); return;
	case -10:
		printf("%s: %s: Is not a file\n",message, path); return;
	default:
		printf("unknown error code\n");
		return;
	}
}

int get_emptyBlock(){
	int i, j, k;
	const int NBITMAPS = SFS_BITBLOCKS(spb.sp_nblocks);
	struct sfs_bitmap *bm = (struct sfs_bitmap*)malloc(NBITMAPS);
	bzero(bm, SFS_BLOCKSIZE * NBITMAPS);

	for(i = 0; i < NBITMAPS; i++)
		disk_read( &bm[i], SFS_MAP_LOCATION + i);

	for(i = 0; i < NBITMAPS; i++)
		for(j = 0; j < SFS_BLOCKSIZE; j++)
			for(k = 0; k < CHAR_BIT; k++)
				if( BIT_CHECK( bm[i].bitmap[j] , k ) == FALSE ){
					BIT_SET( bm[i].bitmap[j] , k );
					disk_write( &bm[i], SFS_MAP_LOCATION + i );
					return (k + (j * CHAR_BIT) + (i * SFS_BLOCKSIZE * CHAR_BIT));	
				}

	return -1;	
}

int get_emptyEntry(struct sfs_inode* si, int *dirBlock, int *dirEntry, const char *path ){
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];
	int i = 0, j = 0;

	disk_read( sd, si->sfi_direct[i] );

	while(sd[j].sfd_ino != SFS_NOINO){
				
		if( !strcmp(sd[j].sfd_name, path) ){
			return FALSE;
		}
		
		j++;
		if(j == SFS_DENTRYPERBLOCK){
			j = 0;
			i++;
		}
		*dirBlock = i;
		*dirEntry = j;
		disk_read( sd, si->sfi_direct[i] );
	}
	return TRUE;
}

void sfs_mount(const char* path)
{printf("test : %d\n", SFS_DIR_MAX);
	if( sd_cwd.sfd_ino !=  SFS_NOINO )  // Current Working Directory가 Free Directroy Entry가 아닐 경우
	{
		//umount
		disk_close();
		printf("%s, unmounted\n", spb.sp_volname);
		bzero(&spb, sizeof(struct sfs_super));
		sd_cwd.sfd_ino = SFS_NOINO;
	}

	printf("Disk image: %s\n", path);

	disk_open(path);
	disk_read( &spb, SFS_SB_LOCATION );

	printf("Superblock magic: %x\n", spb.sp_magic);

	assert( spb.sp_magic == SFS_MAGIC );
	
	printf("Number of blocks: %d\n", spb.sp_nblocks);
	printf("Volume name: %s\n", spb.sp_volname);
	printf("%s, mounted\n", spb.sp_volname);
	
	sd_cwd.sfd_ino = 1;		//init at root
	sd_cwd.sfd_name[0] = '/';
	sd_cwd.sfd_name[1] = '\0';
}

void sfs_umount()
{
	if( sd_cwd.sfd_ino !=  SFS_NOINO )
	{
		//umount
		disk_close();
		printf("%s, unmounted\n", spb.sp_volname);
		bzero(&spb, sizeof(struct sfs_super));
		sd_cwd.sfd_ino = SFS_NOINO;
	}
}

void sfs_touch(const char* path)
{
	struct sfs_inode si;
	disk_read( &si, sd_cwd.sfd_ino );

	//for consistency
	assert( si.sfi_type == SFS_TYPE_DIR );

	if(si.sfi_size >= SFS_DIR_MAX + ((SFS_BLOCKSIZE / 4) * SFS_BLOCKSIZE) ){
		error_message("touch", path, -3);
		return;
	}

	/*
	// block access
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];
	int emptyDB, emptyDE; // Empty Direct Block, Empry Directory Entry
	int i, j;
	for(i = 0; i <= emptyDB; i++){
		disk_read( sd, si.sfi_direct[i] );
		for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
			if(sd[j].sfd_ino != SFS_NOINO){
				emptyDB = i;
				emptyDE = j;
			}

			if( !strcmp(sd[j].sfd_name, path) ){
				error_message("touch", path, -6);
				return;
			}
		}
	}
	*/
	
	/*
	//buffer for disk read
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	// block access
	int emptyDB, emptyDE; // Empty Direct Block, Empry Directory Entry
	int i = 0, j = 0;
	disk_read( sd, si.sfi_direct[i] );
	while(sd[j].sfd_ino != SFS_NOINO){
				
		if( !strcmp(sd[j].sfd_name, path) ){
			error_message("touch", path, -6);
			return;
		}
		
		j++;
		if(j == SFS_DENTRYPERBLOCK){
			j = 0;
			i++;
		}
		emptyDB = i;
		emptyDE = j;
		disk_read( sd, si.sfi_direct[i] );
	}
	*/

	int emptyDB, emptyDE; // Empty Direct Block, Empry Directory Entry
	if( !get_emptyEntry( &si, &emptyDB, &emptyDE, path ) ){
		error_message("touch", path, -6);
		return;
	}

	//allocate new block
	int newbie_ino = get_emptyBlock();
	if( newbie_ino == -1 ){
		error_message("touch", path, -4);
		return;
	}

	if(emptyDE == 0){
		int newdir_ino = get_emptyBlock();
		if( newdir_ino == -1 ){
			error_message("touch", path, -4);
			return;
		}

		si.sfi_direct[emptyDB] = newdir_ino;
	}

	struct sfs_dir sd[SFS_DENTRYPERBLOCK];
	disk_read( sd, si.sfi_direct[emptyDB] );

	sd[emptyDE].sfd_ino = newbie_ino;
	strncpy( sd[emptyDE].sfd_name, path, SFS_NAMELEN );

	int i;
	for(i = emptyDE + 1; i < SFS_DENTRYPERBLOCK; i++)
	    sd[i].sfd_ino = SFS_NOINO;

	disk_write( sd, si.sfi_direct[emptyDB] );

	si.sfi_size += sizeof(struct sfs_dir);
	disk_write( &si, sd_cwd.sfd_ino );

	struct sfs_inode newbie;

	bzero(&newbie,SFS_BLOCKSIZE); // initalize sfi_direct[] and sfi_indirect
	newbie.sfi_size = 0;
	newbie.sfi_type = SFS_TYPE_FILE;

	disk_write( &newbie, newbie_ino );
}

void sfs_cd(const char* path)
{
	if(path == NULL){ // Change Directory to Root
		sd_cwd.sfd_ino = SFS_ROOT_LOCATION;
		sd_cwd.sfd_name[0] = '/';
		sd_cwd.sfd_name[1] = '\0';
	}
	else{ // Change Directory to "path" Directory
		int i, j;
		struct sfs_inode si, ti;
		struct sfs_dir sd[SFS_DENTRYPERBLOCK];
		disk_read( &si, sd_cwd.sfd_ino );
		
		for(i = 0; i < SFS_NDIRECT; i++){
			if(si.sfi_direct[i] == SFS_NOINO)
				break;
			
			disk_read( &sd, si.sfi_direct[i] );

			for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
				if(sd[j].sfd_ino == SFS_NOINO)
					break;

				if( !strcmp(sd[j].sfd_name, path) ){
					disk_read( &ti, sd[j].sfd_ino );

					if( ti.sfi_type == SFS_TYPE_FILE ){
						error_message("cd", path, -5);
						return;
					}
					else{ // ti.sfi_type == SFS_TYPE_DIR
					    sd_cwd.sfd_ino = sd[j].sfd_ino;
						strcpy(sd_cwd.sfd_name, sd[j].sfd_name);
						return;
					}
				}
			}
		}
		error_message("cd", path, -1);
	}
	
}

void sfs_ls(const char* path)
{
	int i, j;
	struct sfs_inode si, ti;
	struct sfs_dir sd[SFS_DENTRYPERBLOCK], temp;
	disk_read( &si, sd_cwd.sfd_ino );
	assert(si.sfi_type == SFS_TYPE_DIR);

	if ( path == NULL ){  // CASE: ls
		for(i = 0; i < SFS_NDIRECT; i++){
			
			if(si.sfi_direct[i] == SFS_NOINO)
				break;
			
			disk_read( &sd, si.sfi_direct[i] );
			
			for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
				if(sd[j].sfd_ino == SFS_NOINO)
					break;

			    printf("%s", sd[j].sfd_name);

				disk_read( &ti, sd[j].sfd_ino );
				if(ti.sfi_type == SFS_TYPE_DIR)
					printf("/");
				printf("\t");
			}
		}
		printf("\n");
		return;
	}
	
	else{                        // CASE: ls <path>
		for(i = 0; i < SFS_NDIRECT; i++){
			if(si.sfi_direct[i] == SFS_NOINO)
				break;
			
			disk_read( &sd, si.sfi_direct[i] );

			for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
				if(sd[j].sfd_ino == SFS_NOINO)
					break;

				if( !strcmp(sd[j].sfd_name, path) ){
					disk_read( &ti, sd[j].sfd_ino );

					if(ti.sfi_type == SFS_TYPE_FILE){
						printf("%s\n", sd[j].sfd_name);
						return;
					}
					else{ // ti.sfi_type == SFS_TYPE_DIR
					    temp = sd_cwd;
						sd_cwd.sfd_ino = sd[j].sfd_ino;
						strcpy(sd_cwd.sfd_name, sd[j].sfd_name);
						sfs_ls(NULL);
						sd_cwd = temp;
						return;
					}
				}
			}
		}
	}
	error_message("ls", path, -1);
}

void sfs_mkdir(const char* org_path) 
{
	struct sfs_inode si;
	disk_read( &si, sd_cwd.sfd_ino );

	//for consistency
	assert( si.sfi_type == SFS_TYPE_DIR );

	// Error: Directory full
	if(si.sfi_size >= SFS_DIR_MAX){
		error_message("mkdir", org_path, -3);
		return;
	}

	/*
	// Error: <org_path> already exists in this directory
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];
	bzero( &sd, sizeof(struct sfs_dir) * SFS_DENTRYPERBLOCK );

	const int emptyDB // Empty Direct Block
		= (si.sfi_size / sizeof(struct sfs_dir)) / SFS_DENTRYPERBLOCK;
	
	const int emptyDE // Empry Directory Entry
		= (si.sfi_size / sizeof(struct sfs_dir)) % SFS_DENTRYPERBLOCK;

	int i, j;
	for(i = 0; i <= emptyDB; i++){
		disk_read( sd, si.sfi_direct[i] );
		for(j = 0; j < SFS_DENTRYPERBLOCK; j++){
			if( !strcmp(sd[j].sfd_name, org_path) ){
				error_message("mkdir", org_path, -6);
				return;
			}
		}
	}
	*/
	/*
	//buffer for disk read
	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	// block access
	int emptyDB, emptyDE; // Empty Direct Block, Empry Directory Entry
	int i = 0, j = 0;
	disk_read( sd, si.sfi_direct[i] );
	while(sd[j].sfd_ino != SFS_NOINO){
				
		if( !strcmp(sd[j].sfd_name, org_path) ){
			error_message("mkdir", org_path, -6);
			return;
		}
		
		j++;
		if(j == SFS_DENTRYPERBLOCK){
			j = 0;
			i++;
		}
		emptyDB = i;
		emptyDE = j;
		disk_read( sd, si.sfi_direct[i] );
	}
	*/

	int emptyDB, emptyDE; // Empty Direct Block, Empry Directory Entry
	if( !get_emptyEntry( &si, &emptyDB, &emptyDE, org_path ) ){
		error_message("mkdir", org_path, -6);
		return;
	}

	// Error: No block available
	int newbie_ino = get_emptyBlock();
	if( newbie_ino == -1 ){
		error_message("mkdir", org_path, -4);
		return;
	}
	
	// Error: No block available
	int newdir_ino = get_emptyBlock();
	if( newdir_ino == -1 ){
		error_message("mkdir", org_path, -4);
		return;
	}

	// Allcating new directory(newbie)
	struct sfs_inode newbie;
	bzero( &newbie, sizeof(struct sfs_inode) );
	newbie.sfi_size = 2 * sizeof(struct sfs_dir);
	newbie.sfi_type = SFS_TYPE_DIR;
	newbie.sfi_direct[0] = newdir_ino;

	struct sfs_dir sd[SFS_DENTRYPERBLOCK];

	sd[0].sfd_ino = newbie_ino;
	strncpy( sd[0].sfd_name, ".", SFS_NAMELEN );

	sd[1].sfd_ino = sd_cwd.sfd_ino;
	strncpy( sd[1].sfd_name, "..", SFS_NAMELEN );	

	int i;
	for(i = 2; i < SFS_DENTRYPERBLOCK; i++)
		sd[i].sfd_ino = SFS_NOINO;

	disk_write( &newbie, newbie_ino );
	disk_write( sd, newbie.sfi_direct[0] );
	
	// Update parent directory
	if(emptyDE == 0){
		int newdir_ino = get_emptyBlock();
		if( newdir_ino == -1 ){
			error_message("touch", org_path, -4);
			return;
		}

		si.sfi_direct[emptyDB] = newdir_ino;
		disk_write( &si, sd_cwd.sfd_ino );
	}

	disk_read( sd, si.sfi_direct[emptyDB] );

	sd[emptyDE].sfd_ino = newbie_ino;
	strncpy( sd[emptyDE].sfd_name, org_path, SFS_NAMELEN );

	disk_write( sd, si.sfi_direct[emptyDB] );

	si.sfi_size += sizeof(struct sfs_dir);
	disk_write( &si, sd_cwd.sfd_ino );	
}

void sfs_rmdir(const char* org_path) 
{
	// Error: No such file or directory

	// Error: Directory not empty

	// Error: Not a directory
}

void sfs_mv(const char* src_name, const char* dst_name) 
{
	int emptyDB, emptyDE; // Empty Direct Block, Empry Directory Entry
	struct sfs_inode si;
	disk_read( &si, sd_cwd.sfd_ino );

	// Error: dst_name already exists
	if( !get_emptyEntry( &si, &emptyDB, &emptyDE, dst_name ) ){
		error_message("mv", dst_name, -6);
		return;
	}

	// Error: No such file or directory
	if( get_emptyEntry( &si, &emptyDB, &emptyDE, src_name ) ){
		error_message("mv", src_name, -1);
		return;
	}

	struct sfs_dir sd[SFS_DENTRYPERBLOCK];
	disk_read( sd, si.sfi_direct[emptyDB] );
	strncpy( sd[emptyDE].sfd_name, dst_name, SFS_NAMELEN );
	disk_write( sd, si.sfi_direct[emptyDB] );

}

void sfs_rm(const char* path) 
{
	printf("Not Implemented\n");
}

void sfs_cpin(const char* local_path, const char* path) 
{
	printf("Not Implemented\n");
}

void sfs_cpout(const char* local_path, const char* path) 
{
	printf("Not Implemented\n");
}

void dump_inode(struct sfs_inode inode) {
	int i;
	struct sfs_dir dir_entry[SFS_DENTRYPERBLOCK];

	printf("size %d type %d direct ", inode.sfi_size, inode.sfi_type);
	for(i=0; i < SFS_NDIRECT; i++) {
		printf(" %d ", inode.sfi_direct[i]);
	}
	printf(" indirect %d",inode.sfi_indirect);
	printf("\n");

	if (inode.sfi_type == SFS_TYPE_DIR) {
		for(i=0; i < SFS_NDIRECT; i++) {
			if (inode.sfi_direct[i] == 0) break;
			disk_read(dir_entry, inode.sfi_direct[i]);
			dump_directory(dir_entry);
		}
	}

}

void dump_directory(struct sfs_dir dir_entry[]) {
	int i;
	struct sfs_inode inode;
	for(i=0; i < SFS_DENTRYPERBLOCK;i++) {
		printf("%d %s\n",dir_entry[i].sfd_ino, dir_entry[i].sfd_name);
		disk_read(&inode,dir_entry[i].sfd_ino);
		if (inode.sfi_type == SFS_TYPE_FILE) {
			printf("\t");
			dump_inode(inode);
		}
	}
}

void sfs_dump() {
	// dump the current directory structure
	struct sfs_inode c_inode;

	disk_read(&c_inode, sd_cwd.sfd_ino);
	printf("cwd inode %d name %s\n",sd_cwd.sfd_ino,sd_cwd.sfd_name);
	dump_inode(c_inode);
	printf("\n");

}
