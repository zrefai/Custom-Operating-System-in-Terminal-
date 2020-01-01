#include <string.h>
#include "oufs_lib.h"
#include "oufs.h"

#define SEPARATORS " \t\n"
#define MAX_BUFFER 2000

int main(int argc, char** argv) {
    // Fetch the key environment vars
    char buf[MAX_BUFFER];
    char cwd[MAX_PATH_LENGTH];
    char disk_name[MAX_PATH_LENGTH];
    oufs_get_environment(cwd, disk_name);
    
    // Check arguments
    if(argc == 2) {
        // Open the virtual disk
        vdisk_disk_open(disk_name);
        
        //Main inode to write
        INODE inode;
        //Getting file specs
        OUFILE file_specs = *oufs_fopen(cwd, argv[1], "a");
        
        //If inode reference == -1, file does not exist, needs to be created
        if (file_specs.inode_reference == UNALLOCATED_INODE) {
            //printf("File does not exist\n");
            // Make the specified file
            if (oufs_mkdir(cwd, argv[1], 2) == -1) {
                exit(EXIT_FAILURE);
            }
            //Gets the file specs of the newly created file
            file_specs = *oufs_fopen(cwd, argv[1], "w");
            
            //Reading in inode
            oufs_read_inode_by_reference(file_specs.inode_reference, &inode);

            //Checking data blocks of inode, they need to be truncated
            for (int i = 0; i < BLOCKS_PER_INODE; ++i) {
                if (inode.data[i] == UNALLOCATED_INODE) {
                    continue;
                } else {
                    BLOCK_REFERENCE block_reference = inode.data[i];
                    //Get block from block reference in inode
                    BLOCK block;
                    //Read block at reference into block
                    vdisk_read_block(block_reference, &block);
                    //Reset data in the block
                    memset(block.data.data, 0, sizeof(block));
                    //Write block back to disk
                    vdisk_write_block(block_reference, &block);
                }
            }
            
            //Get input from STDIN
            while (!feof(stdin)) {
                if (fgets(buf, MAX_BUFFER, stdin)) {
                    //printf("%lu: %s\n", strlen(buf), buf);
                    oufs_fwrite(&file_specs, buf, strlen(buf));
                    //printf("Offset: %d\n", file_specs.offset);
                }
            }
        } else {
            //printf("File exists\n");
            //Reading in inode
            oufs_read_inode_by_reference(file_specs.inode_reference, &inode);
            
            int counter = 0;
            //Calculate offset from inode size
            for (int i = 0; i < BLOCKS_PER_INODE; ++i) {
                if (inode.data[i] != UNALLOCATED_INODE) {
                    ++counter;
                }
            }
            
            if (inode.size % 256 == 0) {
                file_specs.offset = 256;
            } else {
                //Calculating offset for file
                int difference = (256 * counter) - inode.size;
                //Put it in terms of indexes and find true offset
                file_specs.offset = 255 - difference;
            }
            
            ++file_specs.offset;
            //Get input from STDIN
            while (!feof(stdin)) {
                if (fgets(buf, MAX_BUFFER, stdin)) {
                    //printf("%lu: %s", strlen(buf), buf);
                    oufs_fwrite(&file_specs, buf, strlen(buf));
                }
            }
        }
    } else {
        // Wrong number of parameters
        fprintf(stderr, "Usage: zcreate <filename>\n");
    }
    
}
