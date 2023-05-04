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
    new_inode->refs = 1;
    new_inode->size = 0;
    new_inode->mode = 0;
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
    
    // if the block sucessfully allocated the block
    if(new_inode->block[0] > 0) {
        new_inode->blocks++;
    } else {
        fprintf(stderr, "ERROR: alloc_inode() -> No available blocks to fill.\n");
        // TODO: make sure to reset this inode
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
    bitmap_put(get_inode_bitmap(), inum, 0); // set inode bit to free 
    printf("DEBUG: free_inode(%d)\n", inum);
}

// grows the inode by the given size in bytes
int grow_inode(inode_t *node, int size) {
    assert(node);
    assert(node->block[0] != -1);
    assert(size > 0);
    printf("DEBUG: grow_inode(%i) -> Called Function\n", size);

    // new_size of inodes
    int new_size = node->blocks * BLOCK_SIZE + size;
    printf("DEBUG: grow_inode(%i) -> new_size: %i\n", size, new_size);

    // its new size in blocks
    uint8_t nBlocks = bytes_to_blocks(new_size); 
    printf("DEBUG: grow_inode(%i) -> New Size in Blocks: %i\n", size, nBlocks);

    // adding to direct pointers
    if(node->blocks < MAX_BLOCKS) {
        int allocBlocks;
        // Case where the new number of blocks is greater than MAX_BLOCKS
        if(nBlocks >= MAX_BLOCKS) {
            allocBlocks = MAX_BLOCKS - node->blocks;
        } else {
            allocBlocks = nBlocks - node->blocks;
        }
        printf("DEBUG: grow_inode(%i) -> # of new blocks: %i\n", size, allocBlocks);
        // allocate the blocks to the direct pointers
        for(int ii = 0; ii < allocBlocks; ++ii) {
            node->block[node->blocks + ii] = alloc_block();
        }
        
        // update the number of blocks in the inode
        node->blocks += allocBlocks;
        printf("DEBUG: grow_inode(%i) -> # of blocks in this node: %i\n", size, node->blocks);
    }
    
    // adding to indirect pointers
    if (node->blocks >= MAX_BLOCKS || nBlocks > MAX_BLOCKS) {
        // create indirect pointer block if it doesn't exist
        if(node->indirect == -1) {
            node->indirect = alloc_block();
            if(!node->indirect) {
                node->blocks = nBlocks;
                return -1; // when fs can't allocate more blocks
            }
        }

        // create new blocks to the indirect pointers
        int* ptr_block = blocks_get_block(node->indirect);
        int allocBlocks = nBlocks - MAX_BLOCKS;
        for (int idx = node->blocks - MAX_BLOCKS; idx < allocBlocks; ++idx) {
            ptr_block[idx] = alloc_block();
            if(!node->block[idx]) {
                node->blocks += (idx + 1);
                return -1; // when fs can't allocate more blocks
            }
        }
    }

    return 0;
}

// shrinks the inode by the given size in bytes
int shrink_inode(inode_t *node, int size) {
    assert(node);
    assert(size > 0);
    printf("DEBUG: shrink_inode(%i) -> Called Function\n", size);
    // new size of inodes
    int new_size = node->blocks * BLOCK_SIZE - size;
    new_size = (new_size > 0) ? new_size : 0;

    // new size in blocks
    uint16_t new_size_blocks = bytes_to_blocks(new_size);
    printf("DEBUG: shirnk_inode(%i) -> New size in blocks: %i\n", size, new_size_blocks);

    // shrink the indirect pointers
    int* ptr_block = blocks_get_block(node->indirect);
    if (node->blocks >= MAX_BLOCKS) {
        printf("DEBUG: shrink_inode(%i) -> Shrinking indirect pointers.\n", size);
        assert(node->indirect != -1);
        int ind_blocks = node->blocks - MAX_BLOCKS;
        int target = (new_size_blocks > MAX_BLOCKS) ? (new_size_blocks - MAX_BLOCKS) : 0;
        int* ind_bnums = blocks_get_block(node->indirect);
        for(int ii = ind_blocks - 1; ii >= target; --ii) {
            free_block(ind_bnums[ii]);
        }
        node->blocks = (new_size_blocks > MAX_BLOCKS) ? new_size_blocks : MAX_BLOCKS;
    }

    // shrink the direct pointers
    if (node->blocks < MAX_BLOCKS) {
        printf("DEBUG: shrink_inode(%i) -> Shrinking direct pointers.\n", size);
        for(int ii = node->blocks - 1; ii >= node->blocks - new_size_blocks; --ii) {
            free_block(node->block[ii]);
            node->block[ii] = -1;
        }
        node->blocks = new_size_blocks;
    }

    assert(node->blocks == new_size_blocks);
    return 0;
}

// file block number is the offset in this inode in bytes
int inode_get_bnum(inode_t *node, int offset) {
    assert(offset >= 0);
    printf("DEBUG: inode_get_bnum(%i) -> Called Function\n", offset);
    int nBlocks = offset / BLOCK_SIZE;
    if(nBlocks < MAX_BLOCKS) {
        printf("DEBUG: inode_get_bnum(%i) -> Direct bnum: %i\n", offset, node->block[nBlocks]);
        return node->block[nBlocks];
    } else {
        int *ind_block = blocks_get_block(node->indirect);
        printf("DEBUG: inode_get_bnum(%i) -> Indirect bnum: %i\n", 
            offset, ind_block[nBlocks - MAX_BLOCKS]);
        return ind_block[nBlocks - MAX_BLOCKS];
    }
}

// returns the pointer to the block given the block index of the inode
void *inode_get_block(inode_t *node, int offset) {
    printf("DEBUG: inode_get_block(%i) -> Called Function\n", offset);
    void *ptr = blocks_get_block(inode_get_bnum(node, offset * BLOCK_SIZE));
    printf("DEBUG: inode_get_block(%i) -> %p\n",
        offset, ptr);
    return ptr;
}
