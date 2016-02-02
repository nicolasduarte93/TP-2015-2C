#include <sys/types.h>
#include <sys/socket.h>
#include "libSocket.h"
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include "estructuras.h"
#include <string.h>
#include "paquetes.h"
#include "configuracion.h"
#include <stdbool.h>
#include <unistd.h>
#include "funciones_memoria.h"
#include <signal.h>
#include "log_memoria.h"
#include <ctype.h>
#include <semaphore.h>

t_list * tlb;
t_list * lista_tabla_de_paginas;
t_queue * colaSeniales;
sem_t haySenial;
tconfig_memoria * config;
t_log * logMem;
char * memoria;
int socketClienteSWAP;
pthread_mutex_t mutex;
pthread_mutex_t mutexLog;
int cant_aciertos_tlb = 0;
int cant_accedidas_tlb = 0;

void * sig_handler(){
	while (true){
		sem_wait(&haySenial);
		int * signal = queue_pop(colaSeniales);
		switch(*signal){
			//borrar tlb
			case SIGUSR1:
				pthread_mutex_lock(&mutex);
					printf("borrar tlb...\n");
					pthread_mutex_lock(&mutexLog);
					log_info(logMem, "SIGUSR1, limpiar tlb\n");
					pthread_mutex_unlock(&mutexLog);
					limpiar_la_tlb(&tlb);
				pthread_mutex_unlock(&mutex);
			break;
			//borrar memoria
			case SIGUSR2:
				pthread_mutex_lock(&mutex);
					printf("borrar memoria...\n");
					pthread_mutex_lock(&mutexLog);
					log_info(logMem, "SIGUSR2, limpiar memoria y tlb\n");
					pthread_mutex_unlock(&mutexLog);
					limpiar_la_tlb(&tlb);
					limpiar_memoria(&lista_tabla_de_paginas,memoria,config->tamanio_marco, socketClienteSWAP);
				pthread_mutex_unlock(&mutex);
			break;
			//volcar, en la consola es SIGIO (kill -SIGIO pid) buscar los pid con ps -ef
			case SIGPOLL:
				pthread_mutex_lock(&mutex);
					printf("volcado de memoria...\n");
					pthread_mutex_lock(&mutexLog);
					log_info(logMem, "SIGPOLL, volcar memoria\n");
					pthread_mutex_unlock(&mutexLog);
					volcar_memoria(memoria, config, logMem);
				pthread_mutex_unlock(&mutex);
			break;
		}
		free(signal);
	}
}

void cola_seniales(int signalNum){
	int * signal = malloc(sizeof(int));
	*signal = signalNum;
	queue_push(colaSeniales,signal);
	sem_post(&haySenial);
}

void * mostrar_tasa_tlb(){
	while (1){
		sleep(5);
		pthread_mutex_lock(&mutexLog);
		log_info(logMem,"tasa acierto en la tlb: %f\n", (cant_aciertos_tlb*100.0f)/cant_accedidas_tlb);
		pthread_mutex_unlock(&mutexLog);
	}
}

int main(void) {
	system("clear");
	config = leerConfiguracion();
	logMem = log_create("log.txt", "memoria.c", false, LOG_LEVEL_INFO);

	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&mutexLog, NULL);
	sem_init(&haySenial,0,0);

	colaSeniales = queue_create();

	pthread_t tasa_tlb;
	pthread_t hilo_seniales;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	/*creacion de hilos*/
	pthread_create(&tasa_tlb,&attr,mostrar_tasa_tlb,NULL);
	pthread_create(&hilo_seniales,&attr,sig_handler,NULL);

	//Definimos datos Cliente listener
	int socketServidorCPU, socketClienteCPU;

	client_init(&socketClienteSWAP, config->ipSwap, config->puertoSwap);
	//Definimos datos Server
	server_init(&socketServidorCPU, config->puertoEscucha);
	printf("Memoria lista...\n");
	server_acept(socketServidorCPU, &socketClienteCPU);
	printf("CPU aceptado...\n");

	signal(SIGUSR1, cola_seniales);
	signal(SIGUSR2, cola_seniales);
	signal(SIGPOLL, cola_seniales);
	//creamos la representacion memoria///////////////////////////////////////////////////////////////////////////////////////
	memoria = crear_memoria(config->cantidad_marcos, config->tamanio_marco);
	lista_tabla_de_paginas = list_create();
	// creamos la tlb cache_13, si es que en el archivo de configuracion este esta activado///////////////////////////////////
	if(config->habilitadaTLB)
		tlb = inicializar_tlb(config->entradasTLB);

	tprotocolo_desde_cpu_y_hacia_swap paquete_desde_cpu;

	while(recibir_paquete_desde_cpu(&socketClienteCPU, &paquete_desde_cpu)) {
		sleep(config->retardoMemoria);
		printf("pid-> %d operacion %c\n", paquete_desde_cpu.pid, toupper(paquete_desde_cpu.cod_op));

		switch (paquete_desde_cpu.cod_op) {
			case 'i': {
				pthread_mutex_lock(&mutex);
				void* buffer = serializar_a_swap(&paquete_desde_cpu);
				send(socketClienteSWAP, buffer, paquete_desde_cpu.tamanio_mensaje + 13, 0);
				free(buffer);
				pthread_mutex_lock(&mutexLog);
				log_inicializar(logMem, paquete_desde_cpu.pid, paquete_desde_cpu.paginas);
				pthread_mutex_unlock(&mutexLog);
				tprotocolo_swap_memoria swap_memoria;
				recibir_paquete_desde_swap(socketClienteSWAP, &swap_memoria);

				if(swap_memoria.codAux != 'a')
					list_add(lista_tabla_de_paginas, inicializar_tabla_de_paginas(config->maximoMarcosPorProceso, paquete_desde_cpu.pid));

				avisar_a_cpu(paquete_desde_cpu.cod_op, swap_memoria.codAux, swap_memoria.pid, paquete_desde_cpu.paginas, swap_memoria.mensaje, socketClienteCPU);
				free(paquete_desde_cpu.mensaje);
				free(swap_memoria.mensaje);
				pthread_mutex_unlock(&mutex);
			}
			break;

			case 'f': {
				pthread_mutex_lock(&mutex);
				void* buffer = serializar_a_swap(&paquete_desde_cpu);
				send(socketClienteSWAP, buffer, paquete_desde_cpu.tamanio_mensaje + 13, 0);
				free(buffer);

				tabla_paginas *tabla = dame_la_tabla_de_paginas(paquete_desde_cpu.pid, &lista_tabla_de_paginas);
				pthread_mutex_lock(&mutexLog);
				log_info(logMem, "proceso finalizado -> pid: %d, fallos de paginas: %d, paginas accedidas: %d\n", paquete_desde_cpu.pid,tabla->cant_fallos_paginas,list_size(tabla->paginas_accedidas));
				pthread_mutex_unlock(&mutexLog);

				eliminar_tabla_de_proceso(paquete_desde_cpu.pid, &lista_tabla_de_paginas);
				if(config->habilitadaTLB)
					borrame_las_entradas_del_proceso(paquete_desde_cpu.pid, &tlb);

				avisar_a_cpu(paquete_desde_cpu.cod_op, 'i', paquete_desde_cpu.pid, paquete_desde_cpu.paginas, paquete_desde_cpu.mensaje, socketClienteCPU);
				free(paquete_desde_cpu.mensaje);
				pthread_mutex_unlock(&mutex);
			}
			break;

			case 'e':
			case 'l': {
				pthread_mutex_lock(&mutex);
				cant_accedidas_tlb++;
				//si la tlb esta activada
				int marco_en_tlb = -1;
				if(config->habilitadaTLB)
					marco_en_tlb = dame_el_marco_de_la_pagina_en_la_tlb(&tlb, paquete_desde_cpu.pid, paquete_desde_cpu.paginas);

				if(marco_en_tlb == -1) {//si la pagina no esta en la tlb
					tabla_paginas *tabla_de_paginas = dame_la_tabla_de_paginas(paquete_desde_cpu.pid, &lista_tabla_de_paginas);
					int nro_marco = obtener_marco_pagina(tabla_de_paginas, paquete_desde_cpu.paginas,config->algoritmo_reemplazo == 'C');

					if(nro_marco != -1) {//si la pagina esta en memoria
						int nro_tlb = -1;
						char fifo = 'n'; //n = no esta habilitada la tlb
						if(config->habilitadaTLB) {
							fifo = actualizame_la_tlb(&tlb, paquete_desde_cpu.pid, nro_marco, paquete_desde_cpu.paginas);
							nro_tlb = dame_el_numero_de_entrada_de_la_tlb(tlb, nro_marco);
						}

						pthread_mutex_lock(&mutexLog);
						log_acceso_memoria(logMem, paquete_desde_cpu.pid, paquete_desde_cpu.paginas, nro_marco);
						pthread_mutex_unlock(&mutexLog);

						if(config->algoritmo_reemplazo == 'L')
							aplicar_LRU(&(tabla_de_paginas->list_pagina_direccion), paquete_desde_cpu.paginas);

						if(paquete_desde_cpu.cod_op == 'l') {
							char * operacion = fifo == 'n' ? "-" : (fifo == 'e' ? "encontro una entrada en la tlb" : "apĺico fifo en la tlb");
							char * mensaje = dame_mensaje_de_memoria(&memoria, nro_marco, config->tamanio_marco);
							pthread_mutex_lock(&mutexLog);
							log_lectura_escritura('l', operacion ,logMem, paquete_desde_cpu.pid, paquete_desde_cpu.paginas, nro_tlb, false, nro_marco, mensaje);
							pthread_mutex_unlock(&mutexLog);
							avisar_a_cpu(paquete_desde_cpu.cod_op, 'i', paquete_desde_cpu.pid, paquete_desde_cpu.paginas, mensaje, socketClienteCPU);
							free(paquete_desde_cpu.mensaje);
							free(mensaje);
						}
						else{
							char * operacion = fifo == 'n' ? "-" : (fifo == 'e' ? "encontro una entrada en la tlb" : "apĺico fifo en la tlb");
							pthread_mutex_lock(&mutexLog);
							log_lectura_escritura('e', operacion ,logMem, paquete_desde_cpu.pid, paquete_desde_cpu.paginas, nro_tlb, false, nro_marco, paquete_desde_cpu.mensaje);
							pthread_mutex_unlock(&mutexLog);
							memcpy(memoria + nro_marco * config->tamanio_marco, paquete_desde_cpu.mensaje, paquete_desde_cpu.tamanio_mensaje);
							poneme_en_modificado_la_entrada(tabla_de_paginas, paquete_desde_cpu.paginas);
							avisar_a_cpu(paquete_desde_cpu.cod_op, 'i', paquete_desde_cpu.pid, paquete_desde_cpu.paginas, "nada", socketClienteCPU);
							free(paquete_desde_cpu.mensaje);
						}
					}
					else {//si no esta en la memoria
						pthread_mutex_lock(&mutexLog);
						log_acceso_a_swap(logMem, paquete_desde_cpu.pid, paquete_desde_cpu.paginas);
						pthread_mutex_unlock(&mutexLog);
						tabla_de_paginas->cant_fallos_paginas++;
						registrar_acceso(&(tabla_de_paginas->paginas_accedidas),paquete_desde_cpu.paginas);
						if(estan_los_frames_ocupados(tabla_de_paginas->list_pagina_direccion, config->algoritmo_reemplazo == 'C'))
							//sobre la tabla llena
							aplicar_algoritmos_a_la_tabla(paquete_desde_cpu,socketClienteCPU,socketClienteSWAP,&tabla_de_paginas,logMem,config,&tlb,&memoria);
						//la tabla no esta "llena"
						else {
							int nro_marco = dame_un_marco_libre(lista_tabla_de_paginas, config->cantidad_marcos,config->algoritmo_reemplazo == 'C');
							//si hay algun frame libre

							if(nro_marco != -1) {
								if (config->algoritmo_reemplazo != 'C'){
									/*asignar un marco libre, bucar en las tablas de paginas de cada proceso y si hay uno libre es porque
									no figura en ninguna tabla de pagina de los proceso*/
									int i;
									for(i = 0; i < list_size(tabla_de_paginas->list_pagina_direccion) ; i++) {
										pagina_direccion * pagina = list_get(tabla_de_paginas->list_pagina_direccion, i);
										if(!pagina->en_uso) {
											pagina->en_uso = true;
											pagina->fue_modificado = false;
											pagina->nro_pagina = paquete_desde_cpu.paginas;
											pagina->nro_marco = nro_marco;

											//aplicamos fifo y lru (al final estan las mas recientemente usadas)
											list_remove(tabla_de_paginas->list_pagina_direccion, i);
											list_add(tabla_de_paginas->list_pagina_direccion, pagina);

											char fifo = 'n';
											if(config->habilitadaTLB)
												fifo = actualizame_la_tlb(&tlb, paquete_desde_cpu.pid, pagina->nro_marco, paquete_desde_cpu.paginas);

											traer_de_swap(socketClienteSWAP,memoria,nro_marco,paquete_desde_cpu.paginas,config->tamanio_marco,paquete_desde_cpu.pid);

											char * operacion = fifo == 'n' ? "-" : (fifo == 'e' ? "encontro una entrada en la tlb" : "apĺico fifo en la tlb");
											int nro_tlb = dame_el_numero_de_entrada_de_la_tlb(tlb, pagina->nro_marco);

											//avisar a la cpu
											if(paquete_desde_cpu.cod_op == 'l') {
												char * mensaje = dame_mensaje_de_memoria(&memoria, nro_marco, config->tamanio_marco);
												pthread_mutex_lock(&mutexLog);
												log_lectura_escritura('l', operacion ,logMem, paquete_desde_cpu.pid, paquete_desde_cpu.paginas, nro_tlb, false, nro_marco, mensaje);
												pthread_mutex_unlock(&mutexLog);
												avisar_a_cpu(paquete_desde_cpu.cod_op, 'i', paquete_desde_cpu.pid, paquete_desde_cpu.paginas, mensaje, socketClienteCPU);
												free(mensaje);
											}
											else{
												pthread_mutex_lock(&mutexLog);
												log_lectura_escritura('e', operacion ,logMem, paquete_desde_cpu.pid, paquete_desde_cpu.paginas, nro_tlb, false, nro_marco, paquete_desde_cpu.mensaje);
												pthread_mutex_unlock(&mutexLog);
												memcpy(memoria + nro_marco * config->tamanio_marco, paquete_desde_cpu.mensaje, paquete_desde_cpu.tamanio_mensaje);
												pagina->fue_modificado = true;
												avisar_a_cpu(paquete_desde_cpu.cod_op, 'i', paquete_desde_cpu.pid, paquete_desde_cpu.paginas, "nada", socketClienteCPU);
											}
											free(paquete_desde_cpu.mensaje);
											//cuando la tabla ya esta libre corto con el break el for
											break;
										}
									}
								}
								/*aplico clock modificado*/
								else{
									pagina_direccion * pagina = list_get(tabla_de_paginas->list_pagina_direccion,tabla_de_paginas->pos_puntero);
									pagina->en_uso = true;
									pagina->nro_pagina = paquete_desde_cpu.paginas;
									pagina->nro_marco = nro_marco;
									pagina->fue_modificado = false;

									if (tabla_de_paginas->pos_puntero + 1 < list_size(tabla_de_paginas->list_pagina_direccion))
										tabla_de_paginas->pos_puntero++;
									else
										tabla_de_paginas->pos_puntero = 0;

									char fifo = 'n';
									if(config->habilitadaTLB)
										fifo = actualizame_la_tlb(&tlb, paquete_desde_cpu.pid, pagina->nro_marco, paquete_desde_cpu.paginas);

									traer_de_swap(socketClienteSWAP,memoria,nro_marco,paquete_desde_cpu.paginas,config->tamanio_marco,paquete_desde_cpu.pid);

									char * operacion = fifo == 'n' ? "-" : (fifo == 'e' ? "encontro una entrada en la tlb" : "apĺico fifo en la tlb");
									int nro_tlb = dame_el_numero_de_entrada_de_la_tlb(tlb, pagina->nro_marco);

									//avisar a la cpu
									if(paquete_desde_cpu.cod_op == 'l') {
										char * mensaje = dame_mensaje_de_memoria(&memoria, nro_marco, config->tamanio_marco);
										pthread_mutex_lock(&mutexLog);
										log_lectura_escritura('l', operacion ,logMem, paquete_desde_cpu.pid, paquete_desde_cpu.paginas, nro_tlb, false, nro_marco, mensaje);
										pthread_mutex_unlock(&mutexLog);
										avisar_a_cpu(paquete_desde_cpu.cod_op, 'i', paquete_desde_cpu.pid, paquete_desde_cpu.paginas, mensaje, socketClienteCPU);
										free(mensaje);
									}else{
										pthread_mutex_lock(&mutexLog);
										log_lectura_escritura('e', operacion ,logMem, paquete_desde_cpu.pid, paquete_desde_cpu.paginas, nro_tlb, false, nro_marco, paquete_desde_cpu.mensaje);
										pthread_mutex_unlock(&mutexLog);
										memcpy(memoria + nro_marco * config->tamanio_marco, paquete_desde_cpu.mensaje, paquete_desde_cpu.tamanio_mensaje);
										pagina->fue_modificado = true;
										avisar_a_cpu(paquete_desde_cpu.cod_op, 'i', paquete_desde_cpu.pid, paquete_desde_cpu.paginas, "nada", socketClienteCPU);
									}
									free(paquete_desde_cpu.mensaje);
								}
							} else {

								//podes aplicar el algoritmo sobre los marcos de la tabla de pagina(que no esta llena)
								if (hay_algun_marco_en_la_tabla_de_pagina(tabla_de_paginas->list_pagina_direccion)){

									tabla_paginas * tabla_de_paginas_aux = malloc(sizeof(tabla_paginas));
									tabla_de_paginas_aux->pid = tabla_de_paginas->pid;
									tabla_de_paginas_aux->pos_puntero = tabla_de_paginas->pos_puntero;
									tabla_de_paginas_aux->cant_fallos_paginas = tabla_de_paginas->cant_fallos_paginas;
									tabla_de_paginas_aux->paginas_accedidas = tabla_de_paginas->paginas_accedidas;
									tabla_de_paginas_aux->list_pagina_direccion = list_create();

									int i;
									for (i = 0; i < list_size(tabla_de_paginas->list_pagina_direccion);i++ ) {
										pagina_direccion * pagina = list_get(tabla_de_paginas->list_pagina_direccion, i);
										if (pagina->nro_pagina != -1){
											list_add(tabla_de_paginas_aux->list_pagina_direccion,pagina);
										}
									}

									//aplico sobre tabla auxiliar
									aplicar_algoritmos_a_la_tabla(paquete_desde_cpu,socketClienteCPU,socketClienteSWAP,&tabla_de_paginas_aux,logMem,config,&tlb,&memoria);

									int cantidad_paginas_vacias = list_size(tabla_de_paginas->list_pagina_direccion) - list_size(tabla_de_paginas_aux->list_pagina_direccion);

									for(i = 0 ; i < cantidad_paginas_vacias ; i++ ) {
										pagina_direccion * pagina = malloc(sizeof(pagina_direccion));
										pagina->en_uso = false;
										pagina->fue_modificado = false;
										pagina->nro_pagina = -1;
										pagina->nro_marco = -1;
										list_add(tabla_de_paginas_aux->list_pagina_direccion, pagina);
									}

									list_destroy(tabla_de_paginas->list_pagina_direccion);
									tabla_de_paginas->list_pagina_direccion = tabla_de_paginas_aux->list_pagina_direccion;

									free(tabla_de_paginas_aux);
								}
								//es como finalizar
								else {

									void* buffer = serializar_a_swap(&paquete_desde_cpu);
									send(socketClienteSWAP, buffer, paquete_desde_cpu.tamanio_mensaje + 13, 0);
									free(buffer);

									tabla_paginas *tabla = dame_la_tabla_de_paginas(paquete_desde_cpu.pid, &lista_tabla_de_paginas);
									pthread_mutex_lock(&mutexLog);
									log_info(logMem, "proceso finalizado -> pid: %d, fallos de paginas: %d, paginas accedidas: %d\n", paquete_desde_cpu.pid,tabla->cant_fallos_paginas,list_size(tabla->paginas_accedidas));
									pthread_mutex_unlock(&mutexLog);

									eliminar_tabla_de_proceso(paquete_desde_cpu.pid, &lista_tabla_de_paginas);
									if(config->habilitadaTLB)
										borrame_las_entradas_del_proceso(paquete_desde_cpu.pid, &tlb);

									//avisale a la cpu que no hay mas memoria y no se puede agregar una nueva entrada de paginas
									avisar_a_cpu(paquete_desde_cpu.cod_op, 'a', paquete_desde_cpu.pid, paquete_desde_cpu.paginas, "fallo", socketClienteCPU);
									free(paquete_desde_cpu.mensaje);
								}
							}
						}
					}
				} else {//si esta en la tlb

					cant_aciertos_tlb++;
					int nro_tlb = dame_el_numero_de_entrada_de_la_tlb(tlb, marco_en_tlb);

					tabla_paginas *tabla_de_paginas = dame_la_tabla_de_paginas(paquete_desde_cpu.pid, &lista_tabla_de_paginas);
					if(config->algoritmo_reemplazo == 'L')
						aplicar_LRU(&(tabla_de_paginas->list_pagina_direccion), paquete_desde_cpu.paginas);

					pthread_mutex_lock(&mutexLog);
					log_acceso_memoria(logMem, paquete_desde_cpu.pid, paquete_desde_cpu.paginas, marco_en_tlb);
					pthread_mutex_unlock(&mutexLog);

					if(config->algoritmo_reemplazo == 'C')
						poneme_en_uso_la_entrada(tabla_de_paginas, paquete_desde_cpu.paginas);

					if(paquete_desde_cpu.cod_op == 'l') {
						char * mensaje = dame_mensaje_de_memoria(&memoria, marco_en_tlb, config->tamanio_marco);
						pthread_mutex_lock(&mutexLog);
						log_lectura_escritura('l', "-", logMem, paquete_desde_cpu.pid, paquete_desde_cpu.paginas, nro_tlb, true,marco_en_tlb, mensaje);
						pthread_mutex_unlock(&mutexLog);
						avisar_a_cpu(paquete_desde_cpu.cod_op, 'i', paquete_desde_cpu.pid, paquete_desde_cpu.paginas, mensaje, socketClienteCPU);
						free(mensaje);
					}
					else{
						memcpy(memoria + marco_en_tlb * config->tamanio_marco, paquete_desde_cpu.mensaje, paquete_desde_cpu.tamanio_mensaje);
						tabla_paginas * tabla_de_paginas = dame_la_tabla_de_paginas(paquete_desde_cpu.pid, &lista_tabla_de_paginas);
						pthread_mutex_lock(&mutexLog);
						log_lectura_escritura('e', "-", logMem, paquete_desde_cpu.pid, paquete_desde_cpu.paginas, nro_tlb, true, marco_en_tlb, paquete_desde_cpu.mensaje);
						pthread_mutex_unlock(&mutexLog);
						poneme_en_modificado_la_entrada(tabla_de_paginas, paquete_desde_cpu.paginas);
						avisar_a_cpu(paquete_desde_cpu.cod_op, 'i', paquete_desde_cpu.pid, paquete_desde_cpu.paginas, "nada", socketClienteCPU);
					}
					free(paquete_desde_cpu.mensaje);
				}
				pthread_mutex_unlock(&mutex);
			}
			break;
		}
	}

	close(socketClienteSWAP);
	close(socketClienteCPU);
	close(socketServidorCPU);
	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&mutexLog);
	queue_destroy(colaSeniales);
	sem_destroy(&haySenial);
	return 0;
	// la lista esta alreves, list_add te agrega al final, y en fifo lo que sacamos esta al principio, lru mandamos al final
}
