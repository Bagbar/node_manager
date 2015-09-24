#!/bin/sh

NEWDIR=$'cluster'
mkdir "$HOME/$NEWDIR" -p
rsync -r *.c "$HOME/$NEWDIR"
rsync -r *.h "$HOME/$NEWDIR"
cd "$HOME/$NEWDIR"
gcc -std=c99 -I/usr/include/libxml2 -O2 -c -o "basics.o" "basics.c"
gcc -std=c99 -I/usr/include/libxml2 -O2 -c -o "main.o" "main.c"
gcc -std=c99 -I/usr/include/libxml2 -O2 -c -o "master.o" "master.c"
gcc -std=c99 -I/usr/include/libxml2 -O2 -c -o "slave.o" "slave.c"
gcc -std=c99 -I/usr/include/libxml2 -O2 -c -o "XML.o" "XML.c"

gcc  -o "cluster_control"  ./basics.o ./main.o ./master.o ./slave.o ./XML.o  -lpthread -lxml2

cd ~/git/node_manager/cluster_control