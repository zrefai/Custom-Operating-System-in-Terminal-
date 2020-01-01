#include <stdio.h>
#include "oufs_lib.h"

int main(int argc, char** argv) {
    // Fetch the key environment vars
    char cwd[MAX_PATH_LENGTH];
    char disk_name[MAX_PATH_LENGTH];
    oufs_get_environment(cwd, disk_name);
    
    if (argc == 3) {
        // Open the virtual disk
        vdisk_disk_open(disk_name);
        
        //Check to see if the destfile exists
        //Getting file specs
        OUFILE destfile = *oufs_fopen(cwd, argv[1], "r");
        //Check to see if the file exists
        if (destfile.inode_reference != UNALLOCATED_INODE) {
            //Check to see if newfile does not exist
            OUFILE newfile = *oufs_fopen(cwd, argv[2], "r");
            
            //Check to see if the new file does not exist
            if (newfile.inode_reference == UNALLOCATED_INODE) {
                //Create new file
                if (oufs_link(cwd, argv[2], destfile.inode_reference) != 0) {
                    printf("ERROR: linking failed\n");
                }
            } else {
                fprintf(stderr, "ERROR: newfile does not exist\n");
            }
        } else {
            fprintf(stderr, "ERROR: destfile does not exist\n");
        }
    } else {
        // Wrong number of parameters
        fprintf(stderr, "Usage: zcreate <destfile> <newfile>\n");
    }
    
    return(0);
}
