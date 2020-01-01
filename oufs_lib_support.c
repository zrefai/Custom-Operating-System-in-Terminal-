#include <stdlib.h>
#include <libgen.h>
#include "oufs_lib.h"
#include "oufs.h"

#define debug 0
#define MAX_BUFFER 1024

/**
 *  Compares a string directory_entry_a with the second string directory_entry_b. It is used as the function for qsort when outputting the directory entries in sorted order
 *
 *  @param directory_entry_a The first string to compare
 *  @param directory_entry_b The second string to compare
 *  @return An int that decides which string should go before the other
 */
int string_compare(const void *directory_entry_a, const void *directory_entry_b)
{
    //Directory entry a
    DIRECTORY_ENTRY *a = (DIRECTORY_ENTRY *) directory_entry_a;
    //Directory entry b
    DIRECTORY_ENTRY *b = (DIRECTORY_ENTRY *) directory_entry_b;
    //String compare
    return strcmp(a->name, b->name);
}

/**
 *  Function takes a string and mutates it a certain way depending on what begin and length are. It starts cutting out portions of the string at the begin int, then cuts out a length equal to the length int. What comes out is a mutated string that was cut at specified indexes.
 *
 *  @param char *str The buffer that holds the string to mutate
 *  @param int begin Where to start cutting in the string
 *  @param int length How much of the string to cut after int begin
 *  @return len The length of the original string
 */
int clip(char *str, int begin, int length) {
    //Getting length of the buffer
    int len_of_string = strlen(str);
    //Checks parameters
    if (length < 0) {
        length = len_of_string - begin;
    }
    //Checks parameters
    if (begin + length > len_of_string) {
        length = len_of_string - begin;
    }
    
    //Punches portions of the string
    memmove(str + begin, str + begin + length, len_of_string - length + 1);
    
    return length;
}

/**
 *  Prints out the directories listed in the block parameter. First it sorts the entries using qsort, then lists out the entries with a / or no slash depending on if the entry is a directory or file in sorted order. The block is mutated during qsort, but not written back to the disk so that the sorted order isn't in the disk.
 *
 *  @param INODE inode Used to check if an entry is a file or directory
 *  @param BLOCK block Used to go through the directory entries
 *  @param char *base_name Used for if the inode is a file, and it prints out just the base_name
 */
void list_directory_entries(INODE *inode, BLOCK *block, char *base_name) {
    //Check if inode of basename is directory or file
    if (inode->type == IT_DIRECTORY) {
        //qsort the entries and print them out
        qsort(block->directory.entry, 16, sizeof(DIRECTORY_ENTRY), string_compare);
        
        
        //Loop through entries
        for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
            if(block->directory.entry[i].inode_reference != UNALLOCATED_INODE) {
                printf("%s", block->directory.entry[i].name);
                
                //Checking to see if the inode of the entry is a directory or file
                INODE_REFERENCE inode_ref = block->directory.entry[i].inode_reference;
                INODE mock_inode;
                oufs_read_inode_by_reference(inode_ref, &mock_inode);
                if (mock_inode.type == IT_DIRECTORY) {
                    printf("/\n");
                } else if (mock_inode.type == IT_FILE) {
                    printf("\n");
                }
            }
        }
        exit(EXIT_SUCCESS);
    }
    
    //If the inode is a file
    if (inode->type == IT_FILE) {
        printf("%s\n", base_name);
    }
}

/**
 *  A function that looks for the specified path where is returns the inode of the parent of the specified path. It steps through inodes and blocks sequentially to gather what entries are where. This function is good if the path is absolute, or if the path includes subdirectories and it needs to step through those to find what entries are where. It will use base_name and mkdir_flag to make decisions about what it needs to look for. Base_name is used to check to see if the path base_name is already in the list of entries; so it throws an error depending on what funciton we are in. The mkdir_flag is used to specify if we in the mkdir function and we are creating a directory.
 *
 *  @param char *path The path for the function to take to find the parent inode that it returns
 *  @param INODE_REFERERNCE base_inode the inode to start at to find path, can be 0 for base inode, or other inode already allocated
 *  @param char *base_name Contains the base name of path
 *  @param int mkdir_flag Flag used depending on if we are creating a directory entry, not a file
 *  @return The parent inode of where we need to do operations
 */
INODE_REFERENCE oufs_look_for_inode_from_root(char *path, INODE_REFERENCE base_inode, char *base_name, int mkdir_flag) {
    
    //Varibles for tokenizing inputs of the parameter
    char *path_tokens[64];
    char **path_arg;
    //Tokenize input
    path_arg = path_tokens;
    *path_arg++ = strtok(path, "/");
    while ((*path_arg++ = strtok(NULL, "/")));
    
    BLOCK_REFERENCE base_block;
    //Get root inode
    INODE inode;
    oufs_read_inode_by_reference(base_inode, &inode);
    //Get block from block reference in inode
    BLOCK block;
    base_block = inode.data[0];
    //Read block at reference into block
    vdisk_read_block(base_block, &block);
    
    int counter = 0;
    //Go through parameters of path
    for (int i = 0; i < 64; ++i) {
        if (debug) {
            printf("Path_token[%d]: %s\n", counter, path_tokens[counter]);
        }
        //If empty element in path
        if (path_tokens[counter] == NULL) {
            break;
        }
        
        if (mkdir_flag == 1) {
            if (!strcmp(base_name, path_tokens[counter])) {
                break;
            }
        }

        if(block.directory.entry[i].inode_reference != UNALLOCATED_INODE) {
            if(!strcmp(block.directory.entry[i].name, path_tokens[counter])) {
                //Getting inode reference of entry found
                base_inode = block.directory.entry[i].inode_reference;
                //Get inode of inode reference
                oufs_read_inode_by_reference(base_inode, &inode);
                if (inode.type == IT_DIRECTORY) {
                    //Getting block reference in inode
                    base_block = inode.data[0];
                    //Setting block to newly found block
                    vdisk_read_block(base_block, &block);
                } else if (inode.type == IT_FILE) {
                    break;
                }
                //Reset for loop
                i = 0;
                ++counter;
            }
        } else if (i == 14 && block.directory.entry[i].inode_reference == UNALLOCATED_INODE) {
            fprintf(stderr, "ERROR: Parent directory doesn't exist\n");
            exit(EXIT_FAILURE);
            break;
        }
    }
    return base_inode;
}

/**
 *  Same functionality of oufs_look_for_inode_from_root but it starts finding the path from cwd first. The main functionality comes when path_flag is enabled. If path_flag is enabled it allows a user to start at a cwd that's not the root, then continue with path in oufs_look_for_inode_from_root since the latter function starts at what "root" we give it. So it uses 2 functions if you want to to move through cwd and path.
 *
 *  @param char *path The path to take for oufs_look_for_inode_from_root
 *  @param char *cwd The cwd path to take to find normal root
 *  @param INODE_REFERENCE base_inode base_inode or "root" to start at, is always going to be 0
 *  @param char *base_name path name of path
 *  @param int path_flag If we want to search through the path as well
 *  @param int mkdir_flag If we are mkaing a directory
 *  @return The parent inode to do operations in
 */
INODE_REFERENCE oufs_look_for_inode_not_from_root(char *path, char *cwd, INODE_REFERENCE base_inode, char *base_name, int path_flag, int mkdir_flag) {
    //Variables for tokenizing inputs of the cwd
    char *cwd_tokens[64];
    char **cwd_arg;
    //Tokenize input
    cwd_arg = cwd_tokens;
    *cwd_arg++ = strtok(cwd, "/");
    while ((*cwd_arg++ = strtok(NULL, "/")));
    
    BLOCK_REFERENCE base_block;
    //Get root inode
    INODE inode;
    oufs_read_inode_by_reference(base_inode, &inode);
    //Get block from block reference in inode
    BLOCK block;
    base_block = inode.data[0];
    //Read block at reference into block
    vdisk_read_block(base_block, &block);
    
    //Counter for cwd_tokens
    int counter = 0;
    
    /* Go through cwd_tokens and match arguments to entries in the
     directory entry. Then grab inode and block of that directory and repeat
     */
    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
        //If empty element in cwd
        if (cwd_tokens[counter] == NULL) {
            break;
        }
        
        //Check to see if entry contains name
        if(block.directory.entry[i].inode_reference != UNALLOCATED_INODE) {
            if(!strcmp(block.directory.entry[i].name, cwd_tokens[counter])) {
                //Getting inode reference of entry found
                base_inode = block.directory.entry[i].inode_reference;
                //Get inode of inode reference
                oufs_read_inode_by_reference(base_inode, &inode);
                //Getting block reference in inode
                base_block = inode.data[0];
                //Setting block to newly found block
                vdisk_read_block(base_block, &block);
                //Reset for loop
                i = 0;
                ++counter;
            }
            //Went through entire entry list and didn't find parent
        } else {
            fprintf(stderr, "ERROR: (zfilez, current working directory is not root, and no parameter specified): Parent directory doesn't exist\n");
            exit(EXIT_FAILURE);
            break;
        }
    }
    
    if (debug) {
        printf("Base_inode after cwd: %d\n", base_inode);
    }
    
    //If we want to continue through path
    if (path_flag == 1) {
        //If we are making a directory
        if (mkdir_flag == 1) {
            base_inode = oufs_look_for_inode_from_root(path, base_inode, base_name, 1);
        } else {
            //If we are making a file
            base_inode = oufs_look_for_inode_from_root(path, base_inode, base_name, 0);
        }
        if (debug) {
            printf("Base_inode after cwd after path: %d\n", base_inode);
        }
    }
    
    return base_inode;
}

/**
 * Read the ZPWD and ZDISK environment variables & copy their values into cwd and disk_name.
 * If these environment variables are not set, then reasonable defaults are given.
 *
 * @param cwd String buffer in which to place the OUFS current working directory.
 * @param disk_name String buffer containing the file name of the virtual disk.
 */
void oufs_get_environment(char *cwd, char *disk_name)
{
    // Current working directory for the OUFS
    char *str = getenv("ZPWD");
    if(str == NULL) {
        // Provide default
        strcpy(cwd, "/");
    } else {
        // Exists
        strncpy(cwd, str, MAX_PATH_LENGTH-1);
    }
    
    // Virtual disk location
    str = getenv("ZDISK");
    if(str == NULL) {
        // Default
        strcpy(disk_name, "vdisk1");
    }else{
        // Exists: copy
        strncpy(disk_name, str, MAX_PATH_LENGTH-1);
    }
}

/**
 * Configure a directory entry so that it has no name and no inode
 *
 * @param entry The directory entry to be cleaned
 */
void oufs_clean_directory_entry(DIRECTORY_ENTRY *entry) 
{
    entry->name[0] = 0;  // No name
    entry->inode_reference = UNALLOCATED_INODE;
}

/**
 *  Create a virtual disk with initial inode and directory set up
 *
 *  @param virtual_disk_name The name of the virtual disk
 */
int oufs_format_disk(char  *virtual_disk_name) {
    if (vdisk_disk_open(virtual_disk_name) != 0) {
        fprintf(stderr, "ERROR: openening vdisk\n");
        return -1;
    }
    
    BLOCK b;
    memset(b.data.data, 0, sizeof(b));
    
    for (int i = 0; i < N_BLOCKS_IN_DISK; i++) {
        if (vdisk_write_block(i, &b) != 0) {
            fprintf(stderr, "ERROR: writing 0 to block\n");
            return -1;
        }
    }
    
    //Mark the blocks as allocated
    b.master.inode_allocated_flag[0] = 1;
    b.master.block_allocated_flag[0] = 0xff;
    b.master.block_allocated_flag[1] = 0x3;
    
    for (int i = 0; i < N_BLOCKS_IN_DISK; i++) {
        if (vdisk_write_block(i, &b) != 0) {
            fprintf(stderr, "ERROR: writing 0 to block\n");
            return -1;
        }
    }
    
    //Initializing inode
    INODE inode;
    inode.data[0] = 9;
    inode.type = IT_DIRECTORY;
    for (int i = 1; i < 15; i++) {
        inode.data[i] = UNALLOCATED_INODE;
    }
    inode.n_references = 1;
    inode.size = 2;
    b.inodes.inode[0] = inode;
    
    //Writing inode to disk
    vdisk_write_block(1, &b);
    
    oufs_clean_directory_block(0, 0, &b);
    //Writing directories to disk
    vdisk_write_block(9, &b);
    
    //Return success
    return 0;
}

/**
 *  Find the first open bit in a byte
 *
 *  @param val The byte that contains an open bit
 *  @return 0-7 = bit to allocate in byte
 *          -1  = error occurred
 */
int oufs_find_open_bit(unsigned char val) {
    //If first bit is open
    if (!(val & 0x1)) {
        return 0;
    }
    
    //If second bit is open
    if (!(val & 0x2)) {
        return 1;
    }
    
    //If third bit is open
    if (!(val & 0x4)) {
        return 2;
    }
    
    //If fourth bit is open
    if (!(val & 0x8)) {
        return 3;
    }
    
    //If fifth bit is open
    if (!(val & 0x10)) {
        return 4;
    }
    
    //If sixth bit is open
    if (!(val & 0x20)) {
        return 5;
    }
    
    //If seventh bit is open
    if (!(val & 0x40)) {
        return 6;
    }
    
    //If eighth bit is open
    if (!(val & 0x80)) {
        return 7;
    }
    
    //Return error if no open bit found
    return -1;
}

/**
 * Initialize a directory block as an empty directory
 *
 * @param self Inode reference index for this directory
 * @param self Inode reference index for the parent directory
 * @param block The block containing the directory contents
 */
void oufs_clean_directory_block(INODE_REFERENCE self, INODE_REFERENCE parent, BLOCK *block)
{
    // Debugging output
    if(debug)
        fprintf(stderr, "New clean directory: self=%d, parent=%d\n", self, parent);
    
    // Create an empty directory entry
    DIRECTORY_ENTRY entry;
    oufs_clean_directory_entry(&entry);
    
    // Copy empty directory entries across the entire directory list
    for(int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
        block->directory.entry[i] = entry;
    }
    
    // Now we will set up the two fixed directory entries
    
    // Self
    strncpy(entry.name, ".", 2);
    entry.inode_reference = self;
    block->directory.entry[0] = entry;
    
    // Parent
    strncpy(entry.name, "..", 3);
    entry.inode_reference = parent;
    block->directory.entry[1] = entry;
}

/**
 * Allocate a new data block
 *
 * If one is found, then the corresponding bit in the block allocation table is set
 *
 * @return The index of the allocated data block.  If no blocks are available,
 * then UNALLOCATED_BLOCK is returned
 */
BLOCK_REFERENCE oufs_allocate_new_block()
{
    BLOCK block;
    // Read the master block
    vdisk_read_block(MASTER_BLOCK_REFERENCE, &block);
    
    // Scan for an available block
    int block_byte;
    int flag;
    
    // Loop over each byte in the allocation table.
    for(block_byte = 0, flag = 1; flag && block_byte < N_BLOCKS_IN_DISK / 8; ++block_byte) {
        if(block.master.block_allocated_flag[block_byte] != 0xff) {
            // Found a byte that has an opening: stop scanning
            flag = 0;
            break;
        };
    };
    // Did we find a candidate byte in the table?
    if(flag == 1) {
        // No
        if(debug)
            fprintf(stderr, "No open blocks\n");
        return(UNALLOCATED_BLOCK);
    }
    
    // Found an available data block
    
    // Set the block allocated bit
    // Find the FIRST bit in the byte that is 0 (we scan in bit order: 0 ... 7)
    int block_bit = oufs_find_open_bit(block.master.block_allocated_flag[block_byte]);
    
    // Now set the bit in the allocation table
    block.master.block_allocated_flag[block_byte] |= (1 << block_bit);
    
    // Write out the updated master block
    vdisk_write_block(MASTER_BLOCK_REFERENCE, &block);
    
    if(debug)
        fprintf(stderr, "Allocating block=%d (%d)\n", block_byte, block_bit);
    
    // Compute the block index
    BLOCK_REFERENCE block_reference = (block_byte << 3) + block_bit;
    
    if(debug)
        fprintf(stderr, "Allocating block=%d\n", block_reference);
    
    // Done
    return(block_reference);
}

/**
 * Allocate a new inode block
 *
 * If one is found, then the corresponding bit in the block allocation table is set
 *
 * @return The index of the allocated inode block. If no blocks are available,
 * then UNALLOCATED_BLOCK is returned
 */
INODE_REFERENCE oufs_allocate_new_inode() {
    BLOCK block;
    //Read master block
    vdisk_read_block(MASTER_BLOCK_REFERENCE, &block);
    
    //Scan for an available block
    int inode_byte;
    int flag;
    
    //Loop over each byte in the inode table
    for (inode_byte = 0, flag = 1; flag && inode_byte < N_INODES / 8; ++inode_byte) {
        //If byte is not full open byte was found: stop scanning
        if (block.master.inode_allocated_flag[inode_byte] != 0xff) {
            flag = 0;
            break;
        };
    };
    
    //Did we find a candidate byte in the table?
    if (flag == 1) {
        //No
        if (debug)
            fprintf(stderr, "No open blocks\n");
        return (IT_NONE);
    }
    
    //Found an available data block
    //Set the block allocated bit
    int inode_bit = oufs_find_open_bit(block.master.inode_allocated_flag[inode_byte]);

    //Now set the bit in the allocation table
    block.master.inode_allocated_flag[inode_byte] |= (1 << inode_bit);
    
    //Write to master block to update
    vdisk_write_block(MASTER_BLOCK_REFERENCE, &block);
    
    if (debug)
        fprintf(stderr, "Allocating block=%d (%d)\n", inode_byte, inode_bit);
    
    //Compute the inode index
    INODE_REFERENCE inode_reference = (inode_byte << 3) + inode_bit;
    
    if (debug)
        fprintf(stderr, "Allocating block=%d\n", inode_reference);
    
    return(inode_reference);
}

/**
 *  Given an inode reference, read the inode from the virtual disk.
 *
 *  @param i Inode reference (index into the inode list)
 *  @param inode Pointer to an inode memory structure.  This structure will be
 *                filled in before return)
 *  @return 0 = successfully loaded the inode
 *         -1 = an error has occurred
 */
int oufs_read_inode_by_reference(INODE_REFERENCE i, INODE *inode)
{
    if(debug)
        fprintf(stderr, "Fetching inode %d\n", i);
    
    // Find the address of the inode block and the inode within the block
    BLOCK_REFERENCE block = i / INODES_PER_BLOCK + 1;
    int element = (i % INODES_PER_BLOCK);
    
    BLOCK b;
    if(vdisk_read_block(block, &b) == 0) {
        // Successfully loaded the block: copy just this inode
        *inode = b.inodes.inode[element];
        return(0);
    }
    // Error case
    return(-1);
}

/**
 *  Given an inode reference, write the inode to the virtual disk.
 *
 *  @param i Inode Reference (index into the inode list)
 *  @param inode Pointer to an inode memeory structure. This structure will be
 *              written to the block)
 *  @return 0 = successfully written to block
 *         -1 = an error occurred
 */
int oufs_write_inode_by_reference(INODE_REFERENCE i, INODE *inode) {
    if (debug) {
        printf("INODE_REFERENCE in write_inode_by_reference: %d\n", i);
    }
    
    BLOCK_REFERENCE block = i / INODES_PER_BLOCK + 1;
    int element = (i % INODES_PER_BLOCK);
    
    BLOCK b;
    if (vdisk_read_block(block, &b) == 0) {
        //Put inode into element in block
        b.inodes.inode[element] = *inode;
        
        //Write block back to disk
        if (vdisk_write_block(block, &b) == 0) {
            return (0);
        }
    }
    return (-1);
}

/**
 *  Checks to see if an entry exists in a directory
 *
 *  @param BLOCK block The block to take an look through its entries
 *  @param char *base_name The name of the new entry to look through
 *  @param int flag If we are checking for the file in the list of entries
 *  @return True, false, 2 for Entry with same name if file, -2 for directory
 */
int check_for_entry(BLOCK *block, char *base_name, int flag) {
    //Check to see if basename is in the list of entries
    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
        if(block->directory.entry[i].inode_reference != UNALLOCATED_INODE) {
            if (!strcmp(block->directory.entry[i].name, base_name)) {
                
                //For file clarification
                if (flag == 1) {
                    INODE_REFERENCE base_inode;
                    INODE inode;
                    //Getting inode reference of entry found
                    base_inode = block->directory.entry[i].inode_reference;
                    //Get inode of inode reference
                    oufs_read_inode_by_reference(base_inode, &inode);
                    
                    if (inode.type == IT_FILE) {
                        return 1;
                    } else {
                        fprintf(stderr, "ERROR: Entry with same name found and is a directory, not file\n");
                        return 2;
                    }
                }
                return -1;
            }
        } else {
            break;
        }
    }
    return 0;
}

/**
 *  Gets the information of the specified entry and returns the inode reference of that entry
 *
 *  @param BLOCK_REFERENCE base_block the block reference of the parent block
 *  @param INODE_REFERENCE inode_reference inode referene of parent inode
 *  @param char *base_name Entry to find
 *  @return INODE_REFERENCE of the entry of base_name
 */
INODE_REFERENCE get_specified_entry(BLOCK_REFERENCE base_block, INODE_REFERENCE base_inode, char *base_name) {
    //Read in parent block
    BLOCK parent_block;
    vdisk_read_block(base_block, &parent_block);
    
    //Inode reference of entry to delete
    INODE_REFERENCE key_inode_reference;
    //Get inode reference of entry to delete
    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
        if (!strcmp(parent_block.directory.entry[i].name, base_name)) {
            key_inode_reference = parent_block.directory.entry[i].inode_reference;
            if (debug) {
                printf("Found base name: %s in block.directory.entry[i].name: %s\n", base_name, parent_block.directory.entry[i].name);
            }
            break;
        }
    }
    //Returning inode reference of specified entry
    return key_inode_reference;
}

/**
 *  This function removes only files. It removes them depending on the amount of n_references int the inode of the file you are trying to delete. If n_references is 1, then it resets the entire inode, deallocates on master tables, erases entry from parent directory, and memsets data blocks allocated to inode. If n_references is bigger than 1, then it only decrements n_references of it inode, and erases its entry in the parent directory.
 *
 *  @param BLOCK_REFERENCE base_block Block reference of parent block
 *  @param INODE_REFERENCE base_inode Inode reference of inode of parent
 *  @param char *base_name Name of entry to delete
 *  @return nothing
 */
void oufs_rmfile(BLOCK_REFERENCE base_block, INODE_REFERENCE base_inode, char *base_name) {
    //Read in parent block
    BLOCK parent_block;
    vdisk_read_block(base_block, &parent_block);
    
    if (debug) {
        printf("Base_name: %s\n", base_name);
    }
    
    //Inode reference of entry to delete
    INODE_REFERENCE inode_to_delete;
    int n_references = 1;
    //Get inode reference of entry to delete
    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
        if (!strcmp(parent_block.directory.entry[i].name, base_name)) {
            inode_to_delete = parent_block.directory.entry[i].inode_reference;
            if (debug) {
                printf("Found base name: %s in block.directory.entry[i].name: %s\n", base_name, parent_block.directory.entry[i].name);
            }
            
            //Check the n_references to know how to delete it
            INODE inode_of_to_delete;
            oufs_read_inode_by_reference(inode_to_delete, &inode_of_to_delete);
            //Get n_references from inode_of_to_delete (the inode we possibly need to delete)
            n_references = inode_of_to_delete.n_references;
            //Clean the entry
            oufs_clean_directory_entry(&parent_block.directory.entry[i]);
        }
    }
    //Writ block back to disk
    vdisk_write_block(base_block, &parent_block);
    
    //Decrement parent inode size
    INODE parent_inode;
    oufs_read_inode_by_reference(base_inode, &parent_inode);
    //Increment parent inode size
    parent_inode.size = parent_inode.size - 1;
    //Write inode parent inode back to the block
    oufs_write_inode_by_reference(base_inode, &parent_inode);
    
    //Reading in inode to delete
    INODE deleting_inode;
    oufs_read_inode_by_reference(inode_to_delete, &deleting_inode);
    if (n_references == 1) {
        //Go through inode data blocks, deallocate them, reset inode, deallocate in table, memset blocks
        for (int i = 0; i < BLOCKS_PER_INODE; ++i) {
            //If UNALLOCATED_INODE block found break
            if (deleting_inode.data[i] == UNALLOCATED_INODE) {
                break;
            }
            //Grab block reference
            BLOCK_REFERENCE old_block = deleting_inode.data[i];
            //Unallocate in inode
            deleting_inode.data[i] = UNALLOCATED_INODE;
            
            if (debug) {
                printf("Old block to reset: %d\n", old_block);
            }
            
            //Read in old block to empty out
            BLOCK empty_block;
            vdisk_read_block(old_block, &empty_block);
            //Reset raw data in block
            memset(empty_block.data.data, 0, sizeof(empty_block));
            //Write new empty block back
            vdisk_write_block(old_block, &empty_block);
            
            //Deallocate inode and block
            int old_block_index = old_block >> 3;
            int old_block_bit = old_block & 0x7;
            //Read in master block
            BLOCK block;
            vdisk_read_block(MASTER_BLOCK_REFERENCE, &block);
            //Deallocate block on master table
            block.master.block_allocated_flag[old_block_index] &= ~(1 << old_block_bit);
            //Write disk back to block
            vdisk_write_block(MASTER_BLOCK_REFERENCE, &block);
        }
        //Creating new empty inode
        INODE empty_inode;
        for (int i = 0; i < 15; i++) {
            empty_inode.data[i] = 0;
        }
        empty_inode.type = IT_NONE;
        empty_inode.size = 0;
        empty_inode.n_references = 0;
        //Writing empty inode
        oufs_write_inode_by_reference(inode_to_delete, &empty_inode);
        
        //Read in block
        BLOCK block;
        vdisk_read_block(MASTER_BLOCK_REFERENCE, &block);
        //Deallocate an reset inode of entry
        int old_inode_index = inode_to_delete >> 3;
        int old_inode_bit = inode_to_delete & 0x7;
        //Perform bitwise operations
        block.master.inode_allocated_flag[old_inode_index] &= ~(1 << old_inode_bit);
        //Write disk back to block
        vdisk_write_block(MASTER_BLOCK_REFERENCE, &block);
    } else {
        deleting_inode.n_references -= 1;
        oufs_write_inode_by_reference(inode_to_delete, &deleting_inode);
    }
}

/**
 *  Deletes a directory entry by formatting the directory block, decrementing inode size or parent inode, deallocating directory block in master table and deleting any references to the directory specified in the base_block that is found in the entries through base_name
 *
 *  @param BLOCK_REFERENCE base_block block of parent directory
 *  @param INODE_REFERENCE base_inode inode of parent directory
 *  @param char *base_name Name of entry to delete
 *  @return Nothing
 */
void oufs_rmdir(BLOCK_REFERENCE base_block, INODE_REFERENCE base_inode, char *base_name) {
    //Read in parent block
    BLOCK parent_block;
    vdisk_read_block(base_block, &parent_block);
    
    if (debug) {
        printf("Base_name: %s\n", base_name);
    }
    
    //Inode reference of entry to delete
    INODE_REFERENCE inode_to_delete;
    //Get inode reference of entry to delete
    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
        if (!strcmp(parent_block.directory.entry[i].name, base_name)) {
            inode_to_delete = parent_block.directory.entry[i].inode_reference;
            if (debug) {
                printf("Found base name: %s in block.directory.entry[i].name: %s\n", base_name, parent_block.directory.entry[i].name);
            }
            
            //Check to make sure that the directory is empty before deleting
            INODE inode_of_to_delete;
            oufs_read_inode_by_reference(inode_to_delete, &inode_of_to_delete);
            //Get block reference of entry to delete
            BLOCK_REFERENCE block_of_delete = inode_of_to_delete.data[0];
            BLOCK block_to_delete;
            vdisk_read_block(block_of_delete, &block_to_delete);
            //Go through block entries
            int delete_flag = 0;
            for (int j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; ++j) {
                if (!strcmp(block_to_delete.directory.entry[i].name, ".")) {
                    continue;
                } else if (!strcmp(block_to_delete.directory.entry[i].name, "..")) {
                    continue;
                } else if (!strcmp(block_to_delete.directory.entry[i].name, "")) {
                    continue;
                } else {
                    delete_flag = 1;
                }
            }
            
            if (delete_flag) {
                fprintf(stderr, "ERROR: Entries exist in the directory to delete, cannot delete directory\n");
                return;
            } else {
                //Clean the entry
                oufs_clean_directory_entry(&parent_block.directory.entry[i]);
                break;
            }
        }
    }
    
    //Writ block back to disk
    vdisk_write_block(base_block, &parent_block);
    
    if (debug) {
        printf("Inode reference to delete: %d\n", inode_to_delete);
    }
    
    //Decrement parent inode size
    INODE parent_inode;
    oufs_read_inode_by_reference(base_inode, &parent_inode);
    //Increment parent inode size
    parent_inode.size = parent_inode.size - 1;
    //Write inode parent inode back to the block
    oufs_write_inode_by_reference(base_inode, &parent_inode);
    
    //Getting dblock to reset
    INODE old_inode;
    oufs_read_inode_by_reference(inode_to_delete, &old_inode);
    BLOCK_REFERENCE old_block = old_inode.data[0];
    
    if (debug) {
        printf("Old block to reset: %d\n", old_block);
    }
    
    //Creating new empty inode
    INODE empty_inode;
    for (int i = 0; i < 15; i++) {
        empty_inode.data[i] = 0;
    }
    empty_inode.type = IT_NONE;
    empty_inode.size = 0;
    empty_inode.n_references = 0;
    //Writing empty inode
    oufs_write_inode_by_reference(inode_to_delete, &empty_inode);
    
    BLOCK empty_block;
    vdisk_read_block(old_block, &empty_block);
    DIRECTORY_ENTRY entry;
    oufs_clean_directory_entry(&entry);
    
    // Copy empty directory entries across the entire directory list
    for(int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
        empty_block.directory.entry[i] = entry;
    }
    //Write new empty block back
    vdisk_write_block(old_block, &empty_block);

    //Deallocate inode and block
    int old_block_index = old_block >> 3;
    int old_block_bit = old_block & 0x7;
    int old_inode_index = inode_to_delete >> 3;
    int old_inode_bit = inode_to_delete & 0x7;
    
    //Read in block
    BLOCK block;
    vdisk_read_block(MASTER_BLOCK_REFERENCE, &block);
    
    //Perform bitwise operations
    block.master.inode_allocated_flag[old_inode_index] &= ~(1 << old_inode_bit);
    block.master.block_allocated_flag[old_block_index] &= ~(1 << old_block_bit);
    
    //Write disk back to block
    vdisk_write_block(MASTER_BLOCK_REFERENCE, &block);
}

/**
 *  Creates an entry of a directory or file (depending on what the file_flag is). It allocates new references in the table for directory block an inode, but only inode for file. Write inode for both file and directory, but only a block for directory, not file.
 *
 *  @param INODE_REFERENCE base_inode Parent inode to increment
 *  @param BLOCK_REFERENCE base_block Parent block to perform operations on
 *  @param char *base_name Char containing base name of the entry to put into directory
 *  @param file_flag Flag specifiing if we are making directory or file
 *  @return 0 on success, -1 on failure
 */
int create_new_inode_and_block(INODE_REFERENCE base_inode, BLOCK_REFERENCE base_block, char *base_name, int file_flag) {
    BLOCK block;
    vdisk_read_block(base_block, &block);
    //Check for any open entries
    int open_entries = 1;
    //Looping through entries in block to check for empty entry
    for (int i = 2; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
        if (block.directory.entry[i].inode_reference == UNALLOCATED_INODE) {
            break;
        }
        if (i == 15 && block.directory.entry[i].inode_reference != UNALLOCATED_INODE) {
            open_entries = 0;
        }
    }
    
    if (open_entries == 0) {
        fprintf(stderr, "ERROR: no open entries in parent directory to put new file in\n");
        return -1;
    }
    
    vdisk_read_block(base_block, &block);
    //New references for inode and block of new directory
    INODE_REFERENCE inode_reference = oufs_allocate_new_inode();
    BLOCK_REFERENCE block_reference;
    
    if (file_flag == 0) {
        block_reference = oufs_allocate_new_block();
    }
    
    if (debug) {
        printf("inode reference: %d\n", inode_reference);
        printf("block reference: %d\n", block_reference);
    }
    
    //Setting up new inode
    INODE new_inode;
    
    //If what we are making is a file
    if (file_flag == 1) {
        new_inode.type = IT_FILE;
        new_inode.size = 0;
        for (int i = 0; i < 15; i++) {
            new_inode.data[i] = UNALLOCATED_INODE;
        }
    //If what we are making is a directory
    } else {
        new_inode.type = IT_DIRECTORY;
        new_inode.size = 2;
        new_inode.data[0] = block_reference;
        for (int i = 1; i < 15; i++) {
            new_inode.data[i] = UNALLOCATED_INODE;
        }
    }
    //References is the same for files and directories
    new_inode.n_references = 1;
    
    //Adding new inode
    oufs_write_inode_by_reference(inode_reference, &new_inode);
    
    //Adding new directory entry
    vdisk_read_block(base_block, &block);
    //Looping through entries in block to check for empty entry
    for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
        if (!strcmp(block.directory.entry[i].name, "")) {
            strcpy(block.directory.entry[i].name, base_name);
            block.directory.entry[i].inode_reference = inode_reference;
            break;
        }
    }
    //Writing entry to block
    vdisk_write_block(base_block, &block);
    
    if (file_flag == 0) {
        //Creating new block for new directory
        BLOCK new_block;
        //1 should be changed to inode reference
        oufs_clean_directory_block(inode_reference, base_inode, &new_block);
        //Writing directories to disk
        vdisk_write_block(block_reference, &new_block);
    }
    
    return 0;
    
}

/**
 *  Creates a new directory depending on what paramters are specified in the CWD or path. It first checks to see if the path is relative or absolute. If the path is relative, it will go through the current working directory starting at the root and find the necessary references. The new folder will be placed in the directory specified in path, therefore the code also moves through the arguments in path trying to find the correct references to put the new directory in. If the path is absolute, it will not use the current working directory as a means of getting the references. It will only use the path and its arguments to retrieve the correct references.
 *
 *  The parent has to exist if the path that is specified is absolute. It will throw an error if it is not.
 *
 *  @param cwd The current working directory of the environment
 *  @param path The path to create the new directory in
 *  @param operation Hold integer that specifies what operation to do. Operation can be 0 for mkdir, 1 for rmdir, or 2 for touching file, 3 is fore removing files
 *  @return 0 = success
 *         -1 = error occured
 */
int oufs_mkdir(char *cwd, char *path, int operation) {
    //Basename variable of path
    char base_name[128];
    char path_copy[128];
    strcpy(path_copy, path);
    strcpy(base_name, basename(path));
    
    //Varibles for tokenizing inputs of the parameter
    char *path_tokens[64];
    char **path_arg;
    //Tokenize input
    path_arg = path_tokens;
    *path_arg++ = strtok(path, "/");
    while ((*path_arg++ = strtok(NULL, "/")));
    
    if (strlen(base_name) > FILE_NAME_SIZE) {
        //clip(top_off_buff, size_top_off, boundary);
        clip(base_name, FILE_NAME_SIZE - 1, strlen(base_name));
    } else {
        //Do nothing
    }
    
    //Debug
    if (debug) {
        printf("CWD: %s\n", cwd);
        printf("Path: %s\n", path);
    }
    
    //Check to see if the path is relative or absolute
    if (path[0] != '/') {
        if (debug) {
            printf("Relative path\n");
        }
        
        //If cwd is root
        if (!strcmp(cwd, "/")) {
            //Get inode reference of base inode
            INODE_REFERENCE base_inode = 0;
            //Get block reference of base block
            BLOCK_REFERENCE base_block;
            
            //Get root inode
            INODE inode;
            oufs_read_inode_by_reference(base_inode, &inode);
            //Get block from block reference in inode
            BLOCK block;
            base_block = inode.data[0];
            //Read block at reference into block
            vdisk_read_block(base_block, &block);
            
            //Flag to determine if the entry already exists
            int flag = 0;
            //If path only has 1 token
            if (path_tokens[1] == NULL) {
                //Check the names of the entries
                flag = check_for_entry(&block, base_name, 0);
                //If path has more than 1 token means that we are trying to go for subdirectory
            } else {
                //Get parent inode to work off of
                base_inode = oufs_look_for_inode_from_root(path_copy, base_inode, base_name, 1);
                oufs_read_inode_by_reference(base_inode, &inode);
                base_block = inode.data[0];
                vdisk_read_block(base_block, &block);
                
                //Check entries in the block
                flag = check_for_entry(&block, base_name, 0);
            }
            
            //If operation is mkdir
            if (operation == 0) {
                //Checking if entry exists
                if (flag == 0) {
                    //Create new directory
                    if (create_new_inode_and_block(base_inode, base_block, base_name, 0) == 0) {
                        //Increment parent inode size
                        ++inode.size;
                        //Write inode parent inode back to the block
                        oufs_write_inode_by_reference(base_inode, &inode);
                    } else {
                        //DO NOTHING
                    }
                } else {
                    fprintf(stderr, "ERROR: Entry already exists, cannot create directory\n");
                    return (-1);
                }
            }
            //If operation is rmdir
            else if (operation == 1) {
                //Checking if entry exists
                if (flag == -1) {
                    //Erase file
                    oufs_rmdir(base_block, base_inode, base_name);
                } else {
                    fprintf(stderr, "ERROR: Entry does not already exists, cannot delete\n");
                    return (-1);
                }
            }
            //If operation is touch
            else if (operation == 2) {
                flag = check_for_entry(&block, base_name, 1);
                //Checking if entry exists
                if (flag == 0) {
                    //Create new file
                    if (create_new_inode_and_block(base_inode, base_block, base_name, 1) == 0) {
                        //Increment parent inode size
                        ++inode.size;
                        //Write inode parent inode back to the block
                        oufs_write_inode_by_reference(base_inode, &inode);
                    } else {
                        //DO NOTHING
                    }
                } else if (flag == 1) {
                    //DO NOTHING
                    return 0;
                } else if (flag == 2) {
                    fprintf(stderr, "ERROR: Entry already exists cannot create file\n");
                    return (-1);
                }
            } else if (operation == 3) {
                flag = check_for_entry(&block, base_name, 1);
                
                if (flag == 1) {
                    oufs_rmfile(base_block, base_inode, base_name);
                } else {
                    fprintf(stderr, "ERROR: specified file does not exist\n");
                }
            }
        //If cwd isn't root
        } else {
            //Get inode reference of base inode
            INODE_REFERENCE base_inode = oufs_look_for_inode_not_from_root(path_copy, cwd, 0, base_name, 1, 1);
            //Get block reference of base block
            BLOCK_REFERENCE base_block;
            
            //Get root inode
            INODE inode;
            oufs_read_inode_by_reference(base_inode, &inode);
            //Get block from block reference in inode
            BLOCK block;
            base_block = inode.data[0];
            //Read block at reference into block
            vdisk_read_block(base_block, &block);
            
            int flag = check_for_entry(&block, base_name, 0);
            
            //If operation is mkdir
            if (operation == 0) {
                //Checking if entry exists
                if (flag == 0) {
                    //Create new directory
                    if (create_new_inode_and_block(base_inode, base_block, base_name, 0) == 0) {
                        //Increment parent inode size
                        ++inode.size;
                        //Write inode parent inode back to the block
                        oufs_write_inode_by_reference(base_inode, &inode);
                    } else {
                        //DO NOTHING
                    }
                } else {
                    fprintf(stderr, "ERROR: Entry already exists, cannot create directory\n");
                    return (-1);
                }
            }
            //If operation is rmdir
            else if (operation == 1) {
                //Checking if entry exists
                if (flag == -1) {
                    //Erase file
                    oufs_rmdir(base_block, base_inode, base_name);
                } else {
                    fprintf(stderr, "ERROR: Entry does not already exists, cannot delete\n");
                    return (-1);
                }
            }
            //If operation is touch
            else if (operation == 2) {
                flag = check_for_entry(&block, base_name, 1);
                //Checking if entry exists
                if (flag == 0) {
                    //Create new file
                    if (create_new_inode_and_block(base_inode, base_block, base_name, 1) == 0) {
                        //Increment parent inode size
                        ++inode.size;
                        //Write inode parent inode back to the block
                        oufs_write_inode_by_reference(base_inode, &inode);
                    } else {
                        //DO NOTHING
                    }
                } else if (flag == 1) {
                    //DO NOTHING
                    return 0;
                } else if (flag == 2) {
                    fprintf(stderr, "ERROR: Entry already exists cannot create file\n");
                    return (-1);
                }
            } else if (operation == 3) {
                flag = check_for_entry(&block, base_name, 1);
                
                if (flag == 2) {
                    oufs_rmfile(base_block, base_inode, base_name);
                } else {
                    fprintf(stderr, "ERROR: specified file does not exist\n");
                }
            }
        }
        //Path is absolute
    } else {
        //Go through path, not cwd and check the if parent of path exists, if it doesn't, throw an errror
        if (debug) {
            printf("Absolute path\n");
        }
        //Get inode reference of base inode
        INODE_REFERENCE base_inode = oufs_look_for_inode_from_root(path_copy, 0, base_name, 1);
        BLOCK_REFERENCE base_block;
        
        //Get root inode
        INODE inode;
        oufs_read_inode_by_reference(base_inode, &inode);
        //Get block from block reference in inode
        BLOCK block;
        base_block = inode.data[0];
        //Read block at reference into block
        vdisk_read_block(base_block, &block);

        int flag = check_for_entry(&block, base_name, 0);
        
        //If operation is mkdir
        if (operation == 0) {
            //Checking if entry exists
            if (flag == 0) {
                //Create new directory
                if (create_new_inode_and_block(base_inode, base_block, base_name, 0) == 0) {
                    //Increment parent inode size
                    ++inode.size;
                    //Write inode parent inode back to the block
                    oufs_write_inode_by_reference(base_inode, &inode);
                } else {
                    //DO NOTHING
                }
            } else {
                fprintf(stderr, "ERROR: Entry already exists, cannot create directory\n");
                return (-1);
            }
        }
        //If operation is rmdir
        else if (operation == 1) {
            //Checking if entry exists
            if (flag == -1) {
                //Erase file
                oufs_rmdir(base_block, base_inode, base_name);
            } else {
                fprintf(stderr, "ERROR: Entry does not already exists, cannot delete\n");
                return (-1);
            }
        }
        //If operation is touch
        else if (operation == 2) {
            flag = check_for_entry(&block, base_name, 1);
            //Checking if entry exists
            if (flag == 0) {
                //Create new file
                if (create_new_inode_and_block(base_inode, base_block, base_name, 1) == 0) {
                    //Increment parent inode size
                    ++inode.size;
                    //Write inode parent inode back to the block
                    oufs_write_inode_by_reference(base_inode, &inode);
                } else {
                    //DO NOTHING
                }
            } else if (flag == 1) {
                //DO NOTHING
                return 0;
            } else if (flag == 2) {
                fprintf(stderr, "ERROR: Entry already exists cannot create file\n");
                return (-1);
            }
        } else if (operation == 3) {
            flag = check_for_entry(&block, base_name, 1);
            
            if (flag == 2) {
                oufs_rmfile(base_block, base_inode, base_name);
            } else {
                fprintf(stderr, "ERROR: specified file does not exist\n");
            }
        }
    }
    return (0);
}

/**
 *  Opens a file from cwd and path and gets the specifics of the file. Offset, inode reference, and mode is mutated in this function so that we know how to do specific operations on the file
 *
 *  @param char *cwd Path of cwd
 *  @param char *path Path of the file to find
 *  @param char *mode Mode to perform operations of the file on
 *  @return OUFILE* contianing the file specs
 */
OUFILE* oufs_fopen(char *cwd, char *path, char *mode) {
    //Basename variable of path
    char base_name[128];
    char path_copy[128];
    strcpy(path_copy, path);
    strcpy(base_name, basename(path));
    
    //Varibles for tokenizing inputs of the parameter
    char *path_tokens[64];
    char **path_arg;
    //Tokenize input
    path_arg = path_tokens;
    *path_arg++ = strtok(path, "/");
    while ((*path_arg++ = strtok(NULL, "/")));
    
    if (strlen(base_name) > FILE_NAME_SIZE) {
        //clip(top_off_buff, size_top_off, boundary);
        clip(base_name, FILE_NAME_SIZE - 1, strlen(base_name));
    } else {
        //Do nothing
    }
    
    //Check to see if the path is relative
    if (path[0] != '/') {
        //If cwd is the root
        if (!strcmp(cwd, "/")) {
            //Get inode reference of base inode
            INODE_REFERENCE base_inode = 0;
            //Get block reference of base block
            BLOCK_REFERENCE base_block;
            
            //Get root inode
            INODE inode;
            oufs_read_inode_by_reference(base_inode, &inode);
            //Get block from block reference in inode
            BLOCK block;
            base_block = inode.data[0];
            //Read block at reference into block
            vdisk_read_block(base_block, &block);
            
            //Flag to determine if the entry already exists
            int flag = 0;
            //If path only has 1 token
            if (path_tokens[1] == NULL) {
                //Check the names of the entries
                flag = check_for_entry(&block, base_name, 0);
                //If path has more than 1 token means that we are trying to go for subdirectory
            } else {
                //Get parent inode to work off of
                base_inode = oufs_look_for_inode_from_root(path_copy, base_inode, base_name, 1);
                oufs_read_inode_by_reference(base_inode, &inode);
                base_block = inode.data[0];
                vdisk_read_block(base_block, &block);
                
                //Check entries in the block
                flag = check_for_entry(&block, base_name, 0);
            }
            if (flag == -1) {
                //OUFILE structure to return
                OUFILE* file_specs = malloc(sizeof(OUFILE));
                
                //Get inode of file
                file_specs->inode_reference = get_specified_entry(base_block, base_inode, base_name);
                //Get mode of file
                file_specs->mode = *mode;
                //Get offset
                INODE file_inode;
                oufs_read_inode_by_reference(file_specs->inode_reference, &inode);
                
                file_specs->offset = inode.size;
                
                if (debug) {
                printf("\nOUFILE file_specs: %s\n\tinode_reference: %d\n\tmode: %c\n\toffset: %d\n", base_name, file_specs->inode_reference, file_specs->mode, file_specs->offset);
                }
                return file_specs;
            } else {
                OUFILE* file_specs = malloc(sizeof(OUFILE));
                file_specs->mode = *mode;
                file_specs->offset = 0;
                file_specs->inode_reference = UNALLOCATED_INODE;
                
                return file_specs;
            }
        //If cwd isn't root
        } else {
            //Get inode reference of base inode
            INODE_REFERENCE base_inode = oufs_look_for_inode_not_from_root(path_copy, cwd, 0, base_name, 1, 1);
            //Get block reference of base block
            BLOCK_REFERENCE base_block;
            
            //Get root inode
            INODE inode;
            oufs_read_inode_by_reference(base_inode, &inode);
            //Get block from block reference in inode
            BLOCK block;
            base_block = inode.data[0];
            //Read block at reference into block
            vdisk_read_block(base_block, &block);
            
            int flag = check_for_entry(&block, base_name, 0);
            
            if (flag == -1) {
                //OUFILE structure to return
                OUFILE* file_specs = malloc(sizeof(OUFILE));
                
                //Get inode of file
                file_specs->inode_reference = get_specified_entry(base_block, base_inode, base_name);
                //Get mode of file
                file_specs->mode = *mode;
                //Get offset
                INODE file_inode;
                oufs_read_inode_by_reference(file_specs->inode_reference, &inode);
                
                file_specs->offset = inode.size;
                
                if (debug) {
                    printf("\nOUFILE file_specs: %s\n\tinode_reference: %d\n\tmode: %c\n\toffset: %d\n", base_name, file_specs->inode_reference, file_specs->mode, file_specs->offset);
                }
                return file_specs;
            } else {
                OUFILE* file_specs = malloc(sizeof(OUFILE));
                file_specs->mode = *mode;
                file_specs->offset = 0;
                file_specs->inode_reference = UNALLOCATED_INODE;
                
                return file_specs;
            }
        }
    //Path is absolute
    } else {
        //Get inode reference of base inode
        INODE_REFERENCE base_inode = oufs_look_for_inode_from_root(path_copy, 0, base_name, 1);
        BLOCK_REFERENCE base_block;
        
        //Get root inode
        INODE inode;
        oufs_read_inode_by_reference(base_inode, &inode);
        //Get block from block reference in inode
        BLOCK block;
        base_block = inode.data[0];
        //Read block at reference into block
        vdisk_read_block(base_block, &block);
        
        int flag = check_for_entry(&block, base_name, 0);
        
        if (flag == -1) {
            //OUFILE structure to return
            OUFILE* file_specs = malloc(sizeof(OUFILE));
            
            //Get inode of file
            file_specs->inode_reference = get_specified_entry(base_block, base_inode, base_name);
            //Get mode of file
            file_specs->mode = *mode;
            //Get offset
            INODE file_inode;
            oufs_read_inode_by_reference(file_specs->inode_reference, &inode);
            //Get inode size
            file_specs->offset = inode.size;
            
            if (debug) {
                printf("\nOUFILE file_specs: %s\n\tinode_reference: %d\n\tmode: %c\n\toffset: %d\n", base_name, file_specs->inode_reference, file_specs->mode, file_specs->offset);
            }
            return file_specs;
        } else {
            OUFILE* file_specs = malloc(sizeof(OUFILE));
            file_specs->mode = *mode;
            file_specs->offset = 0;
            file_specs->inode_reference = UNALLOCATED_INODE;
            
            return file_specs;
        }
    }
    return NULL;
}

/**
 *  Writes the specified buffer into the raw data of the data block allocated to the file
 *
 *  @param BLOCK_REFERENCE block_reference Reference of block to write into
 *  @param INODE_REFERENCE inode_reference Inode reference of block to write into
 *  @param int len Length of buff to write
 *  @param char *buf Buffer to write into block
 *  @param int *offset Offset of the block, which changes with every write
 *  @return Nothing
 */
void regular_write(BLOCK_REFERENCE block_reference, INODE_REFERENCE inode_reference, int len, char * buf, int *offset) {
    BLOCK block;
    vdisk_read_block(block_reference, &block);
    
    //Concatenating sequence of strings
    if (debug) {
        printf("Offset in regular write: %d\n", *offset);
        printf("Buf: %s|||||||To block: %d\n", buf, block_reference);
    }
    
    //Copying buf to block at offset in block
    memcpy(&block.data.data[*offset], buf, strlen(buf));
    //Writing block back to disk
    vdisk_write_block(block_reference, &block);
    
    INODE inode;
    oufs_read_inode_by_reference(inode_reference, &inode);
    //Incrementing the size of inode
    if (debug) {
        printf("Inode.size: %d------Inode.size + len: %d\n:", inode.size, inode.size + len);
    }
    //Increment size and offset
    inode.size += len;
    *offset += len;
    //Writing inode back with new size
    oufs_write_inode_by_reference(inode_reference, &inode);
}

/**
 *  Is used when we transition from writing to one block to writing to a new block. If the specified length of the the inode.size + the length of the buffer is > a % of (256 * datablock), then we allocate a new block, top off old block with part of the string, then write the rest of the string to the new block.
 *
 *  @param BLOCK_REFERENCE block_reference Reference of block to write into
 *  @param INODE_REFERENCE inode_reference Inode reference of block to write into
 *  @param int len Length of buff to write
 *  @param char *buf Buffer to write into block
 *  @param int data_block Which data block to write into in block.data[]
 *  @param int *offset Offset of the block, which changes with every write
 *  @return Nothing
 */
void bleed_write(BLOCK_REFERENCE block_reference, INODE_REFERENCE inode_reference, int len, char * buf, int data_block, int *offset) {
    //Getting inode
    INODE inode;
    oufs_read_inode_by_reference(inode_reference, &inode);
    //Getting parameters of clip function
    int total_size = inode.size + len;
    int boundary = total_size - (256 * data_block);
    int size_top_off = len - boundary;
    
    //Creating new buffers to hold new strs from clip
    char top_off_buff[MAX_BUFFER];
    char remainder_buff[MAX_BUFFER];
    //Copying buff to new chars to be mutated
    memcpy(top_off_buff, buf, strlen(buf));
    memcpy(remainder_buff, buf, strlen(buf));
    
    //Cliping new strings
    clip(top_off_buff, size_top_off, boundary);
    
    if (inode.size + len == (256 * data_block)) {
        //Toping off old block
        BLOCK block;
        vdisk_read_block(block_reference, &block);
        //Add top_off_buff to block at block 0 in inode
        memcpy(&block.data.data[*offset], top_off_buff, strlen(top_off_buff));
        //Write block back
        vdisk_write_block(block_reference, &block);
        
        *offset = 256;
        //Incrementing the size of inode
        inode.size += len;
        //Write new block in inode
        oufs_write_inode_by_reference(inode_reference, &inode);
    } else {
        
        clip(remainder_buff, 0, size_top_off);
        
        if (debug) {
            printf("\n");
            printf("inode.size: %d\n", inode.size);
            printf("top_off_buff: %s len: %lu\n", top_off_buff, strlen(top_off_buff));
            printf("remainder_buff: %s len: %lu\n", remainder_buff, strlen(remainder_buff));
            printf("\n");
        }
        
        //Toping off old block
        BLOCK block;
        vdisk_read_block(block_reference, &block);
        //Add top_off_buff to block at block 0 in inode
        memcpy(&block.data.data[*offset], top_off_buff, strlen(top_off_buff));
        //Write block back
        vdisk_write_block(block_reference, &block);
        
        //Writing rest of string to new block
        block_reference = oufs_allocate_new_block();
        //Writing new info to inode
        inode.data[data_block] = block_reference;
        //Incrementing the size of inode
        inode.size += len;
        //Write new block in inode
        oufs_write_inode_by_reference(inode_reference, &inode);
        
        //Reading in new block
        vdisk_read_block(block_reference, &block);
        //Resetting block, so that the raw data is cleaned
        memset(block.data.data, 0, sizeof(block));
        //Add remainder_buff to new block at block 1 in inode
        memcpy(&block.data.data[0], remainder_buff, strlen(remainder_buff));
        //Reset offset
        *offset = len - size_top_off;
        //Write block back to disk
        vdisk_write_block(block_reference, &block);
    }
}

/**
 *  The main function that uses regular_write and bleed_write to write to data block of inodes. There is some prep work in determining what blocks we want to use since we are writing by string, and not character. It determines what the offset is to make descisions about which data block to use in the inode, so that it writes correctly to each block in the inode
 *
 *  @param OUFILE *fp Holds the file specifications that we need like the inode reference and the offset of the file
 *  @param char *buf The string to write to the data blocks
 *  @param int len The length of what we want to write
 *  @return 0 on success, anything else is error
 */
int oufs_fwrite(OUFILE *fp, char * buf, int len) {

    //First grab inode
    INODE inode;
    oufs_read_inode_by_reference(fp->inode_reference, &inode);
    BLOCK_REFERENCE block_reference = inode.data[0];
    
    //If first data block is unallocated, it means its an empty inode, no blocks
    if (block_reference == UNALLOCATED_INODE) {
        block_reference = oufs_allocate_new_block();
        
        //Writing new info to inode
        inode.data[0] = block_reference;
        oufs_write_inode_by_reference(fp->inode_reference, &inode);
        
        //Clean block
        BLOCK block;
        //Read block at reference into block
        vdisk_read_block(block_reference, &block);
        //Reset data in the block
        memset(block.data.data, 0, sizeof(block));
        //Write block back to disk
        vdisk_write_block(block_reference, &block);
    }
    
    //If block becomes all the way full, allocate new block to next unallocated inode
    if (fp->offset == 256) {
        block_reference = oufs_allocate_new_block();
        
        //Finding first unallocated inode block in inode.data and setting it equal to block_reference
        for (int i = 0; i < BLOCKS_PER_INODE; ++i) {
            if (inode.data[i] == UNALLOCATED_INODE) {
                inode.data[i] = block_reference;
                oufs_write_inode_by_reference(fp->inode_reference, &inode);
                break;
            }
        }
        
        //Clean block
        BLOCK block;
        //Read block at reference into block
        vdisk_read_block(block_reference, &block);
        //Reset data in the block
        memset(block.data.data, 0, sizeof(block));
        //Write block back to disk
        vdisk_write_block(block_reference, &block);
        //Reset offset
        fp->offset = 0;
    }
    //printf("Offset: %d\n", fp->offset);
    
    /*  Finds which data block to write to so that everything is written in the correct spot as the file is to write is written. Everything works off of what the overall inode.size is, and depending on that, it writes to the correct data blocks in the inode
     */
    if (inode.size < 256) {
        if (inode.size + len >= 256) {
            bleed_write(inode.data[0], fp->inode_reference, len, buf, 1, &fp->offset);
        } else {
            regular_write(inode.data[0], fp->inode_reference, len, buf, &fp->offset);
        }
    } else if (inode.size < 512) {
        if (inode.size + len >= 512) {
            bleed_write(inode.data[1], fp->inode_reference, len, buf, 2, &fp->offset);
        } else {
            regular_write(inode.data[1], fp->inode_reference, len, buf, &fp->offset);
        }
    } else if (inode.size < 768) {
        if (inode.size + len >= 768) {
            bleed_write(inode.data[2], fp->inode_reference, len, buf, 3, &fp->offset);
        } else {
            regular_write(inode.data[2], fp->inode_reference, len, buf, &fp->offset);
        }
    } else if (inode.size < 1024) {
        if (inode.size + len >= 1024) {
            bleed_write(inode.data[3], fp->inode_reference, len, buf, 4, &fp->offset);
        } else {
            regular_write(inode.data[3], fp->inode_reference, len, buf, &fp->offset);
        }
    } else if (inode.size < 1280) {
        if (inode.size + len >= 1280) {
            bleed_write(inode.data[4], fp->inode_reference, len, buf, 5, &fp->offset);
        } else {
            regular_write(inode.data[4], fp->inode_reference, len, buf, &fp->offset);
        }
    } else if (inode.size < 1536) {
        if (inode.size + len >= 1536) {
            bleed_write(inode.data[5], fp->inode_reference, len, buf, 6, &fp->offset);
        } else {
            regular_write(inode.data[5], fp->inode_reference, len, buf, &fp->offset);
        }
    } else if (inode.size < 1792) {
        if (inode.size + len >= 1792) {
            bleed_write(inode.data[6], fp->inode_reference, len, buf, 7, &fp->offset);
        } else {
            regular_write(inode.data[6], fp->inode_reference, len, buf, &fp->offset);
        }
    } else if (inode.size < 2048) {
        if (inode.size + len >= 2048) {
            bleed_write(inode.data[7], fp->inode_reference, len, buf, 8, &fp->offset);
        } else {
            regular_write(inode.data[7], fp->inode_reference, len, buf, &fp->offset);
        }
    } else if (inode.size < 2034) {
        if (inode.size + len >= 2034) {
            bleed_write(inode.data[8], fp->inode_reference, len, buf, 9, &fp->offset);
        } else {
            regular_write(inode.data[8], fp->inode_reference, len, buf, &fp->offset);
        }
    }  else if (inode.size < 2560) {
        if (inode.size + len >= 2560) {
            bleed_write(inode.data[9], fp->inode_reference, len, buf, 10, &fp->offset);
        } else {
            regular_write(inode.data[9], fp->inode_reference, len, buf, &fp->offset);
        }
    } else if (inode.size < 2816) {
        if (inode.size + len >= 2816) {
            bleed_write(inode.data[10], fp->inode_reference, len, buf, 11, &fp->offset);
        } else {
            regular_write(inode.data[10], fp->inode_reference, len, buf, &fp->offset);
        }
    } else if (inode.size < 3072) {
        if (inode.size + len >= 3072) {
            bleed_write(inode.data[11], fp->inode_reference, len, buf, 12, &fp->offset);
        } else {
            regular_write(inode.data[11], fp->inode_reference, len, buf, &fp->offset);
        }
    } else if (inode.size < 3328) {
        if (inode.size + len >= 3328) {
            bleed_write(inode.data[12], fp->inode_reference, len, buf, 13, &fp->offset);
        } else {
            regular_write(inode.data[12], fp->inode_reference, len, buf, &fp->offset);
        }
    }  else if (inode.size < 3584) {
        if (inode.size + len >= 3584) {
            bleed_write(inode.data[13], fp->inode_reference, len, buf, 14, &fp->offset);
        } else {
            regular_write(inode.data[13], fp->inode_reference, len, buf, &fp->offset);
        }
    }
    return 0;
}

/**
 *  It works like oufs_mkdir in the it steps through the cwd and path properly so that the file is put into the correct location. Then it just increments the parent inode size, n references in the dest file inode, and adds an entry of the file in the correct directory
 *
 *  @param char *cwd The path of the CWD
 *  @param char *path The path of the file to link
 *  @param INODE_REFERENCE dest_reference Reference of destination file
 *  @return 0 on success, and -1 on error
 */
int oufs_link(char *cwd, char *path, INODE_REFERENCE dest_reference) {
    //Basename variable of path
    char base_name[128];
    char path_copy[128];
    strcpy(path_copy, path);
    strcpy(base_name, basename(path));
    
    //Varibles for tokenizing inputs of the parameter
    char *path_tokens[64];
    char **path_arg;
    //Tokenize input
    path_arg = path_tokens;
    *path_arg++ = strtok(path, "/");
    while ((*path_arg++ = strtok(NULL, "/")));
    
    if (strlen(base_name) > FILE_NAME_SIZE) {
        //clip(top_off_buff, size_top_off, boundary);
        clip(base_name, FILE_NAME_SIZE - 1, strlen(base_name));
    } else {
        //Do nothing
    }
    
    //Debug
    if (debug) {
        printf("CWD: %s\n", cwd);
        printf("Path: %s\n", path);
    }
    
    //Check to see if the path is relative or absolute
    if (path[0] != '/') {
        if (debug) {
            printf("Relative path\n");
        }
        
        //If cwd is root
        if (!strcmp(cwd, "/")) {
            //Get inode reference of base inode
            INODE_REFERENCE base_inode = 0;
            //Get block reference of base block
            BLOCK_REFERENCE base_block;
            
            //Get root inode
            INODE inode;
            oufs_read_inode_by_reference(base_inode, &inode);
            //Get block from block reference in inode
            BLOCK block;
            base_block = inode.data[0];
            //Read block at reference into block
            vdisk_read_block(base_block, &block);
            
            //Flag to determine if the entry already exists
            int flag = 0;
            //If path only has 1 token
            if (path_tokens[1] == NULL) {
                //Check the names of the entries
                flag = check_for_entry(&block, base_name, 1);
                //If path has more than 1 token means that we are trying to go for subdirectory
            } else {
                //Get parent inode to work off of
                base_inode = oufs_look_for_inode_from_root(path_copy, base_inode, base_name, 1);
                oufs_read_inode_by_reference(base_inode, &inode);
                base_block = inode.data[0];
                vdisk_read_block(base_block, &block);
                
                //Check entries in the block
                flag = check_for_entry(&block, base_name, 1);
            }
            
            //Entry does not exist
            if (flag == 0) {
                //Increment parent inode size
                ++inode.size;
                //Write inode parent inode back to the block
                oufs_write_inode_by_reference(base_inode, &inode);
                
                oufs_read_inode_by_reference(dest_reference, &inode);
                ++inode.n_references;
                oufs_write_inode_by_reference(dest_reference, &inode);
                
                //Looping through entries in block to check for empty entry
                for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
                    if (!strcmp(block.directory.entry[i].name, "")) {
                        strcpy(block.directory.entry[i].name, base_name);
                        block.directory.entry[i].inode_reference = dest_reference;
                        break;
                    }
                }
                //Writing entry to block
                vdisk_write_block(base_block, &block);
                
            } else {
                fprintf(stderr, "ERROR: Entry already exists\n");
                return (-1);
            }
            //If cwd isn't root
        } else {
            //Get inode reference of base inode
            INODE_REFERENCE base_inode = oufs_look_for_inode_not_from_root(path_copy, cwd, 0, base_name, 1, 1);
            //Get block reference of base block
            BLOCK_REFERENCE base_block;
            
            //Get root inode
            INODE inode;
            oufs_read_inode_by_reference(base_inode, &inode);
            //Get block from block reference in inode
            BLOCK block;
            base_block = inode.data[0];
            //Read block at reference into block
            vdisk_read_block(base_block, &block);
            
            int flag = check_for_entry(&block, base_name, 1);
            //If no entry for new directory
            if (flag == 0) {
                //Increment parent inode size
                ++inode.size;
                //Write inode parent inode back to the block
                oufs_write_inode_by_reference(base_inode, &inode);
                
                oufs_read_inode_by_reference(dest_reference, &inode);
                ++inode.n_references;
                oufs_write_inode_by_reference(dest_reference, &inode);
                
                //Looping through entries in block to check for empty entry
                for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
                    if (!strcmp(block.directory.entry[i].name, "")) {
                        strcpy(block.directory.entry[i].name, base_name);
                        block.directory.entry[i].inode_reference = dest_reference;
                        break;
                    }
                }
                //Writing entry to block
                vdisk_write_block(base_block, &block);
            //Entry exists in the directory entry list
            } else {
                fprintf(stderr, "ERROR: Entry already exists\n");
                return (-1);
            }
        }
        //Path is absolute
    } else {
        //Go through path, not cwd and check the if parent of path exists, if it doesn't, throw an errror
        if (debug) {
            printf("Absolute path\n");
        }
        //Get inode reference of base inode
        INODE_REFERENCE base_inode = oufs_look_for_inode_from_root(path_copy, 0, base_name, 1);
        BLOCK_REFERENCE base_block;
        
        //Get root inode
        INODE inode;
        oufs_read_inode_by_reference(base_inode, &inode);
        //Get block from block reference in inode
        BLOCK block;
        base_block = inode.data[0];
        //Read block at reference into block
        vdisk_read_block(base_block, &block);
        
        int flag = check_for_entry(&block, base_name, 1);
        
        //If no entry for new directory
        if (flag == 0) {
            //Increment parent inode size
            ++inode.size;
            //Write inode parent inode back to the block
            oufs_write_inode_by_reference(base_inode, &inode);
            
            oufs_read_inode_by_reference(dest_reference, &inode);
            ++inode.n_references;
            oufs_write_inode_by_reference(dest_reference, &inode);
            
            //Looping through entries in block to check for empty entry
            for (int i = 0; i < DIRECTORY_ENTRIES_PER_BLOCK; ++i) {
                if (!strcmp(block.directory.entry[i].name, "")) {
                    strcpy(block.directory.entry[i].name, base_name);
                    block.directory.entry[i].inode_reference = dest_reference;
                    break;
                }
            }
            //Writing entry to block
            vdisk_write_block(base_block, &block);
        } else {
            fprintf(stderr, "ERROR: Entry already exists\n");
            return (-1);
        }
    }
    return (0);
}
