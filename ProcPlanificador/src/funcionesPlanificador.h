/*
 * funcionesPlanificador.h
 *
 *  Created on: 22/9/2015
 *      Author: utnso
 */

#ifndef SRC_FUNCIONESPLANIFICADOR_H_
#define SRC_FUNCIONESPLANIFICADOR_H_

#include "estructuras.h"


tpcb* armarPCB (char* path,int cant);
int clasificarComando(char* message);
void adaptadorPCBaProtocolo(tpcb* pcb,protocolo_planificador_cpu* paquete);
void procesarComando(int nro_comando, char* message, int* cantProc);
void convertirEstado(testado estadoEnum, char** estado);
void mostrarEstadoProcesosLista(t_list* lista, char* mensaje);
int deserializarCPU(protocolo_planificador_cpu *package,int socketCPU);
void* serializarPaqueteCPU(protocolo_planificador_cpu* paquete, int* tamanio);
int maxLineas(char * archivo);
char* nombrePrograma(char* path);
int buscoPCB(int pidBuscado,t_list* lista);
void finalizarPID(char* pidBuscado);
bool hayQueFinalizarlo(int pid);
bool comparadorTid(void * elemA, void * elemB);
void ponerPrimero(t_queue ** cola, tpcb * pcb);
/*char* definirMensaje(tpcb* pcb);*/

#endif /* SRC_FUNCIONESPLANIFICADOR_H_ */
