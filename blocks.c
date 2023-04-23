/**
 * @file blocks.c
 * @author CS3650 staff
 *
 * Implementation of a block-based abstraction over a disk image file.
 */
#define _GNU_SOURCE

#include <string.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bitmap.h"
#include "blocks.h"
#include "inode.h"

static int blocks_fd = -1;
static void *blocks_base = 0;

// Get the number of blocks needed to store the given number of bytes.
int bytes_to_blocks(int bytes) {
  int quo = bytes / BLOCK_SIZE;
  int rem = bytes % BLOCK_SIZE;
  if (rem == 0) {
    return quo;
  } else {
    return quo + 1;
  }
}

// Load and initialize the given disk image.
void blocks_init(const char *image_path) {
  blocks_fd = open(image_path, O_CREAT | O_RDWR, 0644);
  assert(blocks_fd != -1);

  // make sure the disk image is exactly 1MB
  int rv = ftruncate(blocks_fd, NUFS_SIZE);
  assert(rv == 0);

  // map the image to memory
  blocks_base =
      mmap(0, NUFS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, blocks_fd, 0);
  assert(blocks_base != MAP_FAILED);

  // DEBUGGING: make sure bitmap is free
  printf("Bitmap for blocks (initial 10):\n");
  bitmap_print(get_blocks_bitmap(), 10);
  printf("\n");
  
  // block 1 stores the block bitmap and the inode bitmap
  void *bbm = get_blocks_bitmap();
  bitmap_put(bbm, 0, 1); // set block 0 to used

  // DEBUGGING: make sure first two bit is used and only the first two
  bitmap_print(bbm, 5);
  printf("\n");
  
  // block 2 - max_blocks stores the inode table
  init_inode_table();
}

// Close the disk image.
void blocks_free() {
  int rv = munmap(blocks_base, NUFS_SIZE);
  assert(rv == 0);
}

// Get the given block, returning a pointer to its start.
void *blocks_get_block(int bnum) { return blocks_base + BLOCK_SIZE * bnum; }

// Return a pointer to the beginning of the block bitmap.
// The size is BLOCK_BITMAP_SIZE bytes.
void *get_blocks_bitmap() { return blocks_get_block(0); }

// Return a pointer to the beginning of the inode table bitmap.
void *get_inode_bitmap() {
  uint8_t *block = blocks_get_block(0);

  // The inode bitmap is stored immediately after the block bitmap
  return (void *) (block + BLOCK_BITMAP_SIZE);
}
// Return a pointer to the beginning of the inode table.
void *get_inode_table() {
  return blocks_get_block(1);
}

// Initialize the inode table and return the beginning of the table.
void *init_inode_table() {
  // Assumes the first two blocks are in use for superblock and bitmap
  assert(bitmap_get(get_blocks_bitmap(), 0));

  int max_blocks = bytes_to_blocks(INODE_LIMIT * sizeof(inode_t));
  printf("Max Blocks: %u\n", max_blocks);

  // DEBUG: Make sure the next two blocks are free
  printf("Bitmap for blocks:\n");
  bitmap_print(get_blocks_bitmap(), 5);
  printf("\n");

  // allocate blocks for 'max_blocks' blocks
  for(int ii = 0; ii < max_blocks; ++ii) {
    // DEBUG: Make sure the next two blocks are free
    bitmap_print(get_blocks_bitmap(), 5);

    // make sure the bitmaps of those blocks are free
    if(!bitmap_get(get_blocks_bitmap(), ii + 1)) {
      alloc_block();
    }
  }

  // make sure the first max_blocks + 1 bits are in-use
  for(int ii = 0; ii < max_blocks + 1; ++ii) {
    assert(bitmap_get(get_blocks_bitmap(), ii));
  }

  return get_inode_table();
}

// Allocate a new block and return its index.
int alloc_block() {
  void *bbm = get_blocks_bitmap();

  // first two are being used for superblock and bitmap
  for (int ii = 1; ii < BLOCK_COUNT; ++ii) {
    if (!bitmap_get(bbm, ii)) {
      bitmap_put(bbm, ii, 1);
      printf("DEBUG: alloc_block() -> %d\n", ii);
      return ii;
    }
  }

  fprintf(stderr, "ERROR: alloc_block() -> (-1)\n");
  return -1;
}

// Deallocate the block with the given index.
void free_block(int bnum) {
  printf("DEBUG: free_block(%d)\n", bnum);
  void *bbm = get_blocks_bitmap();
  bitmap_put(bbm, bnum, 0);
}
