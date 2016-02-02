#include "estructuras.h"
#include <commons/string.h>
#include <semaphore.h>
#include <stdio.h>

void* serializarPaqueteCPU(protocolo_planificador_cpu* paquete, int* tamanio){ //malloc(1)
	//SERIALIZA SOLO CORRER
	size_t messageLength = strlen(paquete->mensaje);

	void* paqueteSerializado = malloc(sizeof(protocolo_planificador_cpu) + messageLength);
	int offset = 0;
	int size_to_send;

	size_to_send = sizeof(paquete->tipoProceso);
	memcpy(paqueteSerializado + offset, &(paquete->tipoProceso),size_to_send);
	offset += size_to_send;
	size_to_send = sizeof(paquete->tipoOperacion);
	memcpy(paqueteSerializado + offset, &(paquete->tipoOperacion),size_to_send);
	offset += size_to_send;
	size_to_send = sizeof(paquete->estado);
	memcpy(paqueteSerializado + offset, &(paquete->estado), size_to_send);
	offset += size_to_send;
	size_to_send = sizeof(paquete->pid);
	memcpy(paqueteSerializado + offset, &(paquete->pid), size_to_send);
	offset += size_to_send;
	size_to_send = sizeof(paquete->counterProgram);
	memcpy(paqueteSerializado + offset, &(paquete->counterProgram),size_to_send);
	offset += size_to_send;
	size_to_send = sizeof(paquete->quantum);
	memcpy(paqueteSerializado + offset, &(paquete->quantum), size_to_send);
	offset += size_to_send;
	size_to_send = sizeof(paquete->tamanioMensaje);
	memcpy(paqueteSerializado + offset, &messageLength, size_to_send);
	offset += size_to_send;
	size_to_send = messageLength;
	memcpy(paqueteSerializado + offset, paquete->mensaje, size_to_send);
	offset += size_to_send;
	*tamanio = offset;

	return paqueteSerializado;
}

int maxLineas(char* path){
	FILE* archivo = fopen(path, "r+");
	/*desde armarPCB me fijo si es distinto de -1*/
	if (!archivo) return -1;

	int cont = 1;
	int i;
	while (!feof(archivo)) {
		for(i=0; fgetc(archivo) != '\n' && !feof(archivo);i++);
		cont++;
	}
	fclose(archivo);

	return cont;
}

char* nombrePrograma(char* path){

	int tamanio = strlen(path);
	int i;

	for(i= tamanio; i>=0; i--){

		if(path[i]=='/') return path + i + 1;
	}


	return path;
}

tpcb * armarPCB(char* path, int cant) {
	tpcb* pcb = malloc(sizeof(tpcb));
	pcb->ruta = (char*) malloc(strlen(path)+1);
	strcpy(pcb->ruta, path);
	path[strlen(path)-1] = '\0';
	pcb->pid = cant;
	pcb->nombre = string_new();
	string_append(&pcb->nombre,nombrePrograma(path));
	pcb->estado = LISTO;
	pcb->siguiente = 1;
	pcb->maximo = maxLineas(path);
	if(pcb->maximo == -1){
		free(pcb);
		return 0;
	}
	pcb->llegada=time(NULL);
	pcb->tpoBloqueado=0;
	pcb->tpoCPU=0;
	pcb->entroCPU=0;
	pcb->entroIO=0;
	return pcb;
}

void convertirEstado(testado estadoEnum, char** estado){

	if (estadoEnum == LISTO)
		string_append(estado, "Listo");

	if (estadoEnum == IO)
		string_append(estado, "Bloqueado");

	if (estadoEnum == EJECUTANDO)
		string_append(estado, "Ejecutando");

	if (estadoEnum == FINALIZADO)
		string_append(estado, "Finalizado");
}

void mostrarEstadoProcesosLista(t_list* lista, char* mensaje){
	char* infoProceso = (char*)malloc(50);
	tpcb* pcb;
	int i;
	for(i = 0;i < list_size(lista); i++){
		char* estado = string_new();
		pcb=list_get(lista,i);
		convertirEstado(pcb->estado, &estado);
		strcpy(infoProceso, "mProc: ");
		string_append_with_format(&infoProceso, "%d ", pcb->pid);
		string_append(&infoProceso, pcb->nombre);
		string_append(&infoProceso, "-> ");
		string_append(&infoProceso,estado);
		string_append(&infoProceso,"\n");
        //Me fijo si estoy logueando o usando el ps
		if(!strcmp(mensaje,"logueo"))
			log_info(logPlanificador,infoProceso);
		else
			printf("%s",infoProceso);
		free(estado);
	}
    free(infoProceso);
}

int buscoPCB(int pidBuscado,t_list* lista){
	tpcb* pcb;
	int posicion = 0;
	int i;
	for(i = 0; i < list_size(lista); i++){
		pcb = list_get(lista,i);
		if(pcb->pid == pidBuscado)  return posicion;
		posicion++;
	}
	return -1;
}

void ponerPrimero(t_queue ** cola, tpcb * pcb){
	t_queue * aux = queue_create();

	queue_push(aux,pcb);
	/*paso a cola aux que ya tiene primero el pcb a terminar*/
	while(!queue_is_empty(*cola))
		queue_push(aux, queue_pop(*cola));
	/*vuelvo a pasar a la cola*/
	while(!queue_is_empty(aux))
		queue_push(*cola, queue_pop(aux));

	queue_destroy(aux);
}

bool hayQueFinalizarlo(int pid){

	bool r = false;
	int i;
	pthread_mutex_lock(&mutexFinalizarPid);
	for(i=0 ; i<list_size(listaAfinalizar); i++){
		int* aux = list_get(listaAfinalizar,i);
		if(*aux == pid){
			free(list_remove(listaAfinalizar,i));
			r = true;
		}
	}
	pthread_mutex_unlock(&mutexFinalizarPid);
    return r;
}

void finalizarPID(char* pidBuscado){
	t_list* lista= colaListos->elements;
	tpcb* pcb;
	int pid = atoi(pidBuscado);
	int posicion = buscoPCB(pid,lista);

	if(posicion == -1){
		int* aux = malloc(sizeof(int));
		*aux = pid;
		pthread_mutex_lock(&mutexFinalizarPid);
		list_add(listaAfinalizar,aux);
		pthread_mutex_unlock(&mutexFinalizarPid);
	}
	else {
		pcb = list_get(lista,posicion);
		if (pcb->siguiente != 1)
			pcb->siguiente = pcb->maximo;
		else{
			int* aux = malloc(sizeof(int));
			*aux = pid;
			pthread_mutex_lock(&mutexFinalizarPid);
			list_add(listaAfinalizar,aux);
			pthread_mutex_unlock(&mutexFinalizarPid);
		}
	}
}

int clasificarComando(char* message) {//OK
	if (!strcmp(message, "ps\n")) {
		return 1;
	} else {
		if (!strcmp(message, "cpu\n")) {
			return 2;
		} else {
			if (string_starts_with(message, "correr")) {
				return 3;
			} else {
				if (string_starts_with(message, "finalizar")) {
					return 4;
				} else {
					return 0;
				}
			}
		}
	}
}

bool comparadorPid(void * elemA, void * elemB){

	return ((tpcb*)elemA)->pid < ((tpcb*)elemB)->pid;
}

bool comparadorTid(void * elemA, void * elemB){

	return ((tPorcentajeCpu*)elemA)->tid < ((tPorcentajeCpu*)elemB)->tid;
}

void procesarComando(int nro_comando, char* message, int* cantProc) {//OK
	tpcb* pcb;
	switch (nro_comando){
		case 1:{
			pthread_mutex_lock(&mutexSwitchProc);

			/*lista auxiliar*/
			t_list * aux = list_create();
			int i;

			for(i = 0; i < list_size(listaEjecutando); i++)
				list_add(aux,list_get(listaEjecutando,i));

			for(i = 0; i < list_size(colaListos->elements); i++)
				list_add(aux,list_get(colaListos->elements,i));

			for(i = 0; i < list_size(colaIO->elements); i++)
				list_add(aux,((tprocIO*)list_get(colaIO->elements,i))->pcb);

			for(i = 0; i < list_size(listaInicializando); i++)
				list_add(aux,list_get(listaInicializando,i));

			/*ordeno por pid para luego mostrar*/
			list_sort(aux,comparadorPid);
			mostrarEstadoProcesosLista(aux,message);

			pthread_mutex_unlock(&mutexSwitchProc);
		}
			break;
		case 2:{
			int tamanio;
			protocolo_planificador_cpu* package = malloc(sizeof(protocolo_planificador_cpu));
			package->mensaje = malloc(2);
			strcpy(package->mensaje," ");
			package->tamanioMensaje = strlen(package->mensaje) +1;
			package->tipoOperacion = 'u';
			void* message = serializarPaqueteCPU(package, &tamanio);
			pthread_mutex_lock(&mutexListasPorcentajes);
			if (list_size(listaPorcentajeCpus) == 0){
				pthread_mutex_lock(&mutexListasCpu);
				int i;
				for(i = 0; i < list_size(listaCpus); i++){
					int *socketCPU = list_get(listaCpus, i);
					if(send(*socketCPU,message,tamanio,0) < 0)
						printf("fallo envio a cpu %d\n", *socketCPU);
				}
				pthread_mutex_unlock(&mutexListasCpu);
			}
			pthread_mutex_unlock(&mutexListasPorcentajes);
			free(package->mensaje);
			free(package);
			free(message);
		}
			break;

		case 3:
			pcb = armarPCB(&message[7], *cantProc);
			if (pcb){
				pthread_mutex_lock(&mutexProcesoListo);
				queue_push(colaListos, pcb);
				pthread_mutex_unlock(&mutexProcesoListo);
				(*cantProc) = (*cantProc)+ 1;
				sem_post(&hayProgramas);
			}
			else
				printf("No se encontro %s\n", &message[7]);
			break;
		case 4:
			pthread_mutex_lock(&mutexSwitchProc);//TODO este va??
			pthread_mutex_lock(&mutexProcesoListo);
			finalizarPID(&message[10]);
			pthread_mutex_unlock(&mutexProcesoListo);
			pthread_mutex_unlock(&mutexSwitchProc);
			break;
		default:
			printf("Comando ingresado incorrecto\n");
			break;
	}
}

int deserializarCPU(protocolo_planificador_cpu * package, int socketCPU) {
	void* buffer = malloc(sizeof(protocolo_planificador_cpu)-4);
	if (recv(socketCPU, buffer, sizeof(protocolo_planificador_cpu)-4, 0) <= 0) return 0;
	memcpy(&(package->tipoProceso), buffer, 1);
	memcpy(&(package->tipoOperacion), buffer + 1, 1);
	memcpy(&(package->estado), buffer + 2 ,4);
	memcpy(&(package->pid), buffer + 6 ,4);
	memcpy(&(package->counterProgram), buffer + 10 ,4);
	memcpy(&(package->quantum), buffer + 14, 4);
	memcpy(&(package->tamanioMensaje), buffer + 18, 4);
	// ahora el mensaje posta
	package->mensaje = (char*)malloc(package->tamanioMensaje + 1);
	if(recv(socketCPU, package->mensaje, package->tamanioMensaje, 0) <= 0) return 0;
	package->mensaje[package->tamanioMensaje] = '\0';
	free(buffer);
	return 1;
}

void adaptadorPCBaProtocolo(tpcb* pcb,protocolo_planificador_cpu* paquete){//OK
	paquete->tipoProceso = 'p';
	paquete->tipoOperacion = 'c';
	paquete->pid = pcb->pid;
	paquete->estado = pcb->estado;
	paquete->counterProgram = pcb->siguiente;
	if(configPlanificador->algoritmo == 'F') paquete->quantum = 0;
	else paquete->quantum = configPlanificador->quantum;
	paquete->tamanioMensaje = strlen(pcb->ruta)+1;
	paquete->mensaje =malloc(strlen(pcb->ruta)+1);
	strcpy(paquete->mensaje, pcb->ruta);
}




