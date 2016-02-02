/*
 * logueo.h
 *
 *  Created on: 3/11/2015
 *      Author: utnso
 */

#ifndef SRC_LOGUEO_H_
#define SRC_LOGUEO_H_


#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>
#include "estructuras.h"

void logueoProcesos(int pid, char* path, char operacion,tpcb* pcb);
void logueoAlgoritmo(int algoritmo,char* mProc);
char* contenidoDeColas(t_list* lista);
char* contenidoDeColasIO(t_list* lista);
void logTpoTotal();




#endif /* SRC_LOGUEO_H_ */

