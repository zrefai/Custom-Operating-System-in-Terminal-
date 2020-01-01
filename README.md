# Customer-Operating-System-in-Terminal-

An operating system I built to run in the terminal

# Getting Started

To run code, first run the make file and the zformat executable. Then add delete or inspect folders using zmkdir, zrmdir, or zinspect. To list files of a path or current working directory use zfilez. Update: Added functionality includes making files through ztouch, zcreate, and zappend. You can also remove files through zremove, and you can connect files through zlink.

# Commands

zformat - Creates an initial empty disk. Calling zformat after using disk will "reset" the disk. (No parameters needed)

zfilez <path> - <path> is optional in zfilez. If no path specified, it will print out the directory entries in the current working directory. If a path is specified it will print out the contents of that directory if the path is a relative or absolute path depending on whether it exists.

zmkdir <path> - Will create a new directory using path. Parent directory must exist for this to work. Path can be relative or absolute.

zrmdir <path> - Will delete the directory specified in path. Parent directory must exist for this to work, and path can be relative or absolute

zinspect <data structure> <index> - Will print out the contents of the data structure specified at the index specified. Possible data structures include -master, -inode, and -dblock.

ztouch <filename> - Will create a new file in that name

zcreate <filename> - Will create a new file in that name, and also attatch whatever is in STDIN to the file. So the contents of the file now become what was given in STDIN.

zappend <filename> - Will create a new file in that name, and append anything to the after the offset of the file. Newly appended data comes from STDIN and will go straight into the data blocks of the file

zlink <srcfile> <newfile> - Will create the new file and makes its inode the same inode as that of the srcfile. That way the newfile is now the same as the srcfile if one were to inspect it or zmore it

zmore <filename> - Will output the contents of the file in the data blocks located on its inode to STDOUT

zremove <filename> - Will remove the file, and its references. If the file's inode contains an n_reference larger than 1, then the inode and its blocks are not unallocated. Only when the inode's n_references is equal to 1 are the data blocks and the inode unallocated.

# Notes

When creating a new directory, the path to create a new directory has one caveat. If the path is /home/wow/hello, for example, this will create a folder named hello in wow which is already in home. But if the path is /home/hello/hello, it will give some unexpected behaviors. The main takeaway from here is to not name a new directory under a parent directory with the same name. This will cause some errors.
