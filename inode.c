/**
 * @file inode.c
 * @author Alston Liu
 * 
 * Implementation of Inode Data Structure
 */

#include "inode.h"
#include "blocks.h"
#include "bitmap.h"

// Prints information about the inode
void print_inode(inode_t *node) {
    printf("Inode Address: %p\n", node);
    printf("# of refs: %d\n", node->refs);
    printf("Permission & File Type: %d\n", node->mode);
    printf("Size: %d\n", node->size);
    printf("Block pointer: %p\n", node->block);
}

// get the inode from the given index number
inode_t *get_inode(int inum) {
    inode_t* table = get_inode_bitmap();
}
int alloc_inode();
void free_inode(int inum);
int grow_inode(inode_t *node, int size);
int shrink_inode(inode_t *node, int size);
int inode_get_bnum(inode_t *node, int file_bnum);