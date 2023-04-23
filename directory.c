#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "directory.h"

#define nROOT 0

// initialize the directory inode
void directory_init() {
    printf("---Creating root directory!---\n");
    int inum = alloc_inode();
    inode_t *root = get_inode(inum); //inode 0 is the root dir.
    root->mode = 40755;

    // Configure self reference and parent reference
    dirent_t* entries = inode_get_block(root, 0);

    // Create self reference
    entries[0].inum = inum;
    strcpy(entries[0].name, ".");
    entries[0].used = 1;
    
    // Create parent reference
    entries[1].inum = inum;
    strcpy(entries[1].name, "..");
    entries[1].used = 1;
    
    // update inode
    root->refs = 2;
    root->size += 2*(sizeof(dirent_t));

    //DEBUG: Get root inode
    print_inode(root);
    printf("--- Finished creating root ---\n");
}

// Look through directories to find given name and return the inode number.
int directory_lookup(inode_t *di, const char *name) {
    // DEBUG:
    printf("DEBUG: directory_lookup(%s) -> Called function\n", name);

    if (strcmp(name, "") == 0) {
        fprintf(stderr, "ERROR: directory_lookup(%s) -> Given name is empty.\n", name);
        return -1;
    }

    // gets subdirectories
    dirent_t* subdir = inode_get_block(di, 0);
    
    // go through the directories
    for(int ii = 0; ii < di->refs; ++ii) {
        // if name matches
        if(!strcmp(name, subdir[ii].name) && subdir[ii].used) {
            printf("DEBUG: directory_lookup(%s) -> Inode Num: %i.\n", name, subdir[ii].inum);
            return subdir[ii].inum; // return the inum
        }
    }
    
    fprintf(stderr, "ERROR: directory_lookup(%s) -> No such directory entry.\n", name);
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
        fprintf(stderr, "ERROR: directory_put(%s, %i) -> Exceeds Size limit.\n", name, inum);
        return -1;
    } else if (directory_lookup(di, name) > 0) {
        fprintf(stderr, "ERROR: directory_put(%s, %i) -> Entry already exist!\n", name, inum);
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
    printf("DEBUG: directory_put(%s, %i) -> %i\n", name, inum, 1);
    return 1; 
}

// Delete an entry from the directory with the given name
int directory_delete(inode_t *di, const char *name) {
    // get directory entires
    dirent_t* entries = inode_get_block(di, 0);
    printf("DEBUG: directory_delete(%s) -> Attempting to delete!\n", name);
    // find the entry
    for(int ii = 0; ii < di->refs; ++ii) {
        // if name matches
        if(!strcmp(entries[ii].name, name) && entries[ii].used) {
            // found entry and delete any sub files
            delete_entry(entries[ii]);
        }
    }
}

// RECURSIVE: deletes all data in the entry
int delete_entry(dirent_t entry) {
    // get the node from this entry
    inode_t *node = get_inode(entry.inum);
    entry.used = 0;

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
        printf("%s\n", entries[ii].name);
        //print_inode(get_inode(entries[ii].inum));
    }
}

// get inode from the given path
int get_inode_path(const char* path) {
    assert(path[0] == '/');
    if(strcmp(path, "/") == 0) {
        printf("DEBUG: get_inode_path(%s) -> returned root inum (%i)\n", path, nROOT);
        return nROOT;
    }

    // Get the path names
    slist_t* path_list = slist_explode(path + 1, '/');
    slist_t* tmp = path_list;
    
    int count = 0;
    int inum = nROOT;
    while(tmp) {
        //DEBUG: Get Path Names
        printf("DEBUG: get_inode_path(%s) -> Path Name %s.\n", path, path_list->data);
        inum = directory_lookup(get_inode(inum), path_list->data);
        printf("DEBUG: get_inode_path(%s) -> Inum: %i.\n", path, inum);
        if(inum < 0) {
            slist_free(path_list);
            fprintf(stderr, "ERROR: get_inode_path(%s) -> Failed to find inode in this path.(-1)\n",
                path);
            return -1;
        }
        tmp = tmp->next;
        count++;
    }
    slist_free(path_list);
    printf("DEBUG: get_inode_path(%s) -> (%i)\n", path, inum);
    return inum;
}
