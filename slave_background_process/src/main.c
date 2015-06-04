#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "basics.h"
#include "mastercom.h"



int main() {
	struct thread_param time_count;
	time_count.var = 1;


	pthread_t background_listen;
	if (pthread_create(&background_listen, NULL, listen_for_master,(void*) &time_count)) {
		criterr("pthread_create(listen)=", 0);
	}


	while (1) {

		if(pthread_mutex_lock(&time_count.mtx))
						criterr("main: mutex_lock:");
		if (time_count.var > PING_PERIOD * TIMEOUT_PERIODS) {
			printf("main:timecount_over_limit:%d\n",time_count.var);
			if(pthread_mutex_unlock(&time_count.mtx))
				criterr("main: over_mutex_unlock:");
			// TODO (kami#1#): start search for new master

		} else {

			printf("main:increase time_count to:%d\n", ++time_count.var);
			if(pthread_mutex_unlock(&time_count.mtx))
				criterr("main: under_mutex_unlock:");

		}
		// TODO (kami#9#): may use PING_PERIOD here
		sleep(2);
			}

	return 0;
}
