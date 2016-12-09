#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

asmlinkage extern long (*sysptr)(void *arg);

/*struct for input data*/
struct inputargs {
  char *opfile;
  char *ip1;
  char *ip2;
  u_int flags;
  u_int *data;
};

int getNewLineFromBuffer (char *buff, unsigned int startOffset, int buffSize);
void readFlags (u_int flagVal, bool * flags);
void makeFileEmpty(struct file* op);
asmlinkage long xmergesort (void *arg)
{
  /* input data read */
  struct inputargs *args = (struct inputargs *) arg;
  /* size of page */
  size_t PAGE_CACHE_SIZE;
  /* size of input arguments */
  size_t sizeofstruct;
  /* size of last record written to file */
  size_t sizeoflaststr;
  /* holds input file1, input file2 and output file buffers */
  char *buf1;
  char *buf2;
  char *bufop;
  /* holds buffer sizes of input file1, input file2 and output file */
  int bytes1, bytes2, bytesop;
  unsigned long byt;
  mm_segment_t oldfs;
  /* holds error number generated in code */
  long errNo;
  /* file names of input file1, input file2 and output file */
  char *file1name;
  char *file2name;
  char *fileopname;
  /* pointers to input file1, input file2 and output file */
  struct file *ip1;
  struct file *ip2;
  struct file *op;
  /* size of filename */
  int filenamesize;
  /* size of input file1 and  input file2 */
  int f1size, f2size;
  /* fetch data flags for input file1 and input file2 */
  bool l1, l2;
  /* value of flags */
  bool flags[5] = { true };
  /* offset values for input file1, input file2 and output file */
  int f1stof, f1edof, f2stof, f2edof, opstof;
  /* current lines from input file1 and input file2 */
  char *str1;
  char *str2;
  /* last record wriiten to file*/
  char *laststr;
  /* number of records written to output file */
  int *datapointer;
  int it,data;
  int compare;
  /* system file numbers of input file1, input file2 and output file */
  long int file1sysno,file2sysno,fileopsysno;
  /* check if new record written to file */
  bool write;
  /* offset for reading from input file */
  loff_t newoff;

  buf1 = NULL;
  buf2 = NULL;
  bufop = NULL;
  ip1 = NULL;
  ip2 = NULL;
  op = NULL;
  file1name = NULL;
  file2name = NULL;
  fileopname = NULL;
  str1 = NULL;
  str2 = NULL;
  laststr = NULL;
  newoff = 0;
  f1stof = 0;
  f1edof = 0;
  f2stof = 0;
  f2edof = 0;
  opstof = 0;
  file1sysno = 0;
  file2sysno = 0;
  fileopsysno = 0;
  PAGE_CACHE_SIZE = PAGE_SIZE;
  sizeoflaststr = 0;
  sizeofstruct = sizeof (args);
  bytes1 = 0;
  bytes2 = 0;
  bytesop = 0;
  errNo = 0;
  data = 0;
  datapointer = NULL;
  it = 0;
  compare = 0;
  write = false;

  oldfs = get_fs ();
  datapointer = kmalloc(sizeof(int), GFP_KERNEL);

  /* validate inputs */
  /* validate file name sent from user */
  filenamesize = strlen_user (args->ip1);
  file1name = kmalloc (filenamesize, GFP_KERNEL);
  if (file1name == NULL) {
    errNo = -ENOMEM;
    goto out;
  }
  byt = strncpy_from_user(file1name, args->ip1, filenamesize);
  /* open file */
  ip1 = filp_open (file1name, O_RDONLY, 0);
  if (!ip1 || IS_ERR (ip1)) {
    errNo = PTR_ERR(ip1);
    goto out;
  }
  if (file1name != NULL) {
    memset (file1name, 0, filenamesize);
    kfree (file1name);
    file1name = NULL;
  }


  filenamesize = strlen_user (args->ip2);
  file2name = kmalloc (filenamesize, GFP_KERNEL);
  if (file2name == NULL) {
    errNo = -ENOMEM;
    goto out;
  }
  byt = strncpy_from_user(file2name, args->ip2, filenamesize);
  ip2 = filp_open (file2name, O_RDONLY, 0);
  if (!ip2 || IS_ERR (ip2)) {
    errNo = PTR_ERR(ip2);
    printk ("\n------------errorin 2!");
    goto out;
  }
  if (file2name != NULL) {
    memset (file2name, 0, filenamesize);
    kfree (file2name);
    file2name = NULL;
  }


  filenamesize = strlen_user (args->opfile);
  fileopname = kmalloc (filenamesize, GFP_KERNEL);
  if (fileopname == NULL) {
    errNo = -ENOMEM;
    goto out;
  }
  byt = strncpy_from_user(fileopname, args->opfile, filenamesize);
  /* create output file if does not exists */
  op =
  filp_open (fileopname, O_WRONLY | O_CREAT | O_TRUNC, ip1->f_inode->i_mode);
  if (!op || IS_ERR (op)) {
    errNo = PTR_ERR(op);
    printk ("\n=-------found this error in op file %ld", PTR_ERR (op));
    goto out;
  }
  if (fileopname != NULL) {
    memset (fileopname, 0, filenamesize);
    kfree (fileopname);
    fileopname = NULL;
  }

  /* read flags */
  readFlags(args->flags, flags);

  /* check for duplicate files */
  file1sysno  = ip1->f_inode->i_sb->s_magic;
  file2sysno  = ip2->f_inode->i_sb->s_magic;
  fileopsysno  = op->f_inode->i_sb->s_magic;

  if (file1sysno == file2sysno) {
   if (ip1->f_inode->i_ino == ip2->f_inode->i_ino) {
     errNo = -EINVAL;
     goto out;
   }
 }

 if (file1sysno == fileopsysno) {
  if (ip1->f_inode->i_ino == op->f_inode->i_ino) {
    errNo = -EINVAL;
    goto out;
  }
}

if (file2sysno == fileopsysno) {
  if (ip2->f_inode->i_ino == op->f_inode->i_ino) {
    errNo = -EINVAL;
    goto out;
  }
}

/* allocate buffers */
buf1  = kmalloc(PAGE_CACHE_SIZE * sizeof(char), GFP_KERNEL);
buf2  = kmalloc(PAGE_CACHE_SIZE * sizeof(char), GFP_KERNEL);
bufop  = kmalloc(PAGE_CACHE_SIZE * sizeof(char), GFP_KERNEL);


/* initialize offset value for reading */
ip1->f_pos = f1stof;
ip2->f_pos = f2stof;
op->f_pos = opstof;

set_fs(KERNEL_DS);

/* read data into buffers */
bytes1 = vfs_read(ip1,buf1,PAGE_CACHE_SIZE,&ip1->f_pos);
bytes2 = vfs_read(ip2,buf2,PAGE_CACHE_SIZE,&ip2->f_pos);

l1 = false;
l2 = false;
f1size = bytes1; 
f2size = bytes2;

/* loop until both file are not completely read */
while (f1size > 0 ||  f2size > 0) {
  /* read next data from input file1 */
  if (!l1) {
    /* get offset for new line found */
    if (f1size > 0)
      f1edof = getNewLineFromBuffer(buf1, f1stof, bytes1);
    /* read new record from input file if buffer size exhausted */
    if (f1edof == -1) {

      memset(buf1,0,PAGE_CACHE_SIZE * sizeof(char));
      newoff = (PAGE_CACHE_SIZE - f1stof);
      ip1->f_pos = ip1->f_pos - newoff;
      f1size = vfs_read(ip1, buf1, PAGE_CACHE_SIZE, &ip1->f_pos );
      if ( f1size == (PAGE_CACHE_SIZE - f1stof) ) {
        f1edof = f1size;
      } else {
        f1edof = getNewLineFromBuffer(buf1, 0, f1size);
      }
      f1stof = 0;

    }
    str1 = kmalloc(f1edof * sizeof(char), GFP_KERNEL);

    strncpy(str1, buf1 + f1stof, f1edof);
    l1 = true;
  }
  /* read next data from input file2*/
  if (!l2) {
    if (f2size > 0)
      f2edof = getNewLineFromBuffer(buf2, f2stof, bytes2);
    if (f2edof == -1) {
      memset(buf2, 0, PAGE_CACHE_SIZE * sizeof(char));

      newoff = (PAGE_CACHE_SIZE - f2stof);
      ip2->f_pos = ip2->f_pos - newoff;
      f2size = vfs_read(ip2, buf2, PAGE_CACHE_SIZE, &ip2->f_pos);

      if ( f2size == (PAGE_CACHE_SIZE - f2stof) ) {
        f2edof = f2size;
      } else {
        f2edof = getNewLineFromBuffer(buf2, 0, f2size);
      }
      f2stof = 0;

    }
    str2 = kmalloc(f2edof * sizeof(char), GFP_KERNEL);

    strncpy(str2, buf2+f2stof, f2edof);
    l2 = true;
  }
  it = 0;
  if (f1size > 0 && f2size > 0) {
   if (!flags[2]) {
    it = strcmp(str1, str2);
  } else {
    it = strcasecmp(str1, str2);
  }

}
/* if file1 empty, write line from file 2 */
else if (f1size < 1) {
  it = 1;
}
/* if file2 empty, write record from file 1 */
else if (f2size < 1) {
  it = -1;
}

/* str1 is smaller than str2 */
if (it <  0) {

  /* if output buffer is full, write to ouptput file */
  if (opstof + f1edof >=  PAGE_CACHE_SIZE) {
    bytesop = vfs_write(op, bufop, opstof,&op->f_pos);
    memset(bufop, 0, PAGE_CACHE_SIZE * sizeof(char));
    opstof = 0;
  }

  compare = 0;
  if (laststr && sizeoflaststr > 0) {
    if (flags[2]) {
      compare = strcasecmp(laststr, str1);
    } else {
      compare = strcmp(laststr, str1);
    }
  }

  /* check for different flag cases */
  if ( sizeoflaststr > 0  && flags[0] && !flags[1]) {

    if (compare != 0) {

      if (flags[3] && compare > 0) {
        errNo = -EINVAL;
        goto out;
      }
      memcpy(bufop + opstof, str1, f1edof);
      data++;
      opstof = opstof + f1edof;
    }
  } else {
    /* check flag cases with last record written to file */
    if ( sizeoflaststr > 0 ) {
      if (flags[3] && compare > 0) {
        errNo = -EINVAL;
        goto out;
      }
    }


    memcpy(bufop + opstof, str1, f1edof);
    data++;
    opstof = opstof + f1edof;

  }
  if (laststr && sizeoflaststr > 0) {
    memset(laststr, 0, sizeoflaststr * sizeof(char));
    kfree(laststr);
    laststr = NULL;
  }
  laststr = kmalloc(sizeof(char) * f1edof,GFP_KERNEL);

  strncpy(laststr,str1,f1edof);

  sizeoflaststr  = f1edof;
  f1stof = f1stof + f1edof;
  f1size = f1size - f1edof;

  l1 = false;
  memset(str1,0,f1edof * sizeof(char));
  kfree(str1);
  str1 = NULL;

}
/* str2 smaller that str1 */
else if (it > 0) {
  if (opstof + f2edof >=  PAGE_CACHE_SIZE) {
    bytesop = vfs_write(op, bufop, opstof, &op->f_pos);
    memset(bufop, 0, PAGE_CACHE_SIZE * sizeof(char));
    opstof = 0;
  }

  compare = 0;
  if (laststr && sizeoflaststr > 0) {
    if (flags[2])
      compare = strcasecmp(laststr, str2);
    else
      compare = strcmp(laststr, str2);
  }
  if ( sizeoflaststr > 0  && flags[0] && !flags[1]) {

    if (compare != 0) {
      if (flags[3] && compare > 0) {
        errNo = -EINVAL;
        goto out;
      }
      memcpy(bufop + opstof, str2, f2edof);
      data++;
      opstof = opstof + f2edof;
    }
  } else {
    if ( sizeoflaststr > 0 ) {
      if (flags[3] && compare > 0) {
        errNo = -EINVAL;
        goto out;
      }
    }


    memcpy(bufop + opstof, str2, f2edof);
    data++;
    opstof = opstof + f2edof;

  }


  if (laststr && sizeoflaststr > 0) {
    memset(laststr, 0, sizeoflaststr * sizeof(char));
    kfree(laststr);
    laststr = NULL;
  }
  laststr = kmalloc(sizeof(char) * f2edof, GFP_KERNEL);
  strncpy(laststr, str2, f2edof);
  sizeoflaststr  = f2edof;


  f2stof = f2stof + f2edof;
  f2size = f2size - f2edof;

  l2 = false;
  memset(str2, 0, f2edof * sizeof(char));
  kfree(str2);
  str2 = NULL;

}
/* str1 and str2 are equal */
else {

  if (opstof + f2edof + f1edof >=  PAGE_CACHE_SIZE || opstof + f2edof >= PAGE_CACHE_SIZE ) {
    bytesop = vfs_write(op, bufop, opstof, &op->f_pos);
    memset(bufop, 0, PAGE_CACHE_SIZE * sizeof(char));
    opstof = 0;
  }

  write = false;
  compare = 0;
  if (laststr && sizeoflaststr > 0) {
    if (flags[2]) {
      compare = strcasecmp(laststr, str1);
    } else {
      compare = strcmp(laststr, str1);
    }
  }

  if ( sizeoflaststr > 0  && flags[0] && !flags[1]) {

    if (compare != 0) {
      if (flags[3] && compare >  0) {
        errNo = -EINVAL;
        goto out;
      }
      memcpy(bufop + opstof, str1, f1edof);
      data++;
      opstof = opstof + f1edof;
      write = true;
    }
  } else {
    if ( sizeoflaststr > 0 ) {
      if (flags[3] && compare > 0) {
        errNo = -EINVAL;
        goto out;
      }
    }

    memcpy(bufop + opstof, str1, f1edof);
    data++;
    opstof = opstof + f1edof;
    write = true;

  }

  if (laststr && sizeoflaststr > 0) {
    memset(laststr,0,sizeoflaststr * sizeof(char));
    kfree(laststr);
    laststr = NULL;
  }
  laststr = kmalloc(sizeof(char) * f1edof, GFP_KERNEL);
  strncpy(laststr, str1, f1edof);
  sizeoflaststr  = f1edof;
  f1stof = f1stof + f1edof;
  f1size = f1size - f1edof;
  l1 = false;
  memset(str1, 0, f1edof * sizeof(char));
  kfree(str1);
  str1 = NULL;

  if ((!flags[0] || flags[1]) && write) {
    memcpy(bufop + opstof, str2, f2edof);
    data++;
    opstof = opstof + f2edof;
  }
  f2stof = f2stof + f2edof;
  f2size = f2size - f2edof;
  l2 = false;
  memset(str2, 0, f2edof * sizeof(char));
  kfree(str2);
  str2 = NULL;
}

}

/* write data to output file */
bytesop = vfs_write(op,bufop, opstof, &op->f_pos);
/* copy number of sorted records to user space */
*datapointer = data;
byt = copy_to_user((void *) args->data, (void *) datapointer, sizeof(data));
/* Clean up code */
out:
set_fs(oldfs);

/* If error found, clean up output file */
if (errNo < 0 && !IS_ERR(op))
 makeFileEmpty(op);

if (!IS_ERR(ip1))
  filp_close (ip1, NULL);
if (!IS_ERR(ip2))
  filp_close (ip2, NULL);
if (!IS_ERR(op))
  filp_close (op, NULL);

if (file1name) {
  memset (file1name, 0, filenamesize);
  kfree (file1name);
  file1name = NULL;
}
if (file2name) {
  memset (file2name, 0, filenamesize);
  kfree (file2name);
  file2name = NULL;
}
if (fileopname) {
  memset (fileopname, 0, filenamesize);
  kfree (fileopname);
  fileopname = NULL;
}


if (str1) {
  memset (str1, 0, f1edof * sizeof (char));
  kfree (str1);
  str1 = NULL;
}

if (str2) {
  memset (str2, 0, f2edof * sizeof (char));
  kfree (str2);
  str2 = NULL;
}

if (laststr && sizeoflaststr > 0) {
  memset (laststr, 0, sizeoflaststr);
  kfree (laststr);
  laststr = NULL;
}
if (buf1) {
  memset (buf1, 0, PAGE_CACHE_SIZE * sizeof (char));
  kfree (buf1);
  buf1 = NULL;
}
if (buf2) {
  memset (buf2, 0, PAGE_CACHE_SIZE * sizeof (char));
  kfree (buf2);
  buf2 = NULL;
}
if (bufop) {
  memset (bufop, 0, PAGE_CACHE_SIZE * sizeof (char));
  kfree (bufop);
  bufop = NULL;
}
memset (flags, 0, 5 * sizeof (bool));

*flags = NULL;

memset (args, 0, sizeofstruct);

args = NULL;

return errNo;
}

/* Returns the offset when new line encountered */
int getNewLineFromBuffer(char *buff, unsigned int startOffset, int buffSize)
{
  int endOffset;
  endOffset = 1;

  while (*(buff + startOffset) != '\n' && endOffset < buffSize) {
    startOffset++;
    endOffset++;
  }
  if (*(buff + startOffset) == '\n') {
    return endOffset;
  } else {
    return -1;
  }
}

/* Fills array with flag values */
void readFlags(u_int flagVal, bool* flags)
{
    /* u : 0x01  a:0x02  i:0x04  t:0x10  d =0x20 */
  *flags = flagVal & 0x01;
  *(flags + 1) = flagVal & 0x02;
  *(flags + 2) = flagVal & 0x04;
  *(flags + 3) = flagVal & 0x10;
  *(flags + 4) = flagVal & 0x20;

}

/* deletes file */
void
makeFileEmpty(struct file* op)
{
  struct inode *dirinode;
  struct dentry *dirdentry;
  struct dentry *filedentry;
  int err;

  dirinode = NULL;
  dirdentry = NULL;
  filedentry = NULL;
  err = 0;

  filedentry = op->f_path.dentry;
  dirinode = filedentry->d_parent->d_inode;
  dirdentry = dget_parent(filedentry);

  /* get lock on file */
  mutex_lock_nested(&(dirdentry->d_inode->i_mutex),I_MUTEX_PARENT);
  err = vfs_unlink(dirinode,filedentry,NULL);

  /* release lock */
  mutex_unlock(&(dirdentry->d_inode->i_mutex));
  dput(dirdentry);
  dput(filedentry);

}
static int __init init_sys_xmergesort(void)
{
  printk("installed new sys_xmergesort module\n");
  if (sysptr == NULL)
    sysptr = xmergesort;
  return 0;
}
static void  __exit exit_sys_xmergesort(void)
{
  if (sysptr != NULL)
    sysptr = NULL;
  printk("removed sys_xmergesort module\n");
}
module_init(init_sys_xmergesort);
module_exit(exit_sys_xmergesort);
MODULE_LICENSE("GPL");
