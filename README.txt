The program will need to be compiled using two different commands depending on if PROCESS or THREAD macro is being tested. 

For the PROCESS version use the following compile commands:

gcc -Wall -g3 -O0 -std=c11 -pthread -D_POSIX_SOURCE -D_GNU_SOURCE -D_POSIX_C_SOURCE -DPROCESS -c -o msort.o uniqify.c
gcc -Wall -g3 -O0 -std=c11 -pthread -D_POSIX_SOURCE -D_GNU_SOURCE -D_POSIX_C_SOURCE -DPROCESS -o msort msort.o


For the THREAD version, use the following compile commands:

gcc -Wall -g3 -O0 -std=c11 -pthread -D_POSIX_SOURCE -D_GNU_SOURCE -D_POSIX_C_SOURCE -DTHREAD -c -o msort.o uniqify.c
gcc -Wall -g3 -O0 -std=c11 -pthread -D_POSIX_SOURCE -D_GNU_SOURCE -D_POSIX_C_SOURCE -DTHREAD -o msort msort.o

An example run command of the msort program would look like the following:
./msort 4 < input.txt



PDF files detailing the comparisons between thread-based vs process-based mergesort implementation are included for your viewing.



**Results**
Of the two versions the process version of the program generally took less time to complete vs the threads version. These results varied
a bit as the input.txt file size became larger. As the input file size became larger, we started to see that threads would run slightly faster 
at certain points. This was a bit surprising since threads are supposed to be a more efficient solution for less intensive tasks, while processes
are usually faster for more intensive tasks. Regardless, the final results of the programs are that generally, the process version was faster, but 
at times, especially with larger file size input, threads could execute the program faster. Another trend that could be seen was as the amount of 
k sort processes was increased, regardless of file input size, the program took less time to complete, as there were now a greater amount of resources
available to compute the results. 