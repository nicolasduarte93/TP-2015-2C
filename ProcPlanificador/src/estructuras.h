#ifndef SRC_ESTRUCTURAS_H_
#define SRC_ESTRUCTURAS_H_

#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/log.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <semaphore.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>

sem_t hayProgramas;
sem_t hayCPU;
sem_t hayIO;

t_queue* colaListos;
t_queue* colaIO;

t_list* listaEjecutando;
t_list* listaCpuLibres;
t_list* listaInicializando;
t_list* listaAfinalizar;
t_list* listaCpus;
t_list* listaPorcentajeCpus;

pthread_mutex_t mutexListaCpusLibres;
pthread_mutex_t mutexProcesoListo;
pthread_mutex_t mutexListaEjecutando;
pthread_mutex_t mutexIO;
pthread_mutex_t mutexInicializando;
pthread_mutex_t mutexSwitchProc;
pthread_mutex_t mutexFinalizarPid;
pthread_mutex_t mutexListasCpu;
pthread_mutex_t mutexListasPorcentajes;

t_log* logPlanificador;
time_t tiempoEjec;

typedef struct {
	int socket;
	t_list* listaCpus;
} tParametroSelector;

typedef struct {
	int tid;
	int porcentaje;
} tPorcentajeCpu;

typedef struct {
	char* puertoEscucha;
	char algoritmo;
	int quantum;
}tconfig_planif;

tconfig_planif *configPlanificador;

typedef enum {
	LISTO, IO, EJECUTANDO, FINALIZADO
}testado;

typedef struct {
	char* ruta;
	int pid;
	char* nombre;
	testado estado;
	int siguiente;
	int maximo;
	time_t llegada;
	time_t entroIO;
	time_t entroCPU;
	time_t tpoBloqueado;
	time_t tpoCPU;
}tpcb;

typedef struct{
	tpcb* pcb;
	int tiempo;
}tprocIO;

typedef struct {
	char tipoProceso;
	char tipoOperacion;
	testado estado;
	int pid;
	int counterProgram;
	int quantum;           //SI ES 0 EL ALGORITMO ES FIFO, SINO RR
	int tamanioMensaje;
	char* mensaje;//en este caso es la ruta del "mprod"
}__attribute__((packed)) protocolo_planificador_cpu;

#endif /* SRC_ESTRUCTURAS_H_ */
