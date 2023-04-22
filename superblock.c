
#include <string.h>
#include "superblock.h"


// Initializes the superblock given the mapped memory location & several properties.
void* initSuperBlock(void* base, int nBlocks, int nInodes, int tableBlock) {
    // create the meta data block
    superblock_t* fsMetaData = (superblock_t *) malloc(sizeof(superblock_t));

    // initialize the meta data block
    fsMetaData->magic = 0xf0f03410;
    fsMetaData->blocks = nBlocks;
    fsMetaData->blockSize = 4096; // 4KB
    fsMetaData->inodes = nInodes;
    fsMetaData->inodeTable = tableBlock;
    
    //Copy the superblock struct to the mapped memory
    memcpy(base, fsMetaData, sizeof(superblock_t));

    // free the allocated memory for the superblock struct
    free(fsMetaData);

    return base;
}

// returns the superblock object at the given pointer
superblock_t* getSuperBlock(void* ptr) {
    return (superblock_t *) ptr;
}

// returns the file system type
int getMagic(superblock_t* sb) {
    return sb->magic;
}

// returns the number of inodes
int getInodes(superblock_t* sb) {
    return sb->inodes;
}

// returns the block number of the inode table
int getInodeTable(superblock_t* sb) {
    return sb->inodeTable;
}

// returns the number of blocks
int getNumBlocks(superblock_t* sb) {
    return sb->blocks;
}