# Custom File System
This project is based on my work for the implementation of a custom File System for a class. It utilizes the POSIX library to call on system functions in Linux and FUSE (File System in User Space) API to run the file system in the user space and not in the kernel space where File Systems usually do. 
The package will include starter programs provided for me which includes:

- [Makefile](Makefile)   - Targets are explained in the assignment text
- [README.md](README.md) - This README
- [bitmap.c](bitmap.c)   - helper code to keep track of free data
- [blocks.c](blocks.c)   - helper code to create blocks of data
- [slist.c](slist.c)     - helper code to create string linked lists
- [nufs.c](nufs.c)       - The main file of the file system driver
- [test.pl](test.pl)     - Tests to exercise the file system

The package will also include my own custom codes that I have created to assist in the file system:

- [directory.c](directory.c) - implementation of a directory or folder in a file system
- [inode.c](inode.c)         - implementation of metadata of the files
- [storage.c](storage.c)     - implementation of functions on organizing file data

## Running the tests
You might need install an additional package to run the provided tests:

```
$ sudo apt-get install libtest-simple-perl
```

Then using `make test` will run the provided tests.

### Commands
The commands that can be used in this filesystem are similar to the ones used in LINUX systems. For example, `cat` is used to print out the file's content and `cd` is used to change directories. Here are some stable commands that are able to run:

 - `cat`
 - `cd`
 - `mkdir`
 - `ls`
 - `help`
 - `touch`
 - `rmdir`
 - `readdir`
 - `stat`

## Design Changes
The changes that were made to the project are recorded here. Implementation started from the smallest piece of data to the bigger overview of organizing those pieces of data. I started with the blocks implementation and the inode implementations.

### Blocks of Data
The block implementation contains functions that initializes the bitmaps and blocks of free data that the system and users will be using to store data in. Each block of data, file, or folder all contains metadata to get an overview of what the data contains. The size of the blocks of data are `4096 Bytes` or `4 KB`. We will need some blocks of data allocated specially for the file system since we need to store the metadata, bitmaps, and root directory. The functions that make up the blocks will be stored in blocks.c. The functions in this section of the code is mainly used to initialize the bitmaps and blocks of data that is allocated by the file system. The total amount of data that is allocated is 1MB for the disk image.

### Inodes
The inodes implementation is the metadata information that is kept in each file or directory. The inode contains the number of references, the permission attribute and the type of file it is, the size of the file, the number of blocks allocated, and the time it was created, accessed, and modified. For the system, we limited the inode bitmap to only one block of data. This would mean there are `4096 * 8` or `32,768` inodes available, which should be enough for a 1MB disk image.

### Directories
The directory implementation is to track the entries of a directory. A directory in my implementation would have an inode that tracks the name of the directory and the block containing the directory entries. An entry would point to a file or another directory. In each entry, it tracks the name of the file/directory and the inode of the file/directory. It also includes if the entry itself is free to be used. The file system would need to know if the given entry is free to be used or not. When removing an entry from the directory, it would be easier to toggle the entry on or off.

### Storage Methods
The storage methods allow the block, inode, and directory implementation to come together. One method is the storage_access method that tells the system if the given path name is reachable. Then another important function is the stat function since it is the baseline for functional commands in the FUSE system. The get_attr function returns the stats of the file or directory which includes:
 - id
 - size
 - mode
 - number of references
 - creation time
 - accessed time
 - modified time
 
This is similar to the properties option in a Windows Filesystem that includes these types of information of the file or directory.


