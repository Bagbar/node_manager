#!/bin/sh

gcc -std=c99 -I/usr/include/libxml2 -O0 -c -o "basics.o" "basics.c"
gcc -std=c99 -I/usr/include/libxml2 -O0 -c -o "main.o" "main.c"
gcc -std=c99 -I/usr/include/libxml2 -O0 -c -o "master.o" "master.c"
gcc -std=c99 -I/usr/include/libxml2 -O0 -c -o "slave.o" "slave.c"
gcc -std=c99 -I/usr/include/libxml2 -O0 -c -o "XML.o" "XML.c"

gcc  -o "cluster_control"  ./basics.o ./main.o ./master.o ./slave.o ./XML.o  -lpthread -lxml2

