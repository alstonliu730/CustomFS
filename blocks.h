/**
 * @file blocks.h
 * @author CS3650 staff
 *
 * A block-based abstraction over a disk image file.
 *
 * The disk image is mmapped, so block data is accessed using pointers.
 */
#ifndef BLOCKS_H
#define BLOCKS_H

#define KBtoB 1024
#define BLOCK_SIZE (4 * KBtoB) //4KB (4096 B)

//#define INODE_LIMIT BLOCK_SIZE - (BLOCK_BITMAP_SIZE * 8)
// max number of inodes
#define INODE_LIMIT 64

// we split the "disk" into 256 blocks
#define BLOCK_COUNT 256 

// = 1MB
#define NUFS_SIZE (BLOCK_SIZE * BLOCK_COUNT) 

// Note: assumes block count is divisible by 8
#define BLOCK_BITMAP_SIZE (BLOCK_COUNT / 8)

// ERROR CODES
#define NOSPACE -1

/** 
 * Compute the number of blocks needed to store the given number of bytes.
 *
 * @param bytes Size of data to store in bytes.
 *
 * @return Number of blocks needed to store the given number of bytes.
 */
int bytes_to_blocks(int bytes);

/**
 * Load and initialize the given disk image.
 *
 * @param image_path Path to the disk image file.
 */
void blocks_init(const char *image_path);

/**
 * Close the disk image.
 */
void blocks_free();

/**
 * Get the block with the given index, returning a pointer to its start.
 *
 * @param bnum Block number (index).
 *
 * @return Pointer to the beginning of the block in memory.
 */
void *blocks_get_block(int bnum);

/**
 * Return a pointer to the beginning of the block bitmap.
 *
 * @return A pointer to the beginning of the free blocks bitmap.
 */
void *get_blocks_bitmap();

/**
 * Return a pointer to the beginning of the inode table bitmap.
 *
 * @return A pointer to the beginning of the free inode bitmap.
 */
void *get_inode_bitmap();

/**
 * Return a pointer to the beginning of the inode table.
 * 
 * @return A pointer to the beginning of the inode table
 */
void *get_inode_table();

/**
 * Initialize the Inode Table and return the beginning of the table.
 * 
 * @return A pointer to the beginning of the inode table
 */
void *init_inode_table();

/**
 * Allocate a new block and return its number.
 *
 * Grabs the first unused block and marks it as allocated.
 *
 * @return The index of the newly allocated block.
 */
int alloc_block();

/**
 * Deallocate the block with the given number.
 *
 * @param bnun The block number to deallocate.
 */
void free_block(int bnum);

#endif
