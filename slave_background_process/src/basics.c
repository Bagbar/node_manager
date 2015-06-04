/*
 * basics.c
 *
 *  Created on: 02.06.2015
 *      Author: xubuntu
 */
#include "basics.h"

void criterr(char *s, int sock)
{   perror(s);
    close(sock);
    exit(1);
}



