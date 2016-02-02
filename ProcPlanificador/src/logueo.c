/*
 * logueo.c
 *
 *  Created on: 3/11/2015
 *      Author: utnso
 */

#include "funcionesPlanificador.h"
#include <commons/string.h>
#include <commons/log.h>

/*void convertirOperacionAInstruccion(char operacion, char** instruccion){
	switch(operacion){

	case 'i': string_append(&instruccion, "Iniciado"); break;
	case 'e': string_append(&instruccion, "Entrada-Salida"); break;
	case 'l': string_append(&instruccion, "Lectura"); break;
	case 'E': string_append(&instruccion, "Escritura"); break;
	case 'f': string_append(&instruccion, "Finalizo"); break;

	}
}*/

void logueoProcesos(int pid, char* path, char operacion, tpcb* pcb){

	if (operacion == 'i')
		log_info(logPlanificador,"Proceso %s con PID: %d Iniciado\n",nombrePrograma(path),pid);
	else {
		log_info(logPlanificador,"Proceso %s con PID: %d Finalizado\n",nombrePrograma(path),pid);
		log_info(logPlanificador,"Paso %d segundos Ejecutando, %d segundos Bloqueado, %d segundos desde que se inicio hasta que finalizo\n",pcb->tpoCPU,pcb->tpoBloqueado,(time(NULL)-pcb->llegada));
	}

}


char* contenidoDeColas(t_list* lista){
	int i;
	char* contenido = string_new();
	for(i = 0; i < list_size(lista); i++){
		tpcb* pcb = list_get(lista,i);
		string_append_with_format(&contenido,"PID: %d ",pcb->pid);
		string_append(&contenido,pcb->nombre);
		string_append(&contenido,",");
	}
	return contenido;
}

char* contenidoDeColasIO(t_list* lista){
	int i;
	char* contenido = string_new();
	for(i = 0; i < list_size(lista); i++){
		tprocIO* pcb = list_get(lista,i);
		string_append_with_format(&contenido,"PID: %d ",pcb->pcb->pid);
		string_append(&contenido,pcb->pcb->nombre);
		string_append(&contenido,",");
	}
	return contenido;
}

void logueoAlgoritmo(int inicial,char* mProc){

	char* auxiliar;
	char* algoritmo = string_new();
	if(inicial == 0) string_append(&algoritmo,"FIFO");
	else string_append(&algoritmo,"RR");
	log_info(logPlanificador,"El Proceso %s se encuentra ejecutando algoritmo %s\n",mProc,algoritmo);
	//Envio mensaje para loguear colas de planificacion
	char* contenido = string_new();
	pthread_mutex_lock(&mutexSwitchProc);

	string_append(&contenido,"\nCola Ready: ");
	auxiliar = contenidoDeColas(colaListos->elements);
	string_append(&contenido,auxiliar);
	free(auxiliar);
	auxiliar = contenidoDeColas(listaInicializando);
	string_append(&contenido,auxiliar);
	free(auxiliar);

	string_append(&contenido,"\nCola Ejecutando: ");
	auxiliar = contenidoDeColas(listaEjecutando);
	string_append(&contenido,auxiliar);
	free(auxiliar);

	string_append(&contenido,"\nCola I/O: ");
	auxiliar = contenidoDeColasIO(colaIO->elements);
	string_append(&contenido,auxiliar);
	free(auxiliar);

	pthread_mutex_unlock(&mutexSwitchProc);

	log_info(logPlanificador,contenido);
	free(contenido);
	free(algoritmo);

}

void logTpoTotal(){
	log_info(logPlanificador,"Tiempo total transcurrido de ejecucion %d segundos",(time(NULL)-tiempoEjec));
}

/*void logueoRafaga(char* mProc, char operacion){
	char* logueo = (char*)malloc(65+strlen(mProc));
	char* instruccion = string_new();
	string_append_with_format(&logueo,"Rafaga completada del proceso %s \n",nombrePrograma(mProc));
	convertirOperacionAInstruccion(operacion,&instruccion);
	string_append(&logueo,instruccion);
	log_info(logPlanificador,"Rafaga completada del proceso %s \n",nombrePrograma(mProc));
	//Ver si la cpu envia toda la rafaga en un mensaje y loguear eso

	free(logueo);
}*/
