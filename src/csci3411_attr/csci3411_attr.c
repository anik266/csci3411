/**
 * CSCI 3411 Operating Systems
 * Project 2: Filesystem Attributes
 * Abdul-Rahim, Muhammad	mabdulra@gwmail.gwu.edu
 * Bernier, Brandon			bbernier@gwmail.gwu.edu
 * Malyszek, Filip			malyszek@gwmail.gwu.edu
 *
 * This .c file contains all of the necessary additional
 * system calls to implement Filesystem Attributes in the
 * Linux kernel. They allow for setting, getting, and
 * removing attributes from files or directories. 
 */

#include <linux/linkage.h>
#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <asm-i386/stat.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/dirent.h>   /* library for traversing the directory */
#include <linux/stat.h>     /* stat() allows us to tell if a file exists */

/* macro definitions */
#define CSCI3411_FILE 0
#define CSCI3411_DIR  1

struct linux_dirent{
	long 		d_ino;
	off_t 		d_off;
	unsigned short 	d_reclen;
	char		d_name[];
};

typedef unsigned char bool_t;

/**
 * Set attribute <attrname> with a value <attrvalue> of length <size> to a particular file called <file>
 *
 * First, check if <file> exists. If no, return a negative value and exit function.
 * Check if folder .<file>_attr exists. If no, make the directory.
 * If directory did exist, check if file <attrname> exists. If no, make file.
 * Flush contents of <attr> and replace with <attrvalue>
 * If directory was made through this function call, no need to check if <attr>
 * exists because it is guaranteed that it does not.
 *
 * Return 0 on success and a negative value on failure.
 */

asmlinkage long sys_csci3411_set_attr ( char *filename, char *attrname, char *attrvalue, int size )
{
    struct stat sb	;	/* necessary structure for using stat() */
    char *fstring	;	/* file/directory to give attribute to	*/
    char *dstring	;	/* temporary string for strstr()        */
    char buf[128]	;	/* copy of the original filename        */
    char loc[128]	;	/* full filepath for attr folder        */
    char cmd[128]	;	/* command for system calls             */
    bool_t filetype	;	/* is the tag for a file or dir?        */
	int fd          ;	/* file descriptor for attribute file   */        
    mm_segment_t fs	;	/* FS segment used to make sys calls    */
        
    fs = get_fs();
    set_fs(get_ds());
    
    /* check if filename is an existing file or directory */
    if( sys_newstat(filename,&sb)==0 && S_ISREG(sb.st_mode) )
        filetype = CSCI3411_FILE;
    else if( sys_newstat(filename,&sb)==0 && S_ISDIR(sb.st_mode) )
        filetype = CSCI3411_DIR;
    else
        return -1;	/* file/directory does not exist */
    
    /* split filename strings into containing folder and file */
    fstring  = strrchr(filename, '/');
    fstring += sizeof(char);
    copy_from_user( buf, filename, sizeof(buf)/sizeof(char));
    dstring = strstr( buf, fstring );
    strncpy( dstring,"\0",1);
    sprintf(loc,"%s.%s_attr",buf,fstring);
    
    /* check if attributes directory exists, mkdir if it doesn't */
    if( sys_newstat(loc,&sb)==0 && S_ISDIR(sb.st_mode) );
    else
        sys_mkdir(loc,sb.st_mode);
    
    /* create file in attribute directory
     * in the event of a duplicate attribute file,
     * overwrite with newer information */
    sprintf(cmd,"%s/%s",loc,attrname);
    sys_unlink(cmd);                //remove file if it exists so we can make a new one
    sys_mknod(cmd,sb.st_mode,0);
    fd = sys_open(cmd,O_CREAT|O_WRONLY,0);
    if(fd<0)
        printk("file creation failed\n");
    else
        sys_write(fd,attrvalue,size);
    
    set_fs(fs);
    return 0;
}


/**
 * Gets the value of <attrname> from a particular file or directory <filename>
 *
 * First, check if <filename> exists. If not, return negative error value and exit.
 * Check if folder .<file>_attr exists. If not, return negative error value and exit.
 * Check if file <attrname> exists. If not, return negative error value and exit. 
 * Writes attribute value of <attrname> to buf.
 *
 * Returns number of bytes copied on success and a negative value on failure.
 */

asmlinkage long sys_csci3411_get_attr(char *filename, char *attrname, char *buf, int bufsize){
    struct stat sb  ;   /* necessary structure for using sys_newstat() */
	char *fstring   ;   /* file/directory to get attribute from */
	char *dstring   ;   /* temporary string for strstr()        */
	char loc[128]   ;   /* full filepath for attr folder        */
	char buff[128]  ;   /* temporary buffer for filename        */
	char cmd[128]   ;   /* buffer holds full path to attrname   */
    char attrvalue[128];/* Temporary buffer for attribute value */
    int attr_fd     ;   /* The file we are reading from         */
    bool_t filetype	;	/* is the tag for a file or dir?        */
    mm_segment_t fs	;	/* FS segment used to make sys calls    */
    
    fs = get_fs();
    set_fs(get_ds());
    
	/* check if filename is an existing file or directory */
	if( sys_newstat(filename,&sb)==0 && S_ISREG(sb.st_mode) )
		filetype = CSCI3411_FILE;
	else if( sys_newstat(filename,&sb)==0 && S_ISDIR(sb.st_mode) )
		filetype = CSCI3411_DIR;
	else
		return -1;	/* file/directory does not exist */
    
	/* split filename strings into containing folder and file */
	fstring  = strrchr(filename, '/');
	fstring += sizeof(char);
	copy_from_user( buff, filename, sizeof(buff)/sizeof(char));
	dstring = strstr( buff, fstring );
	strncpy( dstring,"\0",1);
	sprintf(loc,"%s.%s_attr",buff,fstring);
	
	// Check if the directory is valid
	if(!(sys_newstat(loc,&sb)==0 && S_ISDIR(sb.st_mode)))
		return -1;
	
	// Open the file and read its attrvalue if it exists
    sprintf(cmd,"%s/%s",loc,attrname);
	if(!(sys_newstat(cmd,&sb)==0 && S_ISREG(sb.st_mode)))
		return -1;
	attr_fd = sys_open(cmd, O_RDWR, 0);
	sys_read(attr_fd, attrvalue, sb.st_size);
	sys_close(attr_fd);
	
    // Copy the attrvalue to buf
	copy_to_user(buf, attrvalue, sb.st_size);

    set_fs(fs);
    // Return the size of the attribute value
	return(sb.st_size);
}


/**
 * Get all attribute names from a particular file or directory called <filename>
 * 
 * First, check if <filename> exists. If not, return negative error value and exit.
 * Check if folder .<file>_attr exists. If not, return negative error value and exit.
 * If directory exists, enter it and get names of all files existing there.
 * Concatenates names of files in .file_attr directory onto each other separated 
 * by a ":" and stores this in buf.
 *
 * Returns number of bytes copied on success and a negative value on failure.
 */

asmlinkage long sys_csci3411_get_attr_names(char *filename, char *buf, int bufsize){
    struct stat sb  ;   /* Necessary structure for using sys_newstat()  */
	char *fstring   ;   /* File/directory to get attribute names from   */
	char *dstring   ;   /* temporary string for strstr()                */
	char loc[128]   ;   /* Full filepath for attr folder                */
	char buff[128]  ;   /* copy of the original filename                */
	char buf2[1024] ;   /* buffer used for the sys_getdents() call      */
    char total_attr[1024] = "\0"    ;   /* Contains all the attribute names            */
    struct linux_dirent *dent=NULL  ;   /* Necessary for looking through the directory */
	int dir,i,nread;;   /* necessary for opening and looking into directory            */
    bool_t filetype	;	/* is the tag for a file or dir?        */
    mm_segment_t fs	;	/* FS segment used to make sys calls    */
    
    fs = get_fs();
    set_fs(get_ds());
    
	/* Check if filename is an existing file or directory */
	if( sys_newstat(filename,&sb)==0 && S_ISREG(sb.st_mode) )
		filetype = CSCI3411_FILE;
	else if( sys_newstat(filename,&sb)==0 && S_ISDIR(sb.st_mode) )
		filetype = CSCI3411_DIR;
	else
		return -1;	/* file/directory does not exist */
    
	/* Split filename strings into containing folder and file */
	fstring  = strrchr(filename, '/');
	fstring += sizeof(char);
	copy_from_user( buff, filename, sizeof(buff)/sizeof(char));  
	dstring = strstr( buff, fstring );
	strncpy( dstring,"\0",1);
	sprintf(loc,"%s.%s_attr",buff,fstring);
	
	// Check if the directory is valid
	if(!(sys_newstat(loc,&sb)==0 && S_ISDIR(sb.st_mode)))
		return -1;

	// Go inside the directory
    dir = sys_open(loc,O_RDONLY,0);
    if (dir == -1) return -1;
    for( ; ; )
    {
        nread = sys_getdents(dir,buf2,1024);
        if(nread==-1) return -1;
        if(nread==0) break;
        
        // Read each file in the directory
        for(i = 0; i<nread;)
        {
            dent = (struct linux_dirent *)(buf2+i);		
            // The first two entries are "." and "..", skip those
            if(strcmp((char *)(dent->d_name),".") && strcmp((char *)(dent->d_name),".."))
            {
                    // Put all the file names in total_attr
                    strcat(total_attr, (char *)(dent->d_name));
                    strcat(total_attr, ":");
            }
            i+= dent->d_reclen;
        }
    }
    sys_close(dir);
    total_attr[strlen(total_attr)-1] = '\0';	//	remove last ":"
	
	// Copy all the names into buf
	copy_to_user(buf, total_attr, bufsize);
    
    set_fs(fs);
    
    // Return the number of bytes copied
	return(strlen(total_attr));
}


/**
 * Remove attribute <attrname> from a particular file or directory called <filename>
 *
 * First, check if <filename> exists. If not, return negative error value and exit.
 * Check if folder .<file>_attr exists. If not, return negative error value and exit.
 * If directory exists, check if file <attrname> exists. If not, return negative
 * error value and exit.
 * If file <attrname> exists, remove it. If .<file>_attr is now empty after remove,
 * also remove the attributes directory.
 *
 *
 * Return 0 on success and a negative value on failure.
 */

asmlinkage long sys_csci3411_remove_attr    ( char *filename, char *attrname )
{
    struct stat sb	;	/* necessary structure for using sys_newstat()  */
	char *fstring	;	/* file/directory to remove attribute for       */
	char *dstring	;	/* temporary string for strstr()        */
	char buf[128]	;	/* copy of the original filename        */
	char loc[128]	;	/* full filepath for attr folder        */
    char loc2[128]	;	/* full filepath for attrname file      */
	bool_t filetype	;	/* tag for file/directory               */
	mm_segment_t fs	;	/* FS segment used to make sys calls    */
    
    fs = get_fs();
    set_fs(get_ds());
    
	/* check if filename is an existing file or directory */
	if( sys_newstat(filename,&sb)==0 && S_ISREG(sb.st_mode) )
		filetype = CSCI3411_FILE;
	else if( sys_newstat(filename,&sb)==0 && S_ISDIR(sb.st_mode) )
		filetype = CSCI3411_DIR;
	else
		return -1;	/* file/directory does not exist */
	
	/* split filename strings into containing folder and file */
	fstring  = strrchr(filename, '/');
	fstring += sizeof(char);
	copy_from_user( buf, filename, sizeof(buf)/sizeof(char));
	dstring = strstr( buf, fstring );
	strncpy( dstring,"\0",1);
	sprintf(loc,"%s.%s_attr",buf,fstring);
	
	/* check if attributes directory exists, return error and exit if not */
	if( sys_newstat(loc,&sb)==0 && S_ISDIR(sb.st_mode) )
	{
        sprintf(loc2,"%s/%s",loc,attrname);
        if( sys_newstat(loc2,&sb)==0 && S_ISREG(sb.st_mode) )
        {
            if( sys_unlink(loc2) )
                return -1;
            sys_rmdir(loc);
        }
        else
            return -1;	/* file/directory does not exist */
	}
	else
		return -1;	/* file/directory does not exist */
	set_fs(fs);
	return 0;
}


/**
 * Remove all attributes from a particular file or directory called <filename>
 *
 * First, check if <filename> exists. If not, return negative error value and exit.
 * Check if folder .<file>_attr exists. If not, return negative error value and exit.
 * If directory exists, remove all attribute files within it, which will then remove
 * the attribute directory itself and return 0.
 *
 * Return 0 on success and a negative error value on failure.
 */

asmlinkage long sys_csci3411_remove_attr_all	( char *filename )
{
    struct stat sb  ;       /* Necessary structure for using sys_newstat()  */
    char *fstring   ;       /* File/directory to give attribute to          */
	char *dstring   ;       /* temporary string for strstr()                */
	char loc[128]   ;       /* Full filepath for attr folder                */
	char loc2[128]  ;       /* holds filepath for attrname file to remove   */
	char buff[128]  ;       /* holds copy of original filename              */
	char buf2[1024];        /* buffer used for the sys_getdents() call      */
    char total_attr[1024] = "\0"; /* Contains all the attribute names */
    struct linux_dirent *dent=NULL;   /* Necessary for looking through the directory */
	bool_t filetype	;       /* tag for file/directory                       */
    int i,nread,dir ;       /* necessary values for sys_getdents()          */
    char *aPtr      ;       /* holds strsep tokens                          */
	char *nPtr      ;       /* points to colon-seperated list               */
    
    mm_segment_t fs	;       /* FS segment used to make sys calls            */
    	
    fs = get_fs();
    set_fs(get_ds());
    
	/* Check if filename is an existing file or directory */
	if( sys_newstat(filename,&sb)==0 && S_ISREG(sb.st_mode) )
		filetype = CSCI3411_FILE;
	else if( sys_newstat(filename,&sb)==0 && S_ISDIR(sb.st_mode) )
		filetype = CSCI3411_DIR;
	else
		return -1;	/* file/directory does not exist */
    
	/* Split filename strings into containing folder and file */
	fstring  = strrchr(filename, '/');
	fstring += sizeof(char);
	copy_from_user( buff, filename, sizeof(buff)/sizeof(char));
	dstring = strstr( buff, fstring );
	strncpy( dstring,"\0",1);
	sprintf(loc,"%s.%s_attr",buff,fstring);
	
	// Check if the directory is valid
	if(!(sys_newstat(loc,&sb)==0 && S_ISDIR(sb.st_mode)))
		return -1;

	// Go inside the directory
    dir = sys_open(loc,O_RDONLY,0);
    if (dir == -1)  return -1;
    for( ; ; )
    {
        nread = sys_getdents(dir,buf2,1024);
        if(nread==-1) return -1;
        if(nread==0) break;
        
        // Read each file in the directory
            for(i = 0; i<nread;)
            {
                dent = (struct linux_dirent *)(buf2+i);
                // The first two entries are "." and "..", skip those
                if(strcmp((char *)(dent->d_name),".") && strcmp((char *)(dent->d_name),".."))
                {
                    // Put all the file names in total_attr
                    strcat(total_attr, (char *)(dent->d_name));
                    strcat(total_attr, ":");
                }
                i+= dent->d_reclen;
            }
    }
    sys_close(dir);
	total_attr[strlen(total_attr)-1] = '\0';	//	remove last ":"

	printk("%s\n",total_attr);
	nPtr = total_attr;
	do
	{
		aPtr = strsep(&nPtr,":");
		if(aPtr)
        {
			printk("%s\n",aPtr);
			/* remove file */
			memset(loc2,'\0',128);	//	reset the string!
			sprintf(loc2,"%s/%s",loc,aPtr);
		        if( sys_unlink(loc2) )
				return -1;
		        sys_rmdir(loc);
        }
	}while(aPtr);

	set_fs(fs);
	return 0;
}
