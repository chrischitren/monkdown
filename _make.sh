gcc -Wpedantic -Wall -std=c89 -c -g parse.c write.c pages.c
gcc -Wpedantic -Wall -std=c89 parse.o write.o pages.o -o _run
./_run
find . -name "*.o" -delete
