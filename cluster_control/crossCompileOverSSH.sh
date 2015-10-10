#!/bin/sh

scp ~/git/node_manager/dummy_work/main.c linaro@192.168.1.3:~/dummy_work.c
ssh linaro@192.168.1.3
gcc dummy_work.c -o dummy_arm
exit
scp linaro@192.168.1.3:~/dummy_arm ~/git/node_manager/start_cluster/dummy_arm