#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "oufs_lib.h"

#define debug 0

/**
 *  Prints out the directory entries in a directory in sorted order based off of the CWD and the diretory they want to access. Works off of different cases. First it checks whether or not a parameter was specified for path. If not it just prints out the contents of the current working directory. If the current working directory is not the root, a for loop is used to find the corresponding inode and block references so that it can print the contents of that blocks directory entries. If the path is specified and the current working directory is the root, it starts at the root and then uses the path to determine the correct references. If the current working directory is not the root and a path is specified, then it moves through the current working directory, then the path gathering the correct references needed to print out the entries in the directory.
 *
 *  To print out the directories in order, it uses qsort with the child function string_compare. It also checks to see if the directory entry is a file or directory and prints out the / character at the appropriate entries.
 *
 *  @param argc The number of parameters from the command line
 *  @param argv The array containing the parameters from the command line
 *  @return Nothing to return
 */
int main(int argc, char** argv) {
    char cwd[256];
    char disk_name[256];
    char base_name[128];
    
    //Get environmental variables
    oufs_get_environment(cwd, disk_name);
    // Open the virtual disk
    vdisk_disk_open(disk_name);
     
    //If zfilez contains a parameter
    if (argc == 2) {
        strcpy(base_name, basename(argv[1]));
        
        //If cwd is root
        if (!strcmp(cwd, "/")) {
            //Get inode reference of base inode
            INODE_REFERENCE base_inode = oufs_look_for_inode_from_root(argv[1], 0, base_name, 0);
            BLOCK_REFERENCE base_block;
            
            //Get root inode
            INODE inode;
            oufs_read_inode_by_reference(base_inode, &inode);
            
            //Get block from block reference in inode if inode is a directory
            if (inode.type == IT_DIRECTORY) {
                BLOCK block;
                base_block = inode.data[0];
                //Read block at reference into block
                vdisk_read_block(base_block, &block);
                list_directory_entries(&inode, &block, base_name);
            } else {
                printf("%s\n", base_name);
            }
            
            
        //If cwd is not the root
        } else {
            //Get inode reference of base inode
            INODE_REFERENCE base_inode = oufs_look_for_inode_not_from_root(argv[1], cwd, 0, base_name, 1, 0);
            BLOCK_REFERENCE base_block;
            
            //Get root inode
            INODE inode;
            oufs_read_inode_by_reference(base_inode, &inode);
            //Get block from block reference in inode if inode is a directory
            if (inode.type == IT_DIRECTORY) {
                BLOCK block;
                base_block = inode.data[0];
                //Read block at reference into block
                vdisk_read_block(base_block, &block);
                list_directory_entries(&inode, &block, base_name);
            } else {
                printf("%s\n", base_name);
            }
            
            
        }
        
    //If zfilez does not contain a parameter
    } else {
        //If current working directory is the root
        if (!strcmp(cwd, "/")) {
            //Get inode reference of base inode
            INODE_REFERENCE base_inode = 0;
            BLOCK_REFERENCE base_block;
            
            //Get root inode
            INODE inode;
            oufs_read_inode_by_reference(base_inode, &inode);
            //Get block from block reference in inode if inode is a directory
            if (inode.type == IT_DIRECTORY) {
                BLOCK block;
                base_block = inode.data[0];
                //Read block at reference into block
                vdisk_read_block(base_block, &block);
                //Lists all of the entries in the directory
                list_directory_entries(&inode, &block, base_name);
            } else {
                printf("%s\n", base_name);
            }
            
          
            
            exit(EXIT_SUCCESS);
        //If current working directory is not the root
        } else {
            //Getting references for root
            INODE_REFERENCE base_inode = oufs_look_for_inode_not_from_root(argv[1], cwd, 0, base_name, 0, 0);
            BLOCK_REFERENCE base_block;
            
            //Get root inode
            INODE inode;
            oufs_read_inode_by_reference(base_inode, &inode);
            //Get block from block reference in inode if inode is a directory
            if (inode.type == IT_DIRECTORY) {
                BLOCK block;
                base_block = inode.data[0];
                //Read block at reference into block
                vdisk_read_block(base_block, &block);
                //Lists all of the entries in the directory
                list_directory_entries(&inode, &block, base_name);
            } else {
                printf("%s\n", base_name);
            }
            
            
        }
    }
    //Close the disk
    vdisk_disk_close();
}
