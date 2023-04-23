/**
 * @file inode.c
 * @author Alston Liu
 * 
 * Implementation of Inode Data Structure
 */
#include <assert.h>

#include "inode.h"

// Prints information about the inode
void print_inode(inode_t *node) {
    assert(node);
    printf("Inode Address: %p\n", node);
    printf("# of refs: %d\n", node->refs);
    printf("Permission & File Type: %d\n", node->mode);
    printf("Size: %d\n", node->size);
    for(int ii = 0; ii < MAX_BLOCKS; ++ii)
        printf("Block bnum %d: %i\n", ii,(node->block[ii]));
    printf("Indirect bnum: %d\n", node->indirect);
}

// gets the inode from the given index number
// table is from block 
inode_t *get_inode(int inum) {
    assert(inum < INODE_LIMIT);
    inode_t* table = (inode_t *) get_inode_table();

    printf("DEBUG: get_inode(%i) -> %p\n", inum, table+inum);
    return table + inum;
}

// creates a new inode and returns the index number of the inode.
int alloc_inode() {
    printf("DEBUG: alloc_inode() -> called function!\n");
    int inum = -1;
    // find an available inode from the bitmap
    for(int ii = 0; ii < INODE_LIMIT; ++ii) {
        if (!bitmap_get(get_inode_bitmap(), ii)) {
            // set the inode status as used
            bitmap_put(get_inode_bitmap(), ii, 1);
            inum = ii;
            break;
        }
    }

    // if theres no more inodes left
    if(inum == -1) {
        fprintf(stderr, "ERROR: alloc_inode() -> No more inodes left!\n");
        return inum;
    }
    
    // Initialize inode information
    inode_t* new_inode = get_inode(inum);
    memset(new_inode, 0, sizeof(inode_t)); // clear any previous information
    new_inode->refs = 0;
    new_inode->size = 0;
    new_inode->mode = 10644;
    new_inode->blocks = 0;
    new_inode->indirect = -1; // nonexistent bnum

    // set time of creation
    time_t curr = time(NULL);
    new_inode->ctime = curr;
    new_inode->atime = curr;
    new_inode->mtime = curr;

    // set every block bnum to -1
    for(int ii = 0; ii < MAX_BLOCKS; ++ii) {
        new_inode->block[ii] = -1; 
    }

    // set the first block as a new block
    new_inode->block[0] = alloc_block();
    
    // if the block can allocate more
    if(new_inode->block[0] > 0) {
        new_inode->blocks++;
    } else {
        fprintf(stderr, "ERROR: alloc_inode() -> No available blocks to fill.\n");
        return -1;
    }
    printf("DEBUG: alloc_inode() -> %d\n", inum);

    // return index number
    return inum;
}

// clear inode bitmap at the given inum and clear block location
void free_inode(int inum) {
    assert(bitmap_get(get_inode_bitmap(), inum));
    
    // clear the block location in the inode
    inode_t* node = get_inode(inum);

    // Shrink the inode size to 0
    shrink_inode(node, node->blocks*BLOCK_SIZE);

    // clear the bit at the given index number
    bitmap_put(get_inode_bitmap(), inum, 0); 
    printf("DEBUG: free_inode(%d)\n", inum);
}

// grows the inode by the given size in bytes
int grow_inode(inode_t *node, int size) {
    assert(node);
    assert(node->block[0] != -1);
    assert(size > 0);
    
    // new_size of inodes
    int new_size = node->blocks * BLOCK_SIZE + size;

    // its new size in blocks
    uint16_t nBlocks = bytes_to_blocks(new_size); 

    // adding to direct pointers
    if (node->blocks < 12) {
        for(int ii = node->blocks-1; ii < nBlocks && ii < MAX_BLOCKS; ++ii) {
            node->block[ii] = alloc_block();
            if(!node->block[ii]) {
                node->blocks = ii + 1;
                return 0; // when fs can't allocate more blocks
            }
        }
    }
    
    // adding to indirect pointers
    if (node->blocks >= 12 || nBlocks > 12) {
        // create indirect pointer block if it doesn't exist
        if(node->indirect == -1) {
            node->indirect = alloc_block();
            if(!node->indirect) {
                node->blocks = nBlocks;
                return 0; // when fs can't allocate more blocks
            }
        }

        // create new blocks to the indirect pointers
        int* ptr_block = blocks_get_block(node->indirect);
        for (int idx = node->blocks - 12; idx < (nBlocks - 12); ++idx) {
            ptr_block[idx] = alloc_block();
            if(!node->block[idx]) {
                node->blocks += (idx + 1);
                return 0; // when fs can't allocate more blocks
            }
        }
    }

    node->blocks = nBlocks;
    return 1;
}

// shrinks the inode by the given size in bytes
int shrink_inode(inode_t *node, int size) {
    assert(node);
    assert(size > 0);

    // new size of inodes
    int new_size = node->blocks * BLOCK_SIZE - size;
    new_size = (new_size > 0) ? new_size : 0;

    // new size in blocks
    uint16_t nBlocks = bytes_to_blocks(new_size);
    
    // shrink the indirect pointers
    int* ptr_block = blocks_get_block(node->indirect);
    if (node->blocks > 12) {
        for(int idx = node->blocks - 12; idx >= 0 && idx >= (nBlocks - 12); --idx) {
            free_block(ptr_block[idx]);
            ptr_block[idx] = -1;
        }
    }

    // shrink the direct pointers
    if (nBlocks < 12) {
        for(int ii = node->blocks - 1; ii > nBlocks; --ii) {
            free_block(node->block[ii]);
            node->block[ii] = -1;
        }
    }

    node->blocks = nBlocks;
    return 1;
}

// file block number is the offset in this inode in bytes
int inode_get_bnum(inode_t *node, int offset) {
    assert(offset >= 0);
    printf("DEBUG: inode_get_bnum(%i) -> Called Function\n", offset);
    int nBlocks = offset / BLOCK_SIZE;
    if(nBlocks < 12) {
        printf("DEBUG: inode_get_bnum(%i) -> Direct bnum: %i\n", offset, nBlocks);
        return node->block[nBlocks];
    } else {
        int *ind_block = blocks_get_block(node->indirect);
        printf("DEBUG: inode_get_bnum(%i) -> Indirect bnum: %i\n", offset, ind_block[nBlocks-12]);
        return ind_block[nBlocks - 12];
    }
}

// returns the pointer to the block given the block index of the inode
void *inode_get_block(inode_t *node, int offset) {
    printf("DEBUG: inode_get_block(%i) -> %p\n",
        offset, blocks_get_block(inode_get_bnum(node, offset)));
    return blocks_get_block(inode_get_bnum(node, offset));
}
