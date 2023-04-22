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

// set the stat for the given path
int storage_stat(const char *path, struct stat *st) {
    int inum = get_inode_path(path);
    if(inum >= 0) {
        inode_t* node = get_inode(inum);
        st->st_size = node->size;
        st->st_mode = node->mode;
        st->st_nlink = node->refs;
        st->st_ctime = node->ctime;
        st->st_atime = node->atime;
        st->st_mtime = node->mtime;
        return 1;
    }
    return -1; // couldn't find file/directory
}

// read the file at this path for size amount of bytes and copies to the buffer
int storage_read(const char *path, char *buf, size_t size, off_t offset) {
    int inum = get_inode_path(path);
    if (inum == 0) {
        return inum;
    }

    // make sure the offset is not 
    // get inode of the path
    inode_t* node = get_inode(inum);
    
    // ensure offset is valid
    if (offset >= node->size || offset < 0) {
        return 0;
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

    return 1;
}

// writes the file at this path from the buffer with the number of size bytes.
int storage_write(const char *path, const char *buf, size_t size, off_t offset) {
    int size_change = storage_truncate(path, offset + size);
    if(size_change < 0) {
        return -1;
    }

    if(get_inode_path(path) == -1) {
        return -1;
    }

    inode_t* node = get_inode(get_inode_path(path));
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
    return 1;
}

// truncate the file to the given size
int storage_truncate(const char *path, off_t size) {
    int inum = get_inode_path(path);
    if(inum < 0) {
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
    if (get_inode_path(path) != -1) {
        printf("Inode already exists!\n");
        return -1;
    }

    // get the names of the child and parent
    char* dir = (char *) alloca(strlen(path) + 1);
    char* sub = (char *) alloca(strlen(path) + 1);
    get_child(path, sub);
    get_parent(path, dir);

    int parent_inum = get_inode_path(dir);
    if (parent_inum == -1) {
        printf("Parent Inode cannot be found!\n");
        return -1;
    }

    inode_t* parent_node = get_inode(parent_inum);

    int child_inum = alloc_inode();
    inode_t* node = get_inode(child_inum);
    node->mode = mode;
    node->size = 0;
    node->refs = 1;

    directory_put(parent_node, sub, child_inum);
    return 1;
}

// unlink the given path from the disk
int storage_unlink(const char *path) {
    // get the names of the child and parent
    char* dir = (char *) alloca(strlen(path) + 1);
    char* sub = (char *) alloca(strlen(path) + 1);
    get_child(path, sub);
    get_parent(path, dir);

    int parent_inum = get_inode_path(dir);
    if (parent_inum == -1) {
        printf("Parent Inode cannot be found!\n");
        return -1;
    }

    inode_t* parent_node = get_inode(parent_inum);
    int del = directory_delete(parent_node, sub);

    return del;
}

// link the given name of the directories
int storage_link(const char *from, const char *to) {
    // make sure 'to' exist
    int dest_inum = get_inode_path(to);
    if (dest_inum < 0) {
        return -1;
    }

    // get the names of the child and parent
    char* dir = (char *) alloca(strlen(from) + 1);
    char* sub = (char *) alloca(strlen(from) + 1);
    get_child(from, sub);
    get_parent(from, dir);

    int parent_inum = get_inode_path(dir);
    if (parent_inum == -1) {
        printf("Parent Inode cannot be found!\n");
        return -1;
    }

    inode_t* p_node = get_inode(parent_inum);
    directory_put(p_node, sub, dest_inum);

    return 1;
}

// rename the directories
int storage_rename(const char *from, const char *to) {
    storage_link(to, from);
    storage_unlink(from);
    return 0;
}

// set the time of the given file to the given timespec
int storage_set_time(const char *path, const struct timespec ts[2]) {
    int inum = get_inode_path(path);
    if (inum < 0) {
        return -1;
    }

    inode_t* node = get_inode(inum);
    node->atime = ts[0].tv_sec;
    node->mtime = ts[1].tv_sec;
    return 0;
}

// return the names of the directories
slist_t *storage_list(const char *path) {
    return directory_list(path);
}

// set the parent path to the given str
void get_parent(const char *path, char* str) {
    slist_t* path_names = slist_explode(path, '/');
    slist_t* copy = path_names;
    str[0] = '\0';
    while (copy->next) {
        strncat(str, "/", 2);
        strncat(str, copy->data, strlen(copy->data) + 1);
        copy = copy->next;
    }
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
    slist_free(path_names);
}