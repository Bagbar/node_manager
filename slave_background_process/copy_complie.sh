#!/bin/sh
rsync -r *.c /home/xubuntu/testdir/
rsync -r *.h /home/xubuntu/testdir/
cd /home/xubuntu/testdir/bin
gcc -std=c99 -O0 -g3 -pedantic -c -fmessage-length=0 -MMD -MP -MF"basics.d" -MT"basics.d" -o "basics.o" "../basics.c"
gcc -std=c99 -O0 -g3 -pedantic -c -fmessage-length=0 -MMD -MP -MF"main.d" -MT"main.d" -o "main.o" "../main.c"
gcc -std=c99 -O0 -g3 -pedantic -c -fmessage-length=0 -MMD -MP -MF"master.d" -MT"master.d" -o "master.o" "../master.c"
gcc -std=c99 -O0 -g3 -pedantic -c -fmessage-length=0 -MMD -MP -MF"slave.d" -MT"slave.d" -o "slave.o" "../slave.c"

gcc  -o "slave_background_process"  ./basics.o ./main.o ./master.o ./slave.o   -lpthread

cd ~/git/node_manager/slave_background_process