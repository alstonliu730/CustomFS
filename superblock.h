/*
*/
#ifndef SUPERBLOCK_H
#define SUPERBLOCK_H

#define BLOCK_SIZE 4096 //4KB

// File System MetaData
typedef struct superblock {
    int magic; // Type of file system
    int blocks; // # of data blocks
    int blockSize; // size of each block
    int inodes; // # of inodes 
    int inodeTable; // block number to the inode table
} superblock_t; // Size: 20B

/**
 * Populates the superblock with the given base of the memory.
 * 
 * @param base - base of the mapped memory
 * @param nBlocks - the total number of blocks
 * @param nInodes - the total number of Inodes
 * @param tableBlock - the block number of the inode table
 */
void* initSuperBlock(void* base, int nBlocks, int nInodes, int tableBlock);

/**
 * Returns the superblock object in the given pointer.
 * Assuming the pointer points to the superblock memory.
 * 
 * @return
*/
superblock_t* getSuperBlock(void* ptr);

/**
 * Returns the file system type.
 * 
 * @return
 */
int getMagic(superblock_t* sb);

/**
 * Returns the number of Inodes in this File System.
 * 
 * @return
 */
int getInodes(superblock_t* sb);

/**
 * Returns the block number of the Inode Table.
 * 
 * @return 
 */
int getInodeTable(superblock_t* sb);

/**
 * Returns the number of blocks
 * 
 * @return 
 */
int getNumBlocks(superblock_t* sb);

#endif
