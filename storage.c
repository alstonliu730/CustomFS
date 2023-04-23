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
    
    // ensure offset is valid
    if (offset > node->size || offset < 0) {
        fprintf(stderr, "ERROR: storage_read() -> Offset invalid.\n");
        return -1;
    }

    // read through the block
    int bytesRead = 0;
    int bytesRem = size;
    while (bytesRead < size) {
        // get address pointing to the offset in the data block
        char* start = inode_get_block(node, offset + bytesRead);
        char* file_ptr = start + ((offset + bytesRead) % BLOCK_SIZE);
        char* end = start + BLOCK_SIZE;

        int bytesToRead;
        if (bytesRem + file_ptr >= end) {
            bytesToRead = end - file_ptr;
        } else {
            bytesToRead = bytesRem;
        }

        memcpy(buf + bytesRead, file_ptr, bytesToRead);
        bytesRem -= bytesToRead;
        bytesRead += bytesToRead;
    }
    printf("DEBUG: storage_read(%s, %s, %zu, %d) -> (0)\n",
            path, buf, size, (int)offset);
    return 0;
}

// writes the file at this path from the buffer with the number of size bytes.
int storage_write(const char *path, const char *buf, size_t size, off_t offset) {
    int size_change = storage_truncate(path, offset + size);
    if(size_change < 0) {
        fprintf(stderr, "ERROR: storage_write(%s, %s, %zu, %d) -> Failed to truncate file.\n",
            path, buf, size, (int)offset);
        return -1;
    }

    if(path_lookup(path) < 0) {
        fprintf(stderr, "ERROR: storage_write(%s, %s, %zu, %d) -> Could not get inode from path.\n",
            path, buf, size, (int)offset);
        return -1;
    }

    inode_t* node = get_inode(path_lookup(path));
    int bytesWritten = 0;
    int bytesRem = size;
    while (bytesWritten < size) {
        // get address point to the offset in the data block
        char* start = inode_get_block(node, offset + bytesWritten);
        char* file_ptr = start + ((offset + bytesWritten) % BLOCK_SIZE);
        char* end = start + BLOCK_SIZE;

        // calculate how many bytes to write to buffer
        int bytesToWrite;
        if (bytesRem + file_ptr >= end) {
            bytesToWrite = end - file_ptr;
        } else {
            bytesToWrite = bytesRem;
        }

        // write to buffer and update inode
        memcpy(file_ptr + bytesWritten, buf + bytesWritten, bytesToWrite);
        bytesWritten += bytesToWrite;
        bytesRem -= bytesToWrite;
    }
    printf("DEBUG: storage_write(%s, %s, %zu, %d) -> (1)\n", path, buf, size, (int)offset);
    return 1;
}

// truncate the file to the given size
int storage_truncate(const char *path, off_t size) {
    int inum = path_lookup(path);
    if(inum < 0) {
        fprintf(stderr, "ERROR: storage_truncate(%s, %zu) -> Could not get inode from path.\n",
            path, size);
        return -1;
    }

    inode_t* node = get_inode(inum);
    if (node->size < size) {
        return grow_inode(node, (size - node->size));
    } else {
        return shrink_inode(node, node->size - size);
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
    char* dir = (char *) malloc(strlen(path) + 10);
    char* sub = (char *) malloc(strlen(path) + 10);
    get_child(path, sub);
    get_parent(path, dir);

    // DEBUGGING
    printf("DEBUG: storage_mknod(%s, %i) -> Parent: %s\n", path, mode, dir);
    printf("DEBUG: storage_mknod(%s, %i) -> Child: %s\n", path, mode, sub);

    // get the parent inode
    int parent_inum = path_lookup(dir);
    if (parent_inum < 0) {
        fprintf(stderr, "ERROR: storage_mknod(%s, %i) -> Parent Inode cannot be found!\n", 
            path, mode);
        free(dir);
        free(sub);
        return -1;
    }
    inode_t* parent_node = get_inode(parent_inum);

    // initialize the inode
    int child_inum = alloc_inode();
    inode_t* node = get_inode(child_inum);
    node->mode = mode;
    node->size = 0;
    node->refs = 1;

    // if the given path is a directory
    if (mode >= 040755) {
        printf("DEBUG: storage_mknod(%s, %i) -> Creating self and parent reference.\n", 
            path, mode);
        // set the child directory's default entries
        directory_put(node, ".", child_inum);
        directory_put(node, "..", parent_inum);
    }

    directory_put(parent_node, sub, child_inum);
    printf("DEBUG: storage_mknod(%s, %i) -> (1)\n", path, mode);

    free(sub);
    free(dir);
    return 1;
}

// unlink the given path from the disk
int storage_unlink(const char *path) {
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
    // make sure 'to' exist
    int dest_inum = path_lookup(to);
    if (dest_inum < 0) {
        fprintf(stderr, "ERROR: storage_link(%s, %s) -> Could not find path \'to\'.\n", from, to);
        return -1;
    }

    // get the names of the child and parent
    char* dir = (char *) malloc(strlen(from) + 10);
    char* sub = (char *) malloc(strlen(from) + 10);
    get_child(from, sub);
    get_parent(from, dir);

    int parent_inum = path_lookup(dir);
    if (parent_inum < 0) {
        fprintf(stderr, "ERROR: storage_link(%s, %s) -> Parent Inode cannot be found!\n", from, to);
        free(sub);
        free(dir);
        return -1;
    }

    inode_t* p_node = get_inode(parent_inum);
    directory_put(p_node, sub, dest_inum);
    printf("DEBUG: storage_unlink(%s, %s) -> (1)", from, to);
    free(sub);
    free(dir);
    return 1;
}

// rename the directories
int storage_rename(const char *from, const char *to) {
    printf("DEBUG: storage_rename(%s, %s) -> Called link and unlink here.\n", from, to);
    storage_link(to, from);
    storage_unlink(from);
    return 0;
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
    while (copy->next) {
        printf("DEBUG: get_parent(%s) -> str: %s\n", path, str);
        strncat(str, "/", 2);
        strncat(str, copy->data, strlen(copy->data) + 1);
        copy = copy->next;
    }

    printf("DEBUG: get_parent(%s) -> Parent: %s\n", path, str);
    slist_free(path_names);
}

// set the child name to the given str
void get_child(const char *path, char* str) {
    slist_t* path_names = slist_explode(path, '/');
    slist_t* copy = path_names;
    while (copy->next) {
        copy = copy->next; // NULL
    }
    memcpy(str, copy->data, strlen(copy->data));
    strncat("\0", str, 1);
    printf("DEBUG: get_child(%s) -> Child: %s\n", path, str);
    slist_free(path_names);
}
