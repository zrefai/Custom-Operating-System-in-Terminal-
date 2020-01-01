#include <stdio.h>
#include "oufs_lib.h"

int main(int argc, char** argv) {
    //Sets buff
    setbuf(stdout,NULL);
    // Fetch the key environment vars
    char cwd[MAX_PATH_LENGTH];
    char disk_name[MAX_PATH_LENGTH];
    oufs_get_environment(cwd, disk_name);
    
    // Open the virtual disk
    vdisk_disk_open(disk_name);
    
    //Opening file for reading
    OUFILE file_specs = *oufs_fopen(cwd, argv[1], "r");
    
    //Getting inode
    INODE inode;
    oufs_read_inode_by_reference(file_specs.inode_reference, &inode);
    
    for (int i = 0; i < BLOCKS_PER_INODE; ++i) {
        if (inode.data[i] == UNALLOCATED_INODE) {
            break;
        } else {
            BLOCK_REFERENCE block_reference = inode.data[i];
            //Get block from block reference in inode
            BLOCK block;
            //Read block at reference into block
            vdisk_read_block(block_reference, &block);
            
            for (int j = 0; j < 256; ++j) {
                if (block.data.data[j] == NULL) {
                    break;
                } else {
                    fprintf(stdout, "%c", block.data.data[j]);
                }
            }
        }
    }
}
