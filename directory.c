#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "directory.h"

#define nROOT 0

// initialize the directory inode
void directory_init() {
    int inum = alloc_inode();
    inode_t *root = get_inode(inum); //inode 0 is the root dir.
    root->mode = 40755;

    //DEBUG: Get root inode
    print_inode(root);
}

// Look through directories to find given name and return the inode number.
int directory_lookup(inode_t *di, const char *name) {
    // DEBUG:
    printf("+ directory_lookup(%s)\n", name);

    if (strcmp(name, "") == 0) {
        printf("Invalid Directory Lookup.\n");
        return -1;
    }

    // gets subdirectories
    dirent_t* subdir = inode_get_block(di, 0);
    
    // go through the directories
    for(int ii = 0; ii < di->refs; ++ii) {
        // if name matches
        if(!strcmp(name, subdir[ii].name) && subdir[ii].used) {
            printf("DEBUG: Inode Num: %i\n", subdir[ii].inum);
            return subdir[ii].inum; // return the inum
        }
    }

    // otherwise no such directory entry
    return -1;
}

// Add an entry to the directory with the given name and inum
int directory_put(inode_t *di, const char *name, int inum) {
    assert(di->mode == 40755);
    
    // get directory entries
    dirent_t *entries = inode_get_block(di, 0);
    
    // prepare the entry
    dirent_t new_entry;
    int nameLen = strlen(name) + 1;
    if(di->size + nameLen + sizeof(inum) > BLOCK_SIZE) {
        return -1;
    } else if (directory_lookup(di, name)) {
        printf("Entry already exist!\n");
        return 0;
    }

    // set properties of entry
    strncpy(new_entry.name, name, nameLen); // copy name to entry
    new_entry.inum = inum;
    new_entry.used = 1;
    
    // add new entry to the list of entries
    entries[di->refs + 1] = new_entry;
    di->size += sizeof(dirent_t);
    di->refs++;
    return 1; 
}

// Delete an entry from the directory with the given name
int directory_delete(inode_t *di, const char *name) {
    // get directory entires
    dirent_t* entries = inode_get_block(di, 0);

    // find the entry
    for(int ii = 0; ii < di->refs; ++ii) {
        // if name matches
        if(!strcmp(entries[ii].name, name) && entries[ii].used) {
            // found entry and delete any sub files
            delete_entry(entries[ii]);
            entries[ii].used = 0;
        }
    }
}

// RECURSIVE: deletes all data in the entry
int delete_entry(dirent_t entry) {
    // get the node from this entry
    inode_t *node = get_inode(entry.inum);
    
    // check if the entry is a file
    if(node->mode != 40755) {
        free_inode(entry.inum);
        return 1;
    } 
    // case where the entry is a directory
    else if(node->mode == 40755) {
        dirent_t *entries = inode_get_block(node, 0);
        for(int ii = 0; ii < node->refs; ++ii) {
            delete_entry(entries[ii]);
        }
        return 1;
    } else {
        // invalid file
        return 0; // empty entry
    }
}

// get list of names of the entries in the directory
slist_t *directory_list(const char *path) {
    int p_inum = get_inode_path(path);
    inode_t* node = get_inode(p_inum);
    assert(node->mode == 40755);

    dirent_t* entries = inode_get_block(node, 0);
    slist_t* names;
    for(int ii = 0; ii < node->refs; ++ii) {
        names = slist_cons(entries[ii].name, names);
    }

    return names;
}

// print all entries
void print_directory(inode_t *dd) {
    dirent_t* entries = inode_get_block(dd, 0);
    for(int ii = 0; ii < dd->refs; ++ii) {
        printf("%s ", entries[ii].name);
        //print_inode(get_inode(entries[ii].inum));
    }
}

// get inode from the given path
int get_inode_path(const char* path) {
    assert(path[0] == '/');

    if(strcmp(path, "/") == 0) {
        return nROOT;
    }

    slist_t* path_list = slist_explode(path += 1, '/');
    slist_t* tmp = path_list;
    
    int count = 0;
    int inum = nROOT;
    while(tmp) {
        //DEBUG: Get Path Names
        printf("DEBUG: Path Name %i: %s\n", count, path_list->data);
        inum = directory_lookup(get_inode(inum), path_list->data);
        printf("DEBUG: Inum: %i", inum);
        if(inum < 0) {
            slist_free(path_list);
            printf("Failed to find directory.\n");
            return -1;
        }
        tmp = tmp->next;
        count++;
    }
    slist_free(path_list);
    printf("+ get_inode_path(%s) -> %i\n", path, inum);
    return inum;
}