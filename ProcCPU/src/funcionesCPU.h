/*
 * funcionesCPU.h
 *
 *  Created on: 23/9/2015
 *      Author: utnso
 */

#ifndef SRC_FUNCIONESCPU_H_
#define SRC_FUNCIONESCPU_H_

#include <commons/string.h>
#include <stdio.h>
#include "estructuras.h"

bool interpretarInstruccion(char* instruccion, protocolo_planificador_cpu* mensajeDePlanificador,protocolo_cpu_memoria* mensajeParaArmar,int socketPlanificador,t_log* logCpu);
char* leerInstruccion(int* instructionPointer, char* lineaLeida, FILE* archivo, int tam);
void enviarAMemoria(protocolo_cpu_memoria* message);
void armarPaqueteMemoria(protocolo_cpu_memoria* paquete, char codOperacion, int pid, int nroPagina, char* mensaje);
void actualizarOperacionPaquetePlanificador(protocolo_planificador_cpu* paquete, char tipoOperacion);
void actualizarOperacionPaquetePlanificadorIO(protocolo_planificador_cpu* paquete, char tipoOperacion,int IO);
void enviarAPlanificador(protocolo_planificador_cpu* respuestaDeMemo,int socketPlanificador);


#endif /* SRC_FUNCIONESCPU_H_ */
