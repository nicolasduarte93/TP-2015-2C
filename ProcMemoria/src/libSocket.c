#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<netdb.h>
#include<unistd.h>
#include<stdbool.h>


void socket_close(int socket){
	close(socket);
}

void client_init(int * cliSocket, char *ip, char *puerto){

	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/*carga en serverInfo los datos de la conexion*/
	getaddrinfo(ip, puerto, &hints, &serverInfo);


	/*creamos el socket*/
	*cliSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	/*conectamos al servidor*/
	connect(*cliSocket, serverInfo->ai_addr, serverInfo->ai_addrlen);

	/*liberamos*/
	freeaddrinfo(serverInfo);

}

void server_init(int *svrSocket, char *puerto){

	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;

	/*carga en serverInfo los datos de la conexion*/
	getaddrinfo(NULL, puerto, &hints, &serverInfo);

	/*creamos el socket*/
	*svrSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

       int option = 1;
       setsockopt(*svrSocket,SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),&option,sizeof(option));

	bind(*svrSocket,serverInfo->ai_addr, serverInfo->ai_addrlen);

	/*liberamos*/
	freeaddrinfo(serverInfo);

	listen(*svrSocket, SOMAXCONN);

}

void server_acept(int serverSocket, int *clientSocket){

	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	*clientSocket = accept(serverSocket, (struct sockaddr *) &addr, &addrlen);
}




