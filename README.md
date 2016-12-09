
MergeSort System call

Rashmi Narvekar

Command to run system call:
./xhw1 [-uaitd]  outfile.txt inputfile1.txt inputfile2.txt


Flags Implemented:
-u 0x01: output sorted records; if duplicates found, output only one copy
-a 0x02: output all records, even if there are duplicates
-i 0x04: compare records case-insensitive (case sensitive by default)
-t 0x10: if any input file is found NOT to be sorted, stop and return an
	 error (EINVAL); otherwise continue to output records ONLY if any
	 are found that are in ascending order to what you've found so far.
-d 0x20: return the number of sorted records written out in "data"


Design considerations:
1. Each input file is read into a buffer of size equal to 4kb.
2. An output buffer of same size is maintained.
3. Entry from each input buffer is read until new line character '\n' is found.
4. Entries from both the files are compared and the record which is smaller is written to the output file.
5. A laststring buffer is maintained to store the previously written record to the output buffer.
6. Every new entry is compared with the laststring written to buffer. This check os done implement the 'a' and 't' flag.
7. If the current entry to be written is smaller than the laststring, then the current entry is ignored or error is thrown, depending on the value of flag 't', whether is set or not.
8. If the current entry to be written is equal to the laststring, then the current entry is ignored, depending on the value of flag, whether 'u' is set or not.
9. When the size of output buffer reaches capacity, the buffer contents are written to the output file.
11. The output file is created with permissions no greater than the permissions of the first input file.
12. Also when the last records of the input buffers are read, the buffers are loaded with next records from the input files.
13. A check is put to find if the current record being fetched from the input buffer is incomplete. In this case the next records for that particular input file are fetched and then the normal flow of the code is resumed.
14. If error is encountere mid way in the code, then the output file is deleted.
15. Validations are done for all input arguments on both user and kernel level.
