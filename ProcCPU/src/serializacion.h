/*
 * serializacion.h
 *
 *  Created on: 25/10/2015
 *      Author: utnso
 */

#ifndef SRC_SERIALIZACION_H_
#define SRC_SERIALIZACION_H_

#include "estructuras.h"

int deserializarMemoria(protocolo_memoria_cpu * package, int socket_memoria);
int deserializarPlanificador(protocolo_planificador_cpu* package,int socketPlanificador);
void* serializarPaqueteMemoria(protocolo_cpu_memoria* paquete, int* tamanio);
void* serializarPaquetePlanificador(protocolo_planificador_cpu* paquete, int* tamanio);

#endif
