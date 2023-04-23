// Inode manipulation routines.
//
// Feel free to use as inspiration. Provided as-is.

// based on cs3650 starter code
#ifndef INODE_H
#define INODE_H

// Required Libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "blocks.h"
#include "bitmap.h"

// Global Variables
#define INODE_SIZE sizeof(inode_t)
#define MAX_BLOCKS 12 // max number of blocks (Max file size = 48KB)

// inode structure
typedef struct inode {
  int refs;  // reference count (4B)
  int mode;  // permission & type (4B)
  int size;  // bytes contained (4B)
  int blocks; // blocks allocated (4B)
  int block[12]; // 12 direct block number (if max file size <= 48KB)
  int indirect; // single indirect block when file size >= 48KB
  time_t ctime; // when the file was created
  time_t atime; // when the file was last accessed
  time_t mtime; // when the file was last modified
} inode_t;

void print_inode(inode_t *node);
inode_t *get_inode(int inum);
int alloc_inode();
void free_inode(int inum);
int grow_inode(inode_t *node, int size);
int shrink_inode(inode_t *node, int size);
int inode_get_bnum(inode_t *node, int offset);
void *inode_get_block(inode_t *node, int file_bnum);

#endif