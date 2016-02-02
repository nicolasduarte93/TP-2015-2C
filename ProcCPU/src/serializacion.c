/*
 * serializacion.c
 *
 *  Created on: 25/10/2015
 *      Author: utnso
 */

#include "estructuras.h"
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>


void * serializarPaquetePlanificador(protocolo_planificador_cpu * protocolo, int * tamanio) {
	void * chorro = malloc(22 + protocolo->tamanioMensaje);
	memcpy(chorro, &(protocolo->tipoProceso), 1);
	memcpy(chorro + 1, &(protocolo->tipoOperacion), 1);
	memcpy(chorro + 2, &(protocolo->estado), 4);
	memcpy(chorro + 6, &(protocolo->pid), 4);
	memcpy(chorro + 10, &(protocolo->counterProgram), 4);
	memcpy(chorro + 14, &(protocolo->quantum), 4);
	memcpy(chorro + 18, &(protocolo->tamanioMensaje), 4);
	memcpy(chorro + 22, protocolo->mensaje, protocolo->tamanioMensaje);
	*tamanio = 22 + protocolo->tamanioMensaje;
	return chorro;
}

void* serializarPaqueteMemoria(protocolo_cpu_memoria* paquete, int* tamanio) {

	size_t messageLength = strlen(paquete->mensaje) + 1;

	void* paqueteSerializado = malloc(sizeof(protocolo_cpu_memoria) + messageLength);
	int offset = 0;
	int size_to_send;

	size_to_send = sizeof(paquete->tipoOperacion);
	memcpy(paqueteSerializado + offset, &(paquete->tipoOperacion),
			size_to_send);
	offset += size_to_send;

	size_to_send = sizeof(paquete->pid);
	memcpy(paqueteSerializado + offset, &(paquete->pid), size_to_send);
	offset += size_to_send;

	size_to_send = sizeof(paquete->nroPagina);
	memcpy(paqueteSerializado + offset, &(paquete->nroPagina), size_to_send);
	offset += size_to_send;

	size_to_send = sizeof(paquete->tamanioMensaje);
	memcpy(paqueteSerializado + offset, &messageLength, size_to_send);
	offset += size_to_send;

	size_to_send = messageLength;
	memcpy(paqueteSerializado + offset, paquete->mensaje, size_to_send);
	offset += size_to_send;

	*tamanio = offset;
	return paqueteSerializado;
}

int deserializarPlanificador(protocolo_planificador_cpu *package,int socketPlanificador) {
	int status;
	void* buffer = malloc(sizeof(protocolo_planificador_cpu)-4);
	int offset = 0;

	status = recv(socketPlanificador, buffer,sizeof(protocolo_planificador_cpu)-4, 0);

	if(!status) return 0;

	memcpy(&(package->tipoProceso), buffer, sizeof(package->tipoProceso));
	offset += sizeof(package->tipoProceso);

	memcpy(&(package->tipoOperacion), buffer+ offset, sizeof(package->tipoOperacion));
	offset += sizeof(package->tipoOperacion);

	memcpy(&(package->estado), buffer + offset, sizeof(package->estado));
	offset += sizeof(package->estado);

	memcpy(&(package->pid), buffer + offset, sizeof(package->pid));
	offset += sizeof(package->pid);

	memcpy(&(package->counterProgram), buffer + offset,sizeof(package->counterProgram));
	offset += sizeof(package->counterProgram);

	memcpy(&(package->quantum), buffer + offset, sizeof(package->quantum));
	offset += sizeof(package->quantum);

	memcpy(&(package->tamanioMensaje), buffer + offset,sizeof(package->tamanioMensaje));
	offset += sizeof(package->tamanioMensaje);

	package->mensaje = (char*)malloc((package->tamanioMensaje)+1); //valgrind is 1 bytes before a block of size 1 alloc'd

	status = recv(socketPlanificador, package->mensaje, package->tamanioMensaje,0); //valgrind is on thread 1's stack

	package->mensaje[package->tamanioMensaje-1]= '\0';

	free(buffer);

	return status;

}

int deserializarMemoria(protocolo_memoria_cpu * package, int socket_memoria) {
	void* buffer = malloc(sizeof(protocolo_memoria_cpu)-4);
	if (recv(socket_memoria, buffer, sizeof(protocolo_memoria_cpu)-4, 0) <= 0) return false;
	memcpy(&(package->tipoProceso), buffer , 1);
	memcpy(&(package->codOperacion), buffer + 1 , 1);
	memcpy(&(package->codAux), buffer + 2 , 1);
	memcpy(&(package->pid), buffer + 3 , 4);
	memcpy(&(package->numeroPagina), buffer + 7 , 4);
	memcpy(&(package->tamanioMensaje), buffer + 11 , 4);
	// ahora el mensaje posta
	package->mensaje = (char*)malloc(package->tamanioMensaje + 1);
	if(recv(socket_memoria, package->mensaje, package->tamanioMensaje, 0) <= 0) return false;
	package->mensaje[package->tamanioMensaje] = '\0';
	free(buffer);
	return 1;
}
