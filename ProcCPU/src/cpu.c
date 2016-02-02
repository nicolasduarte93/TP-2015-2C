#include <pthread.h>
#include "libSocket.h"
#include <commons/config.h>
#include <commons/process.h>
#include "estructuras.h"
#include "funcionesCPU.h"
#include "logueo.h"
#include "serializacion.h"
#include "config.h"
#include <commons/error.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

bool terminoPlanificador;
tipoConfiguracionCPU *config;

void * procesarInstruccion() {

	t_log *logCpu;
	protocolo_planificador_cpu* datosParaProcesar = malloc(sizeof(protocolo_planificador_cpu));
	protocolo_cpu_memoria* mensajeAMemoria = malloc( sizeof(protocolo_cpu_memoria));
	protocolo_memoria_cpu* mensajeDeMemoria = malloc(sizeof(protocolo_memoria_cpu));
	int tid = process_get_thread_id();
	int socketPlanifAux;
	int maximoInstruccion = (int)(60.0f/config->retardo);
	t_list * listaTiempos = list_create();

	// creacion de la instancia de log
	char* nombrelog = string_new();
	string_append_with_format(&nombrelog,"../src/logCPU%d",tid);
	string_append(&nombrelog,".txt");
	logCpu = log_create(nombrelog, "cpu.c", false, LOG_LEVEL_INFO);
	free(nombrelog);

	/*solo para que quede lindo cuando imprime*/
	pthread_mutex_lock(&mutexConectarPlanificador);
	printf("Conectando CPU %d al Planificador (%s : %s)... ", tid, config->ipPlanificador,config->puertoPlanificador);
	client_init(&socketPlanifAux, config->ipPlanificador, config->puertoPlanificador);
	printf("OK\n");
	pthread_mutex_unlock(&mutexConectarPlanificador);

	if (socketPlanifAux == -1)
		log_info(logCpu, "Fallo al conectar con Planificador");
	else
		log_info(logCpu, "Conectado al Planificador");

	if (socketMemoria == -1)
		log_info(logCpu, "CPU %d fallo al conectar con Memoria", tid);
	else
		log_info(logCpu, "CPU %d se conecto con Memoria", tid);

	int status;
	while (1) {

		status = deserializarPlanificador(datosParaProcesar, socketPlanifAux);
		if(status <= 0) pthread_exit(0);
		if(datosParaProcesar->tipoOperacion == 'u'){

			/*tiempo actual*/
			time_t tiempoActual = time(NULL);

			int i;
			int cantidadInstruccionesLeidas = 0;

			for(i = 0; i < list_size(listaTiempos); i++){
				time_t * tiempoInstruccion = list_get(listaTiempos,i);

				if (tiempoActual - (*tiempoInstruccion) <= 60)
					cantidadInstruccionesLeidas++;
			}

			/*borro los tiempos de las isntrucciones que son anteriores al minuto*/
			for(i = list_size(listaTiempos) - 1; i >= 0; i--){
				time_t * tiempoInstruccion = list_get(listaTiempos,i);

				if (tiempoActual - (*tiempoInstruccion) > 60){
					free(list_remove(listaTiempos,i));
				}
			}

			datosParaProcesar->pid = tid;
			datosParaProcesar->counterProgram = ((int)(((float)cantidadInstruccionesLeidas/(float)maximoInstruccion) * 100.0f));
			enviarAPlanificador(datosParaProcesar,socketPlanifAux);
		}
		else{
			logueoRecepcionDePlanif(datosParaProcesar,tid,logCpu);

			FILE* archivo = fopen(datosParaProcesar->mensaje, "r+");

			if (archivo == NULL){
				printf("Error al abrir mCod: %s\n", datosParaProcesar->mensaje);
				break;
			}

			fseek(archivo, 0, SEEK_END);
			int tamanio = ftell(archivo);
			fseek(archivo, 0, SEEK_SET);
			char* lineaLeida = malloc(tamanio);
			int quantum = 0;

			while ((!feof(archivo) && (quantum < datosParaProcesar->quantum || datosParaProcesar->quantum == 0))) {

				//calcularTamanioDeLinea(archivo,&tamanio);
				char* instruccionLeida = leerInstruccion(&(datosParaProcesar->counterProgram), lineaLeida, archivo,tamanio);
				char** linea = string_split(instruccionLeida, ";");
				printf("pid-> %d ejecutar %s\n", datosParaProcesar->pid, *linea);
				free(*linea);

				/*si no "entiende" la instruccion pasa a la siguiente*/
				if (!interpretarInstruccion(instruccionLeida, datosParaProcesar, mensajeAMemoria, socketPlanifAux,logCpu))
					continue;

				if (datosParaProcesar->tipoOperacion == 'e') break;

				/*asi la comunicacion con la memoria es atomica*/
				pthread_mutex_lock(&mutex);
				enviarAMemoria(mensajeAMemoria);
				deserializarMemoria(mensajeDeMemoria, socketMemoria);
				pthread_mutex_unlock(&mutex);
				loguearEstadoMemoria(mensajeDeMemoria,instruccionLeida,logCpu);

				switch (mensajeDeMemoria->codOperacion){

					case 'f': {
						actualizarOperacionPaquetePlanificador(datosParaProcesar, 'f');
						enviarAPlanificador(datosParaProcesar,socketPlanifAux);

						/*me voy al eof para salir del while*/
						while(getc(archivo) != EOF);
						continue;
					}
					break;

					case 'i':{
						/*para saber desde el planifciador si el fallo es por inicializacion*/
						datosParaProcesar->tipoProceso = 'i';
						actualizarOperacionPaquetePlanificador(datosParaProcesar, mensajeDeMemoria->codAux);
						enviarAPlanificador(datosParaProcesar,socketPlanifAux);

						if (mensajeDeMemoria->codAux == 'a'){

							/*me voy al eof para salir del while*/
							while(getc(archivo) != EOF);
							continue;
						}
					}
					break;

					case 'l':
					case 'e':{
						if (mensajeDeMemoria->codAux == 'a'){
							/*para saber desde el planifciador si el fallo es por lectura o escritura*/
							datosParaProcesar->tipoProceso = mensajeDeMemoria->codOperacion;
							actualizarOperacionPaquetePlanificador(datosParaProcesar, 'a');
							enviarAPlanificador(datosParaProcesar,socketPlanifAux);
							datosParaProcesar->tipoOperacion = 'f';

							/*me voy al eof para salir del while*/
							while(getc(archivo) != EOF);
							continue;
						}
					}
					break;
				}
				sleep(config->retardo);

				/*guardo tiempo en el que se ejecuto la instruccion*/
				time_t * tiempoInstruccion = malloc(sizeof(time_t));
				*tiempoInstruccion = time(NULL);
				list_add(listaTiempos, tiempoInstruccion);

				/*incremento quatum solo para operaciones leer y escribir*/
				if (mensajeDeMemoria->codOperacion != 'i' && mensajeDeMemoria->codOperacion != 'f')
					quantum++;

			}

			/*es rr y salio por quantum y no por io*/
			if (datosParaProcesar->quantum != 0) {
				if (datosParaProcesar->quantum == quantum){
					if (datosParaProcesar->tipoOperacion != 'e' && datosParaProcesar->tipoOperacion != 'f'){
						actualizarOperacionPaquetePlanificador(datosParaProcesar, 'q');
						enviarAPlanificador(datosParaProcesar, socketPlanifAux);
						printf("pid-> %d salio por quantum\n", datosParaProcesar->pid);
					}
				}
			}
			free(lineaLeida);
			fclose(archivo);
		}
	}
	free(mensajeAMemoria);
	free(mensajeDeMemoria);
	free(datosParaProcesar);
	close(socketPlanifAux);
	log_info(logCpu, "Cerrada conexion saliente");
	log_destroy(logCpu);
	return 0;
}

int main() {
	system("clear");
	config = leerConfiguracion();

	//Inicia el Socket para conectarse con la Memoria
	printf("Conectando a la Memoria (%s : %s)... ", config->ipMemoria,config->puertoMemoria);
	client_init(&socketMemoria, config->ipMemoria, config->puertoMemoria);
	printf("OK\n");

	pthread_t vectorHilos[config->cantidadHilos];
	pthread_attr_t atrib;
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&mutexConectarPlanificador,NULL);
	pthread_attr_init(&atrib);

	int i;
	for (i = 0; i < config->cantidadHilos; i++) {
		pthread_create(&vectorHilos[i], &atrib, procesarInstruccion, NULL);
	}

	for (i = 0; i < config->cantidadHilos; i++) {
		pthread_join(vectorHilos[i], NULL);
	}

	printf("Finalizo el planificador...\n");
	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&mutexConectarPlanificador);
	return 0;
}

