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
    root->mode = DIR_MODE | 0755; // permissions as a directory

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
    print_directory(root);
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
    assert(S_ISDIR(di->mode));
    // Debugging:
    printf("DEBUG: directory_put(%s, %i) -> Called Function\n", name, inum);

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
    strcpy(new_entry.name, name); // copy name to entry
    new_entry.inum = inum;
    new_entry.used = 1;
    
    print_inode(di);
    // add new entry to the list of entries
    memcpy(&entries[di->refs], &new_entry, sizeof(dirent_t));
    printf("DEBUG: directory_put(%s, %i) -> {Name: %s, Inum: %i}\n", name, inum, entries[di->refs].name, entries[di->refs].inum);
    di->size += sizeof(dirent_t);
    di->refs++;
    print_inode(di);
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
    if(S_ISREG(node->mode)) {
        free_inode(entry.inum);
        return 1;
    } 
    // case where the entry is a directory
    else if(S_ISDIR(node->mode)) {
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
    int p_inum = path_lookup(path);
    inode_t* node = get_inode(p_inum);
    assert(S_ISDIR(node->mode));

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
    printf("Directories:\n");
    for(int ii = 0; ii < dd->refs; ++ii) {
        printf("Entry #%i:\n", ii);
        printf("- Name: %s\n", entries[ii].name);
        printf("- Inum: %i\n", entries[ii].inum);
        printf("- Used: %i\n", entries[ii].used);
    }
}

// get inode number from the given path
int path_lookup(const char* path) {
    printf("DEBUG: path_lookup(%s) -> First Char: %c\n", path, path[0]);
    // assert(path[0] == '/');
    if(strcmp(path, "/") == 0) {
        printf("DEBUG: path_lookup(%s) -> returned root inum (%i)\n", path, nROOT);
        return nROOT;
    }
    char* name = strdup(path);
    // name += 1;
    
    // Get the path names
    slist_t* path_list = slist_explode(name, '/');
    slist_t* tmp = path_list;

    // the case where the first string is a slash
    if(!strcmp(tmp->data, "") && tmp->next) {
        tmp = tmp->next; // go to the next case since we give the root anyways
    }

    int inum = nROOT;
    while(tmp) {
        //DEBUG: Get Path Names
        printf("DEBUG: path_lookup(%s) -> Path Name: %s\n", name, tmp->data);
        inum = directory_lookup(get_inode(inum), tmp->data);
        printf("DEBUG: path_lookup(%s) -> Inum: %i\n", name, inum);
        if(inum < 0) {
            slist_free(path_list);
            fprintf(stderr, "ERROR: path_lookup(%s) -> Failed to find inode in this path.\n",
                name);
            return -1;
        }
        tmp = tmp->next;
    }
    slist_free(path_list);
    printf("DEBUG: path_lookup(%s) -> (%i)\n", name, inum);
    return inum;
}
