/**
 * @file storage.c
 * @author Alston Liu
 * 
 * Implementation of the storage 
 */
#include "storage.h"

// initialize the storage
void storage_init(const char *path) {
    blocks_init(path);
    directory_init();
}

// returns if the given path has access
int storage_access(const char *path) {
    int inum = path_lookup(path);

    if (inum < 0) {
        return -1;
    }

    inode_t *node = get_inode(inum);
    node->atime = time(NULL);

    return 0;
}

// set the stat for the given path
int storage_stat(const char *path, struct stat *st) {
    printf("DEBUG: storage_stat(%s) -> Called function.\n", path);
    // makes sure the path is not empty
    if(!strcmp(path, "")) {
        fprintf(stderr, "ERROR: storage_stat() -> Given empty path name!\n");
        return -1;
    }

    // gets the inode from the path
    int inum = path_lookup(path);
    if(inum >= 0) {
        memset(st, 0, sizeof(struct stat));
        inode_t* node = get_inode(inum);
        st->st_uid = getuid();
        st->st_size = node->size;
        st->st_mode = node->mode;
        st->st_nlink = node->refs;
        st->st_ctime = node->ctime;
        st->st_atime = node->atime;
        st->st_mtime = node->mtime;
        printf("DEBUG: storage_stat(%s) -> {inum: %i, mode: %i, size: %i, refs: %i}\n",
            path, inum, node->mode, node->size, node->refs);
        return 0;
    }
    fprintf(stderr, "ERROR: storage_stat(%s) -> (-1)\n", path);
    return -1; // couldn't find file/directory
}

// read the file at this path for size amount of bytes and copies to the buffer
int storage_read(const char *path, char *buf, size_t size, off_t offset) {
    printf("DEBUG: storage_read(%s, %s, %zu, %d) -> Function called\n",
        path, buf, size, (int)offset);
    // get the node from the path
    int inum = path_lookup(path);
    if (inum < 0) {
        fprintf(stderr, "ERROR: storage_read(%s, %s, %zu, %d) -> Cannot find file path\n",
            path, buf, size, (int)offset);
        return -1;
    }

    // get inode of the path
    inode_t* node = get_inode(inum);
    assert(S_ISREG(node->mode));

    // ensure offset is valid
    if (offset > node->blocks * BLOCK_SIZE || offset < 0) {
        fprintf(stderr, "ERROR: storage_read() -> Offset invalid.\n");
        return 0;
    }

    // read through the block
    int bytesRead = 0;
    int bytesRem = size;
    while (bytesRead < size) {
        // get address pointing to the offset in the data block
        int bnum = inode_get_bnum(node, offset + bytesRead);
        if (bnum < 0) {
            fprintf(stderr, "ERROR: storage_read() -> cannot find bnum at this point: %ld\n", offset + bytesRead);
            return bytesRead;
        }
        char* start = blocks_get_block(bnum);
        char* end = start + BLOCK_SIZE;
        char* file_ptr = (bytesRead > 0) ? start : start + (offset % BLOCK_SIZE);

        int bytesToRead;
        char* target = file_ptr + bytesRem;
        if (target > end) {
            bytesToRead = end - file_ptr;
        } else {
            bytesToRead = bytesRem;
        }
        printf("DEBUG: storage_read() -> bytes to read: %i\n", bytesToRead);

        memcpy(buf + bytesRead, file_ptr, bytesToRead);
        /*
        int ii = 0;
        while(ii < bytesToRead && file_ptr[ii + bytesRead] != '\0') {
            memset(&buf[ii + bytesRead], file_ptr[ii + bytesRead], sizeof(char));
            //printf("DEBUG: storage_read() -> Letter read: %c\n", buf[ii + bytesRead]);
            ++ii;
        } */
        printf("DEBUG: storage_read() -> String read: %s\n", buf + bytesRead);
        bytesRem -= bytesToRead;
        bytesRead += bytesToRead;
    }
    printf("DEBUG: storage_read(%s, %zu, %d) -> (%i)\n",
            path, size, (int)offset, bytesRead);
    return bytesRead;
}

// writes the file at this path from the buffer with the number of size bytes.
int storage_write(const char *path, const char *buf, size_t size, off_t offset) {
    printf("DEBUG: storage_write(%s, %zu, %d) -> Called Function.\n",
        path, size, (int)offset);
    assert(offset >= 0);
    assert(size >= 0);

    // case where the file can't be found
    int inum = path_lookup(path);
    if(inum < 0) {
        fprintf(stderr, "ERROR: storage_write(%s, %zu, %d) -> Could not get inode from path.\n",
            path, size, (int)offset);
        return -1;
    }

    // if the file needs more space
    inode_t* node = get_inode(inum);
    int new_size = offset + size;
    if(new_size > node->blocks * BLOCK_SIZE) {
        printf("DEBUG: storage_write(%s, %zu, %d) -> Size change: %i\n",
            path, size, (int)offset, new_size);
        int size_change = storage_truncate(path, new_size);
        if(size_change < 0) {
            fprintf(stderr, "ERROR: storage_write(%s, %zu, %d) -> Failed to truncate file.\n",
                path, size, (int)offset);
            return -1;
        }
    }

    // prepare writing variables
    int bytesWritten = 0;
    int bytesRem = size;
    while (bytesWritten < size) {
        // get address point to the offset in the data block
        int bnum = inode_get_bnum(node, offset + bytesWritten);
        if (bnum < 0) {
            fprintf(stderr, "ERROR: storage_write() -> cannot find bnum\n");
            return (bytesWritten > 0) ? bytesWritten : -1;
        }
        char* start = blocks_get_block(bnum);
        char* end = start + BLOCK_SIZE;
        char* file_ptr = (bytesWritten > 0) ? start : start + (offset % BLOCK_SIZE);
        // printf("DEBUG: storage_write() -> {start: %p}\n", start);
        // printf("DEBUG: storage_write() -> {file_ptr: %p}\n", file_ptr);
        // printf("DEBUG: storage_write() -> {end: %p}\n", end);
        
        // calculate how many bytes to write to buffer
        int bytesToWrite;
        char* target = file_ptr + bytesRem;
        if (target > end) {
            bytesToWrite = end - file_ptr;
        } else {
            bytesToWrite = bytesRem;
        }
        printf("DEBUG: storage_write() -> bytes to write: %i\n", bytesToWrite);

        // write to buffer and update inode
        memcpy(file_ptr, buf + bytesWritten, bytesToWrite);
        printf("DEBUG: storage_write() -> File pointer: %p\n", file_ptr);
        printf("DEBUG: storage_write() -> Written:\"%s\"\n", file_ptr + bytesWritten);
        
        // update write variables
        bytesWritten += bytesToWrite;
        bytesRem -= bytesToWrite;
    }

    // update the inode with the new size
    if (size + offset > node->size) {
        node->size = size + offset;
    }

    printf("DEBUG: storage_write(%s, %zu, %d) -> (%i)\n", 
        path, size, (int)offset, bytesWritten);
    return bytesWritten;
}

// truncate the file to the given size
int storage_truncate(const char *path, size_t size) {
    printf("DEBUG: storage_truncate(%s, %zu) -> Called Function\n", path, size);
    int inum = path_lookup(path);
    if(inum < 0) {
        fprintf(stderr, "ERROR: storage_truncate(%s, %zu) -> Could not get inode from path.\n",
            path, size);
        return -1;
    }

    // change depending on the size
    inode_t* node = get_inode(inum);
    int maxSize = node->blocks * BLOCK_SIZE;
    if (size > maxSize) {
        printf("DEBUG: storage_truncate(%s, %zu) -> Growing inode(%i) by %zu bytes\n", path, size, inum, (size - node->size));
        return grow_inode(node, size - maxSize);
    } else if (maxSize > size) {
        printf("DEBUG: storage_truncate(%s, %zu) -> Shrinking inode(%i) by %zu bytes\n", path, size, inum, (node->size - size));
        return shrink_inode(node, maxSize - size);
    } else {
        // Already equal in size
        return 0;
    }
}

// creates a new inode for an entry at the path depending on given mode
int storage_mknod(const char *path, int mode) {
    printf("DEBUG: storage_mknod(%s, %i) -> Called Function.\n", path, mode);
    int inum = path_lookup(path);
    if (inum >= 0) {
        fprintf(stderr, "ERROR: storage_mknod(%s, %i) -> Inode already exists!\n", path, mode);
        return -1;
    }
    
    // get the names of the child and parent
    printf("DEBUG: storage_mknod(%s, %i) -> Getting name of parent.\n", path, mode);
    char dir[strlen(path)];
    get_parent(path, dir);

    printf("DEBUG: storage_mknod(%s, %i) -> Getting name of child.\n", path, mode);
    char sub[DIR_NAME_LENGTH];
    get_child(path, sub);   

    // DEBUGGING
    printf("DEBUG: storage_mknod(%s, %i) -> Parent: %s\n", path, mode, dir);
    printf("DEBUG: storage_mknod(%s, %i) -> Child: %s\n", path, mode, sub);

    // get the parent inode
    int parent_inum = path_lookup(dir);
    if (parent_inum < 0) {
        fprintf(stderr, "ERROR: storage_mknod(%s, %i) -> Parent Inode cannot be found!\n", 
            path, mode);
        return -1;
    }
    inode_t* parent_node = get_inode(parent_inum);

    // initialize the inode
    int child_inum = alloc_inode();
    inode_t* child_node = get_inode(child_inum);
    child_node->mode = mode;
    child_node->size = 0;
    
    // if the given path is a directory
    if (S_ISDIR(mode)) {
        printf("DEBUG: storage_mknod(%s, %i) -> Creating self and parent reference.\n", 
            path, mode);
        // set the child directory's default entries
        directory_put(child_node, ".", child_inum);
        directory_put(child_node, "..", parent_inum);
        child_node->refs = 2;
    } else {
        child_node->refs = 1;
    }

    directory_put(parent_node, sub, child_inum);
    printf("DEBUG: storage_mknod(%s, %i) -> Printing directory entries\n", path, mode);
    print_directory(parent_node);
    return 0;
}

// unlink the given path from the disk
int storage_unlink(const char *path) {
    printf("DEBUG: storage_unlink(%s) -> Called Function.\n", path);
    // get the names of the child and parent
    char* dir = (char *) malloc(strlen(path) + 10);
    char* sub = (char *) malloc(strlen(path) + 10);
    get_child(path, sub);
    get_parent(path, dir);

    int parent_inum = path_lookup(dir);
    if (parent_inum < 0) {
        fprintf(stderr, "ERROR: storage_unlink(%s) -> Parent Inode cannot be found!\n", path);
        free(sub);
        free(dir);
        return -1;
    }
    
    inode_t* parent_node = get_inode(parent_inum);
    int del = directory_delete(parent_node, sub);
    printf("DEBUG: storage_unlink(%s) -> (%i)\n", path, del);
    free(sub);
    free(dir);
    return del;
}

// link the given name of the directories
int storage_link(const char *from, const char *to) {
    // if the paths are the same
    if(!strcmp(from, to)) {
        return 0;
    }

    // make sure from exist
    int from_inum = path_lookup(from);
    if (from_inum < 0) {
        fprintf(stderr, "ERROR: storage_link(%s, %s) -> Could not find path \'from\'.\n", from, to);
        return -1;
    }

    // make sure 'to' exist
    int to_inum = path_lookup(to);

    // case where the destination doesn't exist
    if (to_inum < 0) {
        fprintf(stderr, "ERROR: storage_link(%s, %s) -> Could not find path \'to\'.\n", from, to);
        storage_mknod(to, DIR_MODE | 0775);
        to_inum = path_lookup(to);
    }

    // get the inode for 'to' path
    inode_t* to_node = get_inode(to_inum);
    inode_t* from_inode = get_inode(from_inum);
    inode_t* dir_inode;

    // allocate memory for the 'to'  directory
    char *dir = (char *) malloc(strlen(to) + 1);

    // get the name of the file or directory
    char *sub = (char *) malloc(DIR_NAME_LENGTH);
    get_child(from, sub);

    // if the 'to' path is a file
    if(S_ISREG(to_node->mode)) {
        // set parent and child names
        get_parent(to, dir);
        get_child(to, sub);

        // get the directory inode of the 'to' path
        dir_inode = get_inode(path_lookup(dir));

        // add it to the directory with the name of the file
        directory_put(dir_inode, sub, from_inum);
    } else {
        // assert that the 'to' path is a directory
        assert(S_ISDIR(to_node->mode));

        // add the file/directory to the entries
        directory_put(to_node, sub, from_inum);
    }

    // clear the memory from the allocated variable
    free(sub);
    free(dir);
    return 0;
}

// TODO: rename the directories
int storage_rename(const char *from, const char *to) {
    printf("DEBUG: storage_rename(%s, %s) -> Called function.\n", from, to);

    // make sure the from path exists
    int from_inum = path_lookup(from);
    if(from_inum < 0) {
        fprintf(stderr, "ERROR: storage_rename(%s, %s) -> Cannot find given \'from\' path\n");
        return -1;
    }
    
    // link the 'from' to 'to' path
    storage_link(from, to);

    // delete the file/directory from the parent entries
    // get parent directory
    char* dir = (char *) malloc(strlen(from) + 1);
    get_parent(from, dir);

    char* sub = (char *) malloc(DIR_NAME_LENGTH + 1);
    get_child(from, sub);

    int dir_inum = path_lookup(dir);
    inode_t* dir_inode = get_inode(dir_inum);
    dirent_t* entries = inode_get_block(dir_inode, 0);

    // go through entries and find the name
    for(int ii = 0; ii < dir_inode->refs; ++ii) {
        dirent_t entry = entries[ii];
        if(!strcmp(entry.name, sub)) {
            entry.used = 0;
            entry.inum = -1;
            memset(&entry.name, 0, DIR_NAME_LENGTH);
            return 0;
        }
    }

    // couldn't find the name in the entry
    return -1;
}

// set the time of the given file to the given timespec
int storage_set_time(const char *path, const struct timespec ts[2]) {
    int inum = path_lookup(path);
    if (inum < 0) {
        fprintf(stderr, "ERROR: storage_set_time(%s) -> Could not find inode of given path.\n", path);
        return -1;
    }

    inode_t* node = get_inode(inum);
    node->atime = ts[0].tv_sec;
    node->mtime = ts[1].tv_sec;
    printf("DEBUG: storage_set_time(%s) -> (0)\n", path);
    return 0;
}

// return the names of the directories
slist_t *storage_list(const char *path) {
    return directory_list(path);
}

// set the parent path to the given str
void get_parent(const char *path, char* str) {
    printf("DEBUG: get_parent(%s) -> Called Function.\n", path);
    slist_t* path_names = slist_explode(path, '/');
    slist_t* copy = path_names;
    str[0] = '\0';
    // the case where the first string is a slash
    if(!strcmp(copy->data, "") && copy->next) {
        strcat(str, "/"); // will always add the root
        copy = copy->next; // go to the next case
    }

    while (copy->next) {
        printf("DEBUG: get_parent(%s) -> str: %s\n", path, str);
        if (strcmp(str, "/")){
            strcat(str, "/");
        }
        strncat(str, copy->data, strlen(copy->data) + 1);
        copy = copy->next;
    }

    printf("DEBUG: get_parent(%s) -> Parent: %s\n", path, str);
    slist_free(path_names);
}

// set the child name to the given str and given full path
void get_child(const char *path, char* str) {
    // DEBUG: Enter in the function
    printf("DEBUG: get_child(%s) -> Called Function.\n", path);
    slist_t* path_names = slist_explode(path, '/');
    slist_t* copy = path_names;
    printf("DEBUG: get_child(%s) -> Exploding path into an slist\n", path);
    print_list(copy);

    // the case where the first string is a slash
    if(!strcmp(copy->data, "") && copy->next) {
        copy = copy->next; // go to the next case
    }

    // go through the list until the last one
    while(copy->next || !strcmp(copy->data, "")) {
        printf("DEBUG: get_child(%s) -> Curr Name: %s\n", path, copy->data);
        copy = copy->next; // skip to the end
    }

    // copy data into the string
    strcpy(str, copy->data);
    slist_free(path_names);

    // DEBUG: Output of the function
    printf("DEBUG: get_child(%s) -> Child: %s\n", path, str);
}
