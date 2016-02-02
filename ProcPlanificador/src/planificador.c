#include "funcionesPlanificador.h"
#include "estructuras.h"
#include "libSocket.h"
#include <commons/string.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <commons/log.h>
#include "config.h"
#include "selector.h"
#include <commons/collections/queue.h>
#include <pthread.h>
#include <stdbool.h>
#include "estructuras.h"
#include "logueo.h"

bool llegoexit = false;

void* consumidor(){
	tprocIO* pcbIO;
	while(1){
		sem_wait(&hayIO);
		if(llegoexit) break;
		pthread_mutex_lock(&mutexIO);
		pcbIO = queue_peek(colaIO);
		pthread_mutex_unlock(&mutexIO);

		bool finalizar = hayQueFinalizarlo(pcbIO->pcb->pid);
		/*que no haga io si hay que finalizarlo*/
		if(!finalizar)
			sleep(pcbIO->tiempo);

		pcbIO->pcb->tpoBloqueado += (time(NULL)-pcbIO->pcb->entroIO);

		pthread_mutex_lock(&mutexSwitchProc);
		/*una vez que hizo io recien lo saco de la cola de io*/
		pthread_mutex_lock(&mutexIO);
		queue_pop(colaIO);
		pthread_mutex_unlock(&mutexIO);
		pcbIO->pcb->estado = LISTO;

		/*pongo en listo*/
		pthread_mutex_lock(&mutexProcesoListo);
		if(finalizar){
			/*actualizo cp*/
			pcbIO->pcb->siguiente = pcbIO->pcb->maximo;
			ponerPrimero(&colaListos, pcbIO->pcb);
		}
		else
			queue_push(colaListos,pcbIO->pcb);
		pthread_mutex_unlock(&mutexProcesoListo);
		pthread_mutex_unlock(&mutexSwitchProc);

		sem_post(&hayProgramas);
		printf("pid-> %d salio de io\n",pcbIO->pcb->pid);
		free(pcbIO);
	}
	pthread_exit(0);
	return 0;
}

void *enviar(){

	tpcb* pcb;
	protocolo_planificador_cpu* package = malloc(sizeof(protocolo_planificador_cpu));
	int tamanio = 0;
	int * socketCPU;

	while(1){
		sem_wait(&hayCPU);
		sem_wait(&hayProgramas);
		if(llegoexit) break;
		pthread_mutex_lock(&mutexSwitchProc);
		pthread_mutex_lock(&mutexProcesoListo);
		pcb = queue_pop(colaListos);
		pthread_mutex_unlock(&mutexProcesoListo);
		if(pcb->siguiente == 1){
			pcb->estado = EJECUTANDO;
			pthread_mutex_lock(&mutexInicializando);
			list_add(listaInicializando,pcb);
			pthread_mutex_unlock(&mutexInicializando);
		}else{
			pcb->estado = EJECUTANDO;
			pthread_mutex_lock(&mutexListaEjecutando);
			list_add(listaEjecutando,pcb);
			pthread_mutex_unlock(&mutexListaEjecutando);
		}
		pcb->entroCPU=time(NULL);
		pthread_mutex_unlock(&mutexSwitchProc);
		adaptadorPCBaProtocolo(pcb,package);
		void* message = serializarPaqueteCPU(package, &tamanio);
		pthread_mutex_lock(&mutexListaCpusLibres);
		socketCPU = list_remove(listaCpuLibres, 0);
		pthread_mutex_unlock(&mutexListaCpusLibres);
		if(send(*socketCPU,message,tamanio,0) < 0)
			printf("fallo envio a cpu %d\n", *socketCPU);

		free(message);
	}
	free(package);
	//free(pcb);
	pthread_exit(0);
	return 0;
}

int main(){
	system("clear");

	tiempoEjec=time(NULL);

	listaEjecutando = list_create();
	listaCpuLibres = list_create();
	listaInicializando = list_create();
	listaAfinalizar = list_create();
	listaPorcentajeCpus = list_create();
	listaCpus = list_create();
	colaListos = queue_create();
	colaIO = queue_create();

	sem_init(&hayProgramas,0,0);
	sem_init(&hayCPU,0,0);
	sem_init(&hayIO,0,0);
	pthread_mutex_init(&mutexProcesoListo,NULL);
	pthread_mutex_init(&mutexInicializando,NULL);
	pthread_mutex_init(&mutexListaCpusLibres,NULL);
	pthread_mutex_init(&mutexIO,NULL);
	pthread_mutex_init(&mutexListaEjecutando,NULL);
	pthread_mutex_init(&mutexSwitchProc,NULL);
	pthread_mutex_init(&mutexFinalizarPid,NULL);
	pthread_mutex_init(&mutexListasCpu,NULL);
	pthread_mutex_init(&mutexListasPorcentajes,NULL);
	//creacion de la instancia de log
	logPlanificador = log_create("../src/log.txt", "planificador.c", false, LOG_LEVEL_INFO);

	//leemos el archivo de configuracion
	configPlanificador = leerConfiguracion();

	//Inicia el socket para escuchar
	int serverSocket;
	server_init(&serverSocket, configPlanificador->puertoEscucha);
	printf("Planificador listo...\n");

	tParametroSelector sel;
	sel.socket = serverSocket;
	sel.listaCpus = listaCpuLibres;

	pthread_t enviarAlCpu,selectorCpu,consumidorIO;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	/*creacion de hilos*/
	pthread_create(&selectorCpu,&attr,selector,(void*)&sel);
	pthread_create(&enviarAlCpu,&attr,enviar,NULL);
	pthread_create(&consumidorIO,&attr,consumidor,NULL);

	int enviar = 1;
	int cantProc = 1;
	int lng = 0;
	char * message = 0;

	while(enviar){
		/*reemplaza a fgets*/
		while(1){
			int c ;
			if ((c = getchar()) == EOF)
				break;

			message = realloc(message,lng + 1);
			message[lng] = c;
			lng++;

			if (c == '\n')
				break;
		}

		message = realloc(message,lng + 1);
		message[lng] = '\0';

		if (!strcmp(message,"exit\n")) {
			enviar = 0;
			llegoexit = true;
			sem_post(&hayCPU);
			sem_post(&hayProgramas);
			sem_post(&hayIO);
		}
		else
			procesarComando(clasificarComando(message),message,&cantProc);

		/*reinicio*/
		free(message);
		message = 0;
		lng = 0;
	}
	/*espero la terminacion de enviar*/
	pthread_join(enviarAlCpu,NULL);
	pthread_join(consumidorIO,NULL);

	sem_destroy(&hayCPU);
	sem_destroy(&hayIO);
	sem_destroy(&hayProgramas);
	pthread_mutex_destroy(&mutexIO);
	pthread_mutex_destroy(&mutexInicializando);
	pthread_mutex_destroy(&mutexListaCpusLibres);
	pthread_mutex_destroy(&mutexListaEjecutando);
	pthread_mutex_destroy(&mutexProcesoListo);
	pthread_mutex_destroy(&mutexSwitchProc);
	pthread_mutex_destroy(&mutexFinalizarPid);
	pthread_mutex_destroy(&mutexListasCpu);
	pthread_mutex_destroy(&mutexListasPorcentajes);
	/*destruyo la lista y sus elementos*/
	list_destroy_and_destroy_elements(listaCpuLibres,free);
	list_destroy_and_destroy_elements(listaEjecutando,free);
	list_destroy_and_destroy_elements(listaInicializando,free);
	list_destroy_and_destroy_elements(listaAfinalizar,free);
	list_destroy_and_destroy_elements(listaCpus,free);
	list_destroy_and_destroy_elements(listaPorcentajeCpus,free);
	/*destruyo la cola y sus elementos*/
	queue_destroy_and_destroy_elements(colaListos,free);

	close(serverSocket);
	logTpoTotal();
	return 0;
}
