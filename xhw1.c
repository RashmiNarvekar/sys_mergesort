#include <asm/unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef __NR_xmergesort
#error xmergesort system call not defined
#endif

struct inputargs
{
  char *opfile;
  char *ip1;
  char *ip2;
  u_int flags;
  u_int *data;
};

int
main (int argc, char *argv[])
{
  int rc;
  struct inputargs args;
  int c;

  rc = 0;
  args.data = malloc(sizeof(int));

  /* check for number of arguments*/
  if (argc > 5) {
    printf("\nToo many arguments.");
    printf("\nExpected input -> [-uaitd] Outputfile.txt Inputfile1.txt Inputfile2.txt\n");
    rc = -1;
    goto out;
  }
  if (argc < 5) {
   printf("\nToo few arguments.");
   printf("\nExpected input -> [-uaitd] Outputfile.txt Inputfile1.txt Inputfile2.txt\n");
   rc = -1;
   goto out;
 }

 /*check if input files are null */
 if (argv[2] == NULL) {
  printf("\nOutput file cannot be null\n");
  rc = -1;
  goto out;
}

if (argv[3] == NULL) {
  printf("\nInput file 1 cannot be null\n");
  rc = -1;
  goto out;
}

if (argv[4] == NULL) {
  printf("\nInput file 2 cannot be null\n");
  rc = -1;
  goto out;
}

/* check if output and input files are same*/
if (strcmp(argv[3], argv[4]) == 0) {
  printf("\nBoth input files are same. Duplicate files not allowed\n");
  rc = -1;
  goto out;
}

if (strcmp(argv[3], argv[2]) == 0) {
  printf("\nOutput file and  input file 1 are same. Duplicate files not allowed\n");
  rc = -1;
  goto out;
}

if (strcmp(argv[2], argv[4]) == 0) {
  printf("\nOutput file and input file 2 are same. Duplicate files not allowed\n");
  rc = -1;
  goto out;
}

/* save input arguments */
args.opfile = (char *) argv[2];
args.ip1 = (char *) argv[3];
args.ip2 = (char *) argv[4];
args.flags = 0;


/* validation of flags */
while ((c = getopt (argc, argv, "uaitd")) != -1)
  switch (c)
{
  case 'u':
  args.flags = args.flags ^ 0x01;
  break;
  case 'a':
  args.flags = args.flags ^ 0x02;
  break;
  case 'i':
  args.flags = args.flags ^ 0x04;
  break;
  case 't':
  args.flags = args.flags ^ 0x10;
  break;
  case 'd':
  args.flags = args.flags ^ 0x20;
  break;
  case '?':
  printf("Valid flags are [uaitd]\n");
  rc = -1;
  goto out;
  default:
  break;
}

if ((args.flags & 0x01) && (args.flags & 0x02)) {
  printf("\nFlag options 'u' and 'a' cannot be set together\n");
  rc = -1;
  goto out;
}

/* system call to merge sort */
rc = syscall (__NR_xmergesort, (void *) &args);

if (args.flags & 0x20)
  printf("\nNumber of sorted records %d\n", *args.data);
if (rc > -1)
  printf ("\nsyscall returned %d\n", rc);
else
  printf ("\nsyscall returned %d (errno=%d)\n", rc, errno);

out:
free(args.data);

exit (rc);
}
