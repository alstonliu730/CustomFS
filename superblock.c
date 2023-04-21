#include "superblock.h"

// Initializes the superblock given the mapped memory location & several properties.
void* initSuperBlock(void* base, int nBlocks, int nInodes) {
    // create the meta data block
    superblock_t* fsMetaData = (superblock_t *) malloc(sizeof(superblock_t));

    // initialize the meta data block
    fsMetaData->magic = 0xf0f03410;
    fsMetaData->blocks = nBlocks;
    fsMetaData->blockSize = BLOCK_SIZE; // 4KB
    fsMetaData->inodes = nInodes;
    fsMetaData->inodeTable = 0; // required to use set function
    
    //Copy the superblock struct to the mapped memory
    memcpy(base, fsMetaData, sizeof(superblock_t));

    // free the allocated memory for the superblock struct
    free(fsMetaData);

    return base;
}