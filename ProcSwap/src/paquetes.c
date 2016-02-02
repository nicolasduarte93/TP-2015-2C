#include <stdio.h>
#include "estructuras.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>

FILE* iniciar_archivo_swap(void) {
	tconfig_swap *config_swap = leerConfiguracion();
	FILE* swap = fopen(config_swap->nombreSwap, "wb+"); //rb+ para archivo binario
	//FILE* swap = fopen(config_swap->nombreSwap, "rb+"); para probar
	size_t tamanio_swap = config_swap->tamanioPagina * config_swap->cantidadPaginas;
	//rellenamos en cero (char '\0' es 0)
	int i ;
	char cero = 0;
	for (i = 0 ; i < tamanio_swap; i++)
		fwrite(&cero, sizeof(char), 1, swap);
	fseek(swap, 0, SEEK_SET);
	return swap;

}

bool recibir_paquete_desde_memoria(int *socket_memoria, tprotocolo_memoria_swap *paquete_desde_memoria) {
	void* buffer = malloc(sizeof(tprotocolo_memoria_swap) - 4);
	if (recv(*socket_memoria, buffer, sizeof(tprotocolo_memoria_swap) - 4, 0) <= 0) return false;
	memcpy(&(paquete_desde_memoria->codigo_op), buffer ,1);
	memcpy(&(paquete_desde_memoria->pid), buffer + 1 ,4);
	memcpy(&(paquete_desde_memoria->cantidad_pagina), buffer + 5 ,4);
	memcpy(&(paquete_desde_memoria->tamanio_mensaje), buffer + 9 ,4);
	// ahora el mensaje posta
	paquete_desde_memoria->mensaje = (char*)malloc(paquete_desde_memoria->tamanio_mensaje);
	if(recv(*socket_memoria, paquete_desde_memoria->mensaje, paquete_desde_memoria->tamanio_mensaje, 0) <= 0) return false;
	free(buffer);
	return true;
}

void* serializar_a_memoria(tprotocolo_swap_memoria *protocolo) {
	void * chorro = malloc(9 + protocolo->tamanio);
	memcpy(chorro, &(protocolo->codAux), 1);
	memcpy(chorro+1, &(protocolo->pid), 4);
	memcpy(chorro + 5, &(protocolo->tamanio), 4);
	memcpy(chorro + 9, protocolo->mensaje, protocolo->tamanio);
	return chorro;
}

void armar_estructura_protocolo_a_memoria(tprotocolo_swap_memoria *protocolo,char codAux, int pid, char* mensaje) {
	protocolo->codAux = codAux;
	protocolo->pid = pid;
	protocolo->mensaje = malloc(strlen(mensaje) + 1);
	strcpy(protocolo->mensaje, mensaje);
	protocolo->tamanio = strlen(mensaje) + 1;
}
