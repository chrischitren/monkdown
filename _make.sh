gcc -Wpedantic -Wall -std=c89 -c -g parse.c write.c
gcc -Wpedantic -Wall -std=c89 parse.o write.o -o _run
./_run
find . -name "*.o" -delete
