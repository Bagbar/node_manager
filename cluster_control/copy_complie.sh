#!/bin/sh

NEWDIR=$'cluster'
mkdir "$HOME/$NEWDIR" -p
rsync -r *.c "$HOME/$NEWDIR"
rsync -r *.h "$HOME/$NEWDIR"
cd "$HOME/$NEWDIR"
gcc -std=c99 -O2 -c -o "basics.o" "basics.c"
gcc -std=c99 -O2 -c -o "main.o" "main.c"
gcc -std=c99 -O2 -c -o "master.o" "master.c"
gcc -std=c99 -O2 -c -o "slave.o" "slave.c"

gcc  -o "slave_background_process"  ./basics.o ./main.o ./master.o ./slave.o   -lpthread

cd ~/git/node_manager/slave_background_process