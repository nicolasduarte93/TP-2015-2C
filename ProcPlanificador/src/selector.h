/*
 * selector.h
 *
 *  Created on: 27/10/2015
 *      Author: utnso
 */

#ifndef SRC_SELECTOR_H_
#define SRC_SELECTOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <commons/collections/list.h>

void* selector(void* arg);

#endif /* SRC_SELECTOR_H_ */
