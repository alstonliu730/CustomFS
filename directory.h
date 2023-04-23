// Directory manipulation functions.
//
// Feel free to use as inspiration. Provided as-is.

// Based on cs3650 starter code
#ifndef DIRECTORY_H
#define DIRECTORY_H

#define DIR_NAME_LENGTH 48

#include "blocks.h"
#include "inode.h"
#include "slist.h"

// ERROR CODES
#define INV_DIR -2
#define OUT_SCOPE -3
#define ENT_EXST -4
#define ERR_INODE -5


typedef struct direntry {
  char name[DIR_NAME_LENGTH]; // the string name of directory
  int inum; // inode number
  int used;
  char _reserved[8];
} dirent_t; 

void directory_init();
int directory_lookup(inode_t *di, const char *name);
int directory_put(inode_t *di, const char *name, int inum);
int directory_delete(inode_t *di, const char *name);
int delete_entry(dirent_t entry);
slist_t *directory_list(const char *path);
void print_directory(inode_t *dd);
int get_inode_path(const char* path);

#endif
