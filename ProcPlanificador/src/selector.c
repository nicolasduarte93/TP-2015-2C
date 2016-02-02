#include <commons/collections/list.h>
#include <pthread.h>
#include <commons/string.h>
#include "estructuras.h"
#include "funcionesPlanificador.h"
#include "libSocket.h"
#include "logueo.h"

void * selector(void * arg) {
	fd_set descriptores;
	int numeroCpus = 0;
	int i;

	int servidor = ((tParametroSelector*)arg)->socket;

	while(1){
		FD_ZERO(&descriptores);

		/*añado el socket del planificador*/
		FD_SET(servidor, &descriptores);
		/*añado las cpus*/
		for (i=0; i < numeroCpus; i++)
			FD_SET (*((int*)list_get(listaCpus,i)), &descriptores);

		/*busco el descriptor mas grande*/
		int maxfdset = 0;

		if (numeroCpus > 0){
			maxfdset = *((int*)list_get(listaCpus,0));
			for (i = 0; i < numeroCpus; i++)
				if (*((int*)list_get(listaCpus,i)) > maxfdset)
					maxfdset = *((int*)list_get(listaCpus,i));
		}

		if (maxfdset < servidor)
			maxfdset = servidor;

		/*espero hasta que ocurra un evento*/
		if (select (maxfdset + 1, &descriptores, NULL, NULL, NULL) == -1)
			break;

		/*compruebo las cpus*/
		for (i = 0; i < numeroCpus; i++){

			if (FD_ISSET (*((int*)list_get(listaCpus,i)), &descriptores)){

				/*leo lo que me mando la cpu*/
				protocolo_planificador_cpu respuestaDeCPU;
				if(deserializarCPU(&respuestaDeCPU, *((int*)list_get(listaCpus,i)))){

					switch(respuestaDeCPU.tipoOperacion){
						case 'e':{
							int* puntero = malloc(sizeof(int));
							*puntero = *((int*)list_get(listaCpus,i));
							pthread_mutex_lock(&mutexListaCpusLibres);
							/*agreo la cpu a lista disponible*/
							list_add(listaCpuLibres,puntero);
							pthread_mutex_unlock(&mutexListaCpusLibres);

							tprocIO* pcbIO = malloc(sizeof(tprocIO));
							pthread_mutex_lock(&mutexSwitchProc);
							pthread_mutex_lock(&mutexListaEjecutando);
							pcbIO->pcb = list_remove(listaEjecutando,buscoPCB(respuestaDeCPU.pid,listaEjecutando));
							pthread_mutex_unlock(&mutexListaEjecutando);
							/*lo mando a listo, no hace io*/
							if(hayQueFinalizarlo(pcbIO->pcb->pid)){
								/*actualizo cp*/
								pcbIO->pcb->siguiente = pcbIO->pcb->maximo;
								pcbIO->pcb->estado = LISTO;
								pthread_mutex_lock(&mutexProcesoListo);
								ponerPrimero(&colaListos,pcbIO->pcb);
								pthread_mutex_unlock(&mutexProcesoListo);
								sem_post(&hayProgramas);
							}
							/*no hay que finalizarlo, mando a io*/
							else{
								pcbIO->pcb->siguiente = respuestaDeCPU.counterProgram;
								pcbIO->tiempo = atoi(respuestaDeCPU.mensaje);
								pcbIO->pcb->estado = IO;
								pcbIO->pcb->entroIO=time(NULL);
								pthread_mutex_lock(&mutexIO);
								queue_push(colaIO,pcbIO);
								pthread_mutex_unlock(&mutexIO);
								printf("pid-> %d entro a io\n", respuestaDeCPU.pid);
								sem_post(&hayIO);
							}
							pcbIO->pcb->tpoCPU += (time(NULL)-pcbIO->pcb->entroCPU);
							pthread_mutex_unlock(&mutexSwitchProc);

							sem_post(&hayCPU);
						}
						break;

						case 'i':{
							tpcb* pcb;
							pthread_mutex_lock(&mutexSwitchProc);
							pthread_mutex_lock(&mutexInicializando);
							pcb = list_remove(listaInicializando,buscoPCB(respuestaDeCPU.pid,listaInicializando));
							pthread_mutex_unlock(&mutexInicializando);
							pcb->estado = EJECUTANDO;

							pthread_mutex_lock(&mutexListaEjecutando);
							list_add(listaEjecutando,pcb);
							pthread_mutex_unlock(&mutexListaEjecutando);
							pthread_mutex_unlock(&mutexSwitchProc);

							logueoProcesos(respuestaDeCPU.pid,respuestaDeCPU.mensaje,'i',pcb);
							logueoAlgoritmo(respuestaDeCPU.quantum,pcb->nombre);
							printf("pid-> %d inicio correctamente\n", respuestaDeCPU.pid);

						}
						break;

						/*aca se usa tipo proceso*/
						case 'a':{
							int* puntero = malloc(sizeof(int));
							*puntero = *((int*)list_get(listaCpus,i));
							pthread_mutex_lock(&mutexListaCpusLibres);
							/*agreo la cpu a lista disponible*/
							list_add(listaCpuLibres,puntero);
							pthread_mutex_unlock(&mutexListaCpusLibres);
							sem_post(&hayCPU);

							/*es un fallo de inicializacion*/
							if (respuestaDeCPU.tipoProceso == 'i'){
								pthread_mutex_lock(&mutexSwitchProc);
								pthread_mutex_lock(&mutexInicializando);
								free(list_remove(listaInicializando,buscoPCB(respuestaDeCPU.pid,listaInicializando)));
								pthread_mutex_unlock(&mutexInicializando);
								pthread_mutex_unlock(&mutexSwitchProc);
								log_info(logPlanificador, "Proceso con PID: %d. Fallo la Inicializacion\n", respuestaDeCPU.pid);
								printf("pid-> %d fallo la inicializacion\n", respuestaDeCPU.pid);
							}

							/*es un fallo de lectura o escritura*/
							else {
								pthread_mutex_lock(&mutexSwitchProc);
								pthread_mutex_lock(&mutexListaEjecutando);
								free(list_remove(listaEjecutando,buscoPCB(respuestaDeCPU.pid,listaEjecutando)));
								pthread_mutex_unlock(&mutexListaEjecutando);
								pthread_mutex_unlock(&mutexSwitchProc);
								log_info(logPlanificador, "Proceso con PID: %d abortado. Fallo la %s\n",respuestaDeCPU.pid,(respuestaDeCPU.tipoProceso == 'l' ? "lectura":"escritura"));
								printf("pid-> %d proceso abortado: fallo la %s\n", respuestaDeCPU.pid, (respuestaDeCPU.tipoProceso == 'l' ? "lectura":"escritura"));
							}
						}
						break;

						case 'q':{
							int* puntero = malloc(sizeof(int));
							*puntero = *((int*)list_get(listaCpus,i));
							pthread_mutex_lock(&mutexListaCpusLibres);
							/*agreo la cpu a lista disponible*/
							list_add(listaCpuLibres, puntero);
							pthread_mutex_unlock(&mutexListaCpusLibres);

							tpcb* pcb;
							pthread_mutex_lock(&mutexSwitchProc);
							pthread_mutex_lock(&mutexListaEjecutando);
							pcb = list_remove(listaEjecutando,buscoPCB(respuestaDeCPU.pid,listaEjecutando));
							pthread_mutex_unlock(&mutexListaEjecutando);
							pthread_mutex_lock(&mutexProcesoListo);
							if(hayQueFinalizarlo(pcb->pid)){
								/*actualizo cp*/
								pcb->siguiente = pcb->maximo;
								pcb->estado = LISTO;
								ponerPrimero(&colaListos, pcb);
							}
							else{
								pcb->siguiente = respuestaDeCPU.counterProgram;
								pcb->estado = LISTO;
								queue_push(colaListos,pcb);
							}
							pcb->tpoCPU += (time(NULL)-pcb->entroCPU);
							pthread_mutex_unlock(&mutexProcesoListo);
							pthread_mutex_unlock(&mutexSwitchProc);

							sem_post(&hayProgramas);
							sem_post(&hayCPU);
							log_info(logPlanificador, "Rafaga Completada Proceso %s PID: %d\n",pcb->nombre,pcb->pid);
							printf("pid-> %d volvio por quantum\n", respuestaDeCPU.pid);
						}
						break;

						case 'f':{
							int* puntero = malloc(sizeof(int));
							*puntero = *((int*)list_get(listaCpus,i));
							pthread_mutex_lock(&mutexListaCpusLibres);
							/*agreo la cpu a lista disponible*/
							list_add(listaCpuLibres, puntero);
							pthread_mutex_unlock(&mutexListaCpusLibres);

							pthread_mutex_lock(&mutexSwitchProc);
							pthread_mutex_lock(&mutexListaEjecutando);
							tpcb* pcb=list_remove(listaEjecutando,buscoPCB(respuestaDeCPU.pid,listaEjecutando));
							pthread_mutex_unlock(&mutexListaEjecutando);
							pthread_mutex_unlock(&mutexSwitchProc);

							pcb->tpoCPU += (time(NULL)-pcb->entroCPU);
							logueoProcesos(respuestaDeCPU.pid,respuestaDeCPU.mensaje,'f',pcb);
							free(pcb);
							sem_post(&hayCPU);
							printf("pid-> %d finalizo\n", respuestaDeCPU.pid);
						}
						break;

						case 'u':{
							tPorcentajeCpu* porcentaje = malloc(sizeof(tPorcentajeCpu));
							porcentaje->tid = respuestaDeCPU.pid;
							porcentaje->porcentaje = respuestaDeCPU.counterProgram;
							pthread_mutex_lock(&mutexListasPorcentajes);
							list_add(listaPorcentajeCpus,porcentaje);

							if(list_size(listaPorcentajeCpus) >= list_size(listaCpus)){
								/*ordeno por tid*/
								list_sort(listaPorcentajeCpus,comparadorTid);
								int i;
								for(i = 0; i < list_size(listaPorcentajeCpus); i++){
									tPorcentajeCpu* aMostrar = list_get(listaPorcentajeCpus,i);
									printf("CPU %d : %d %c\n",aMostrar->tid,aMostrar->porcentaje,37);
								}
								list_clean_and_destroy_elements(listaPorcentajeCpus,free);
							}
							pthread_mutex_unlock(&mutexListasPorcentajes);
						}
						break;
					}
				}
				else
				{
					log_info(logPlanificador,"Conexion en socket %d cerrada\n",*((int*)list_get(listaCpus,i)));
					printf("CPU desconectada: %d\n", *((int*)list_get(listaCpus,i)));

					/*elimino cpu*/
					pthread_mutex_lock(&mutexListasCpu);
					int j;
					for(j = 0; j < list_size(listaCpus); j++){
						int* cpu = list_get(listaCpus,j);
						if(*cpu == *((int*)list_get(listaCpus,i))) {
							free(list_remove(listaCpus,j));
							numeroCpus--;
							break;
						}
					}
					pthread_mutex_unlock(&mutexListasCpu);
				}
			}
		}

		/*compruebo si una cpu se quiere conectar*/
		if (FD_ISSET (servidor, &descriptores)){

			int nuevaCpu;
			/*acepto la cpu*/
			server_acept(servidor, &nuevaCpu);

			log_info(logPlanificador,"Nueva conexion en socket %d\n",nuevaCpu);
			if (nuevaCpu != -1){

				int * cpu1 = malloc(sizeof(int));
				*cpu1 = nuevaCpu;
				pthread_mutex_lock(&mutexListaCpusLibres);
				/*agreo la cpu a lista disponible*/
				list_add(listaCpuLibres, cpu1);
				pthread_mutex_unlock(&mutexListaCpusLibres);

				int * cpu2 = malloc(sizeof(int));
				*cpu2 = nuevaCpu;
				pthread_mutex_lock(&mutexListasCpu);
				/*agreo la cpu a lista de cpu conectadas*/
				list_add(listaCpus,cpu2);
				numeroCpus++;
				pthread_mutex_unlock(&mutexListasCpu);

				sem_post(&hayCPU);
				printf("Nueva CPU conectada: %d\n", nuevaCpu);
			}
		}
	}
	pthread_exit(0);
	return 0;
}


