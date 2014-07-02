# Extended File System Attributes

Final project for CSCI 3411, Operating and Distributed Systems.  
[Muhammad Abdul-Rahim](mailto:mabdulra@gwmail.gwu.edu)  
[Brandon Bernier](mailto:bbernier@gwmail.gwu.edu)

## Build Instructions

Apply the patch file <CSCI3411_PATCH> to the kernel. Place the file in /root/Desktop to begin. In the terminal, change your 
directory to the one containing your Linux source tree. Then apply the patch with the following command:  

patch -p1 < /root/Desktop/CSCI3411_PATCH  

In the event that this patch does not succeed for whatever reason, we have also included all of the necessary source code files under the <src> directory. This directory shows the location of each of the necessary files and they can simply be copied into these locations to replace the versions of them already in the kernel. The Makefile would replace the kernel Makefile. Then, the <csci3411_attr> directory must be copied into the main kernel directory. <arch/i386/kernel/syscall_table.S>, <include/asm-i386/unistd.h>, <include/linux/syscalls.h>, and <fs/namei.h> are the files that must be inserted. Doing so will add our implemented support for filesystem attributes.  





## Usage of New Functionality

The header is provided in the zip as <csci3411_attr.h>  
Include this header in any code you write to use the following functions:

#### csci3411_set_attr( char *filename, char *attrname, char *attrvalue, int size );
Set the attribute <attrname> with value <attrvalue> of size <size> to file <filename>

#### csci3411_get_attr( char *filename, char *attrname, char *buffer, int size );
Get the attribute value for <attrname> of file <filename> and store it to <buffer> of size <size>

#### csci3411_get_attr_names( char *filename, char *buffer, int size );
Get all attribute names of file <filename> and store it in a colon-separated list to <buffer> of size <size>  
Example output for <buffer>: Type:Creator:Name:Color

#### csci3411_remove_attr( char *filename, char *attrname );
Remove the attribute <attrname> from file <filename>

#### csci3411_remove_attr_all( char *filename );
Remove all attributes from file <filename>




## Additional Information

Renaming a file on the system will also rename its attribute folder if an attribute folder exists for the file. This can be done through the command line and also through the Linux GUI environment. This also works when a file is moved to another directory (the attributes directory will move with it).