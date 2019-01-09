// NAME: Anthony Humay, Clayton Green
// EMAIL: ahumay@ucla.edu, clayton.green26@gmail.com
// ID: 304731856, 404546151
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <getopt.h>
#include <limits.h>
#include "ext2_fs.h"

// Image
char* ext2_img;
int ext2_fd = 0;

// For analyzing image
struct ext2_super_block super_block;
struct ext2_group_desc *groups = NULL;
int *inodes = NULL;
unsigned int b_size;
int num_groups;
unsigned int numBlocks;

// Other
int SIZESUPERBLOCK = sizeof(struct ext2_super_block);
int SIZEGROUPDESC = sizeof(struct ext2_group_desc);
int SIZEINODE = sizeof(struct ext2_inode);
int SIZEDIRENTRY = sizeof(struct ext2_dir_entry);
int SIZE8int = sizeof(uint8_t);
int SIZE32int = sizeof(unsigned int);

///////////////////////////////////////////////
///////////////// DECLARATIONS ////////////////////
///////////////////////////////////////////////

void exit_handler();

void err_handler(char* message, int exit_code);

int checkArgs(int argc, char** argv);

void analyze_superblock();

void analyze_groups();

void analyze_free_block_and_inode_entries();

void ehandler(char indicator, int errorCode);

void analyze_indirect_directories(unsigned int nodeID, unsigned int offset, unsigned int blockID, int lvl);

void analyze_directories(unsigned int nodeID, struct ext2_inode * inode);

void recurse_indirects(unsigned int nodeID, unsigned int blockID, unsigned int offset, int lvl);

void analyze_indirects(unsigned int nodeID, struct ext2_inode * inode);

void analyze_inodes();

///////////////////////////////////////////////
/////////////////// MAIN //////////////////////
///////////////////////////////////////////////

int main(int argc, char ** argv) {
	if(argc != 2) {
		err_handler("Usage: ./lab3a DISK_IMG", 1);
	}

	if(checkArgs(argc, argv) < 0) {
		err_handler("Usage: ./lab3a DISK_IMG", 1);
	}

	atexit(exit_handler);

	ext2_img = argv[1];
	ext2_fd = open(ext2_img, O_RDONLY);

	if(ext2_fd < 0) {
		err_handler("unable to open EXT2 file system image", 2);
	}

	analyze_superblock();
	analyze_groups();
	analyze_free_block_and_inode_entries();
	analyze_inodes();

	exit(0);
}


///////////////////////////////////////////////
///////////////// HANDLERS ////////////////////
///////////////////////////////////////////////

void exit_handler() {
	if(ext2_fd > 0) {
		close(ext2_fd);
	}
	if(groups != NULL) {
		free(groups);
	}
	if(inodes != NULL) {
		free(inodes);
	}

}

void err_handler(char* message, int exit_code) {
	fprintf(stderr, "Error: %s \n", message);
	exit(exit_code);
}

///////////////////////////////////////////////
////////////////// HELPERS ////////////////////
///////////////////////////////////////////////

int checkArgs(int argc, char** argv) {
	static struct option long_options[] = {
        {0,0,0,0}
    };

    while (1){
    	int returned;
		int optIndex;
        returned = getopt_long(argc, argv, "", long_options, &optIndex);
        //fprintf(stdout, "Returned is %d \n", returned);
        switch(returned) {
        	case -1:
                return 0;
            default:
                return -1;
        }
	}
}

void analyze_superblock() {
	if(pread(ext2_fd, &super_block, SIZESUPERBLOCK, 1024) < 0) {
		err_handler("unable to read superblock", 2);
	}

	b_size = EXT2_MIN_BLOCK_SIZE << super_block.s_log_block_size;

	fprintf(stdout, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", super_block.s_blocks_count, super_block.s_inodes_count, b_size, super_block.s_inode_size, super_block.s_blocks_per_group, super_block.s_inodes_per_group, super_block.s_first_ino);
}

void analyze_groups() {
	num_groups = 1 + ((super_block.s_blocks_count - 1) / super_block.s_blocks_per_group);

	int total_size = SIZEGROUPDESC * num_groups;
	groups = malloc(total_size);
	if(groups == NULL) {
		err_handler("unable to allocate memory", 2);
	}

	if(pread(ext2_fd, groups, total_size, (1024 + b_size)) < 0) {
		err_handler("unable to read groups", 2);
	}

	unsigned int num_blocks = super_block.s_blocks_count, num_inodes = super_block.s_inodes_count;

	int i;
	for(i = 0; i < num_groups; i++) {
		int blocks_in_cur_group, inodes_in_cur_group;

		if(num_blocks < super_block.s_blocks_per_group) {
			blocks_in_cur_group = num_blocks;
		}
		else {
			blocks_in_cur_group = super_block.s_blocks_per_group;
		}

		if(num_inodes < super_block.s_inodes_per_group) {
			inodes_in_cur_group = num_inodes;
		}
		else {
			inodes_in_cur_group = super_block.s_inodes_per_group;
		}

		fprintf(stdout, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", i, blocks_in_cur_group, inodes_in_cur_group, groups[i].bg_free_blocks_count, groups[i].bg_free_inodes_count, groups[i].bg_block_bitmap, groups[i].bg_inode_bitmap, groups[i].bg_inode_table);

		num_inodes = num_inodes - super_block.s_inodes_per_group;
		num_blocks = num_blocks - super_block.s_blocks_per_group;
	}
}

void analyze_free_block_and_inode_entries() {
	int i, m;
	unsigned int k;
	inodes = malloc(num_groups * b_size * SIZE8int);
	if(inodes == NULL) {
		err_handler("unable to allocate for inodes", 2);
	}

	for(i = 0; i < num_groups; i++) {
		for(k = 0; k < b_size; k++) {
			uint8_t block_byte, inode_byte;

			if(pread(ext2_fd, &block_byte, 1, b_size * groups[i].bg_block_bitmap + k) < 0) {
				err_handler("unable to read block_byte", 2);
			}

			if(pread(ext2_fd, &inode_byte, 1, b_size * groups[i].bg_inode_bitmap + k) < 0) {
				err_handler("unable to read inode_byte", 2);
			}

			inodes[i + k] = inode_byte;

			int mask = 1;
			for(m = 0; m < 8; m++) {
				if( !(block_byte & mask)) {
					int free_block_num = (i * super_block.s_blocks_per_group) + (k * 8) + (m + 1);
					fprintf(stdout, "BFREE,%d\n", free_block_num);
				}

				if( !(inode_byte & mask)) {
					int free_inode_num = (i * super_block.s_inodes_per_group) + (k * 8) + (m + 1);
					fprintf(stdout, "IFREE,%d\n", free_inode_num);
				}

				mask = mask << 1;
			}
		}
	}
}

void ehandler(char indicator, int errorCode){
	/*
	Sorry for the extra error handler, I (Anthony) made my own and Clayton made his own.
	*/
	if (errorCode < 0){
		if (indicator == 'a'){
			fprintf(stderr, "\nUnknown or invalid argument.\n");
		}
		if (indicator == 'w'){
			fprintf(stderr, "\nFailure writing.\n");
		}
		if (indicator == 'r'){
			fprintf(stderr, "\nFailure reading.\n");
		}
		if (indicator == 'p'){
			fprintf(stderr, "\nFailure trying to pread().\n");
		}
		exit(2);
	}
}

void analyze_indirects(unsigned int nodeID, struct ext2_inode * inode){
	numBlocks = b_size / SIZE32int;
	ehandler('r', numBlocks);
	int INDBlockFlag = inode -> i_block[EXT2_IND_BLOCK];
	int DINDBlockFlag = inode -> i_block[EXT2_DIND_BLOCK];
	int TINDBlockFlag = inode -> i_block[EXT2_TIND_BLOCK];
	int thirdLevOffset = numBlocks * numBlocks;

	if (INDBlockFlag != 0){
		recurse_indirects(nodeID, INDBlockFlag, EXT2_NDIR_BLOCKS, 1);
	}
	if (DINDBlockFlag != 0){
		recurse_indirects(nodeID, DINDBlockFlag, EXT2_NDIR_BLOCKS + numBlocks, 2);
	}
	if (TINDBlockFlag != 0){
		recurse_indirects(nodeID, TINDBlockFlag, EXT2_NDIR_BLOCKS + numBlocks + thirdLevOffset, 3);
	}
}

void recurse_indirects(unsigned int nodeID, unsigned int blockID, unsigned int offset, int lvl){
	numBlocks = b_size / SIZE32int;
	unsigned int space[numBlocks];
	int readOffset = 1024 + (blockID - 1) * b_size;
	ehandler('r', readOffset);
	ehandler('p', pread(ext2_fd, space, b_size, readOffset));
	unsigned int i = 0;
	while (i != numBlocks){
		if (space[i] != 0){
			printf("INDIRECT,%u,%d,%u,%u,%u\n", nodeID, lvl, offset, blockID, space[i]);
		}

		int thirdLevOffset = numBlocks * numBlocks;
		if (lvl == 1){
			offset = offset + 1;
		} else if (lvl == 2){
			recurse_indirects(nodeID, space[i], offset, lvl - 1);
			offset += numBlocks;
		} else if (lvl == 3){
			recurse_indirects(nodeID, space[i], offset, lvl - 1);
			offset += thirdLevOffset;
		}
		i++;
	}
}

void analyze_indirect_directories(unsigned int nodeID, unsigned int offset, unsigned int blockID, int lvl){
	numBlocks = b_size / SIZE32int;
	unsigned int space[numBlocks];
	int analyzedFlag = 0;
	int readOffset = 1024 + (blockID - 1) * b_size;
	ehandler('r', readOffset);
	ehandler('p', pread(ext2_fd, space, b_size, readOffset));
	if (lvl == 1){
		struct ext2_dir_entry curDirectory;
		unsigned int i = 0; 
		while (i != numBlocks){
			if (space[i] != 0){
				unsigned int adjustment = 0;
				int blockOffset = 1024 + (space[i] - 1) * b_size;
				ehandler('p', pread(ext2_fd, &curDirectory, SIZEDIRENTRY, blockOffset + adjustment));
				while (curDirectory.file_type != 0 && !analyzedFlag && b_size > adjustment){
					int nameLength = curDirectory.name_len;
					int recLength = curDirectory.rec_len;
					if (curDirectory.inode != 0){
						char curName[EXT2_NAME_LEN + 1];
						memcpy(curName, curDirectory.name, nameLength);
						curName[curDirectory.name_len] = '\0';
						printf("DIRENT,%u,%u,%u,%u,%u,'%s'\n", nodeID, offset, curDirectory.inode, recLength, nameLength, curName);
					}
					offset += recLength;
					adjustment += recLength;
					int blockOffset2 = 1024 + (space[i] - 1) * b_size;
					ehandler('p', pread(ext2_fd, &curDirectory, SIZEDIRENTRY, blockOffset2 + adjustment));
				}
			}
			i++;
		}
	} else if (lvl == 2 || lvl == 3){
		unsigned int i = 0;
		while (i != numBlocks){
			if (space[i] != 0){
				analyze_indirect_directories(nodeID, offset, space[i], lvl - 1);
			}
			i++;
		}
	}
}

void analyze_directories(unsigned int nodeID, struct ext2_inode * inode){
	struct ext2_dir_entry curDirectory;
	int analyzedFlag = 0;
	unsigned int i = 0;
	unsigned int offset = 0;
	while (i != EXT2_NDIR_BLOCKS){
		unsigned int adjustment = 0;
		int readOffset = 1024 + ((inode -> i_block[i]) - 1) * b_size;
		ehandler('r', readOffset);
		ehandler('p', pread(ext2_fd, &curDirectory, SIZEDIRENTRY, readOffset + adjustment));
		while (adjustment < b_size && !analyzedFlag && curDirectory.file_type != 0){
			int nameLength = curDirectory.name_len;
			int recLength = curDirectory.rec_len;
			if (curDirectory.inode != 0){
				char curName[EXT2_NAME_LEN + 1];
				memcpy(curName, curDirectory.name, nameLength);
				curName[curDirectory.name_len] = '\0';
				printf("DIRENT,%u,%u,%u,%u,%u,'%s'\n", nodeID, offset, curDirectory.inode, recLength, nameLength, curName);
			}
			offset += recLength;
			adjustment += recLength;
			int blockOffset = 1024 + (inode -> i_block[i] - 1) * b_size;
			ehandler('p', pread(ext2_fd, &curDirectory, SIZEDIRENTRY, blockOffset + adjustment));
		}
		i++;
	}

	int INDBlockFlag = inode -> i_block[EXT2_IND_BLOCK];
	int DINDBlockFlag = inode -> i_block[EXT2_DIND_BLOCK];
	int TINDBlockFlag = inode -> i_block[EXT2_TIND_BLOCK];
	if (INDBlockFlag != 0){
		analyze_indirect_directories(nodeID, offset, INDBlockFlag, 1);
	}
	if (DINDBlockFlag != 0){
		analyze_indirect_directories(nodeID, offset, DINDBlockFlag, 2);
	}
	if (TINDBlockFlag != 0){
		analyze_indirect_directories(nodeID, offset, TINDBlockFlag, 3);
	}
}

void analyze_inodes() {
	struct ext2_inode inode;
	int i, m;
	unsigned int z, k;
	char type;

	for (i = 0; i < num_groups; i++) {
		for (z = 2; z < super_block.s_inodes_count; z++) {
			int val = 0, found = 0;
			for (k = 0 ; k < b_size; k++) {
				uint8_t test = inodes[i + k];
				int mask = 1;
				for (m = 0; m < 8; m++) {
					unsigned long node =  (k * 8) + (m + 1) + (i * super_block.s_inodes_per_group);
					if (node == z) {
						if ((test & mask) == 0) {
							val = 0;
							found = 1;
						}
						else {
							val = 1;
							found = 1;
						}
						break;
					}
					mask = mask << 1;
				}
			}

			if (!found) {
				err_handler("corrupted data", 2);
			}
			else if (!val) {
				continue;
			}

			off_t offset = (1024 + (groups[i].bg_inode_table - 1) * b_size) + (SIZEINODE * (z - 1));
			if (pread(ext2_fd, &inode, SIZEINODE, offset) < 0) {
				err_handler("unable to read image", 2);
			}

			// get mode
			uint16_t mode = (inode.i_mode & (0x01C0 | 0x0038 | 0x0007));

			// get times
			char times[3][100];
			time_t time_vals[3];
			time_vals[0] = inode.i_ctime;
			time_vals[1] = inode.i_mtime;
			time_vals[2] = inode.i_atime;

			int v;
			for(v = 0; v < 3; v++) {
				struct tm gmtime_val = *gmtime(&time_vals[v]);
				strftime(times[v], 100, "%m/%d/%y %H:%M:%S", &gmtime_val);
			}

			// get type
			if (S_ISREG(inode.i_mode)) {
				type = 'f';
			}
			else if (S_ISDIR(inode.i_mode)) {
				type = 'd';
			} 
			else if (S_ISLNK(inode.i_mode)) {
				type = 's';
			}
			else {
				type = '?';
			}

			fprintf(stdout, "INODE,%d,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u", z, type, mode, inode.i_uid, inode.i_gid, inode.i_links_count, /*i_ctime*/times[0], /*i_mtime*/times[1], /*i_atime*/times[2], inode.i_size, inode.i_blocks);

			for (k = 0; k < EXT2_N_BLOCKS; k++) {
				fprintf(stdout, ",%u", inode.i_block[k]);
			}
			fprintf(stdout, "\n");

			if (type == 'd'){
				analyze_directories(z, &inode);
			}
			if (type == 'd' || type == 'f'){
				analyze_indirects(z, &inode);
			}

			if (z == 2) {
				z = super_block.s_first_ino - 1;
			}
		}
	}
}