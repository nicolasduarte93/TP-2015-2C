/*
 * libSocket.h
 *
 *  Created on: 1/10/2015
 *      Author: utnso
 */

#ifndef SRC_LIBSOCKET_H_
#define SRC_LIBSOCKET_H_


void socket_close(int socket);
void client_init(int * cliSocket, char *ip, char *puerto);
void server_init(int *svrSocket, char *puerto);
void server_acept(int serverSocket, int *clientSocket);





#endif /* SRC_LIBSOCKET_H_ */
