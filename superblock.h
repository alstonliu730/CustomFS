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
 */
void* initSuperBlock(void* base, int nBlocks, int nInodes);

/**
 * Returns the file system type.
 */
int getMagic();

/**
 * Returns the number of Inodes in this File System.
 */
int getInodes();

/**
 * Returns the block number of the Inode Table. 
 */
int getInodeTable();

/**
 * Sets the given block number as the Inode Table.
 * 
 * @param bnum
 */
void setInodeTable(int bnum);

#endif
