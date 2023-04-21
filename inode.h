// Inode manipulation routines.
//
// Feel free to use as inspiration. Provided as-is.

// based on cs3650 starter code
#ifndef INODE_H
#define INODE_H

#include "blocks.h"

#define INODE_SIZE sizeof(inode_t)

typedef struct inode {
  int refs;  // reference count (4B)
  int mode;  // permission & type (4B)
  int size;  // bytes (4B)
  int block; // single block pointer (if max file size <= 4K)
} inode_t;

void print_inode(inode_t *node);
inode_t *get_inode(int inum);
int alloc_inode();
void free_inode(int inum);
int grow_inode(inode_t *node, int size);
int shrink_inode(inode_t *node, int size);
int inode_get_bnum(inode_t *node, int file_bnum);

#endif