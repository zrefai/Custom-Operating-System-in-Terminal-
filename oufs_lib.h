#ifndef OUFS_LIB
#define OUFS_LIB
#include "oufs.h"

#define MAX_PATH_LENGTH 200

void oufs_get_environment(char *cwd, char *disk_name);  //ALIVE

int oufs_format_disk(char  *virtual_disk_name);   //ALIVE
int oufs_read_inode_by_reference(INODE_REFERENCE i, INODE *inode);  //ALIVE
int oufs_write_inode_by_reference(INODE_REFERENCE i, INODE *inode);  //ALIVE
int oufs_mkdir(char *cwd, char *path, int operation);  //ALIVE
void oufs_rmdir(BLOCK_REFERENCE base_block, INODE_REFERENCE base_inode, char *base_name);

void oufs_clean_directory_block(INODE_REFERENCE self, INODE_REFERENCE parent, BLOCK *block);    //ALIVE
void oufs_clean_directory_entry(DIRECTORY_ENTRY *entry);    //ALIVE
BLOCK_REFERENCE oufs_allocate_new_block();  //ALIVE
INODE_REFERENCE oufs_allocate_new_inode();  //ALIVE

INODE_REFERENCE oufs_look_for_inode_from_root(char *path_tokens, INODE_REFERENCE base_inode, char *base_name, int mkdir_flag);  //ALIVE
INODE_REFERENCE oufs_look_for_inode_not_from_root(char *path, char *cwd, INODE_REFERENCE base_inode, char *base_name, int path_flag, int mkdir_flag);  //ALIVE
int string_compare(const void *directory_entry_a, const void *directory_entry_b);   //ALIVE
int oufs_find_open_bit(unsigned char value);    //ALIVE
void list_directory_entries(INODE *inode, BLOCK *block, char *base_name);    //ALIVE
int create_new_inode_and_block(INODE_REFERENCE base_inode, BLOCK_REFERENCE base_block, char *base_name, int file_flag);    //ALIVE
int check_for_entry(BLOCK *block, char *base_name, int flag);   //ALIVE
int clip(char *str, int begin, int len);
void regular_write(BLOCK_REFERENCE block_reference, INODE_REFERENCE inode_reference, int len, char * buf, int *offset);
void bleed_write(BLOCK_REFERENCE block_reference, INODE_REFERENCE inode_reference, int len, char * buf, int data_block, int *offset);

INODE_REFERENCE get_specified_entry(BLOCK_REFERENCE base_block, INODE_REFERENCE base_inode, char *base_name);
OUFILE* oufs_fopen(char *cwd, char *path, char *mode);
int oufs_fwrite(OUFILE *fp, char * buf, int len);
int oufs_link(char *cwd, char *path, INODE_REFERENCE dest_reference);
void oufs_rmfile(BLOCK_REFERENCE base_block, INODE_REFERENCE base_inode, char *base_name);
#endif
