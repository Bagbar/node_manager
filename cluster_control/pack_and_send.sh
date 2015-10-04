#!/bin/sh

tar cf sources.tar XML.c XML.h slave.c slave.h unpack_compile.sh compile.sh master.c master.h main.c basics.c basics.h

scp sources.tar linaro@192.168.1.4:~/sources.tar
scp sources.tar linaro@192.168.1.3:~/sources.tar