#include <stdlib.h>
#include "estructuras.h"
#include <commons/collections/list.h>
#include <commons/log.h>
#include <stdbool.h>
#include <string.h>
#include "paquetes.h"
#include <sys/types.h>
#include <sys/socket.h>
#include "log_memoria.h"

char * crear_memoria(int cantidad_marcos, int tamanio_marcos) {
	return calloc(cantidad_marcos * tamanio_marcos, 1);
}

tabla_paginas * inicializar_tabla_de_paginas(int cantidad_maxima_marcos_por_proceso, int pid) {
	tabla_paginas * tabla = malloc(sizeof(tabla_paginas));
	tabla->list_pagina_direccion = list_create();
	tabla->pid = pid;
	tabla->pos_puntero = 0;
	tabla->paginas_accedidas = list_create();
	tabla->cant_fallos_paginas = 0;
	int i;
	for(i = 0 ; i < cantidad_maxima_marcos_por_proceso ; i++) {
		pagina_direccion *pagina = malloc(sizeof(pagina_direccion));
		pagina->en_uso = false;
		pagina->fue_modificado = false;
		pagina->nro_pagina = -1;
		pagina->nro_marco = -1;
		list_add(tabla->list_pagina_direccion, pagina);
	}
	return tabla;
}

void eliminar_tabla_de_proceso(int pid, t_list ** lista_tabla_de_paginas) {
	int i;
	int count = list_size(*lista_tabla_de_paginas);
	for(i = 0 ; i < count ; i++) {
		tabla_paginas *tabla = list_get(*lista_tabla_de_paginas, i);
		if(tabla->pid == pid) {
			list_destroy_and_destroy_elements(tabla->paginas_accedidas,free);
			list_destroy_and_destroy_elements(tabla->list_pagina_direccion, free);
			free(list_remove(*lista_tabla_de_paginas, i));
			break;
		}
	}
}

tabla_paginas * dame_la_tabla_de_paginas(int pid, t_list ** lista_tabla_de_paginas) {
	int i;
	for(i = 0 ; i < list_size(*lista_tabla_de_paginas) ; i++) {
		tabla_paginas *tabla = list_get(*lista_tabla_de_paginas, i);
		if(tabla->pid == pid)
			return tabla;
	}
	return 0;
}

int obtener_marco_pagina(tabla_paginas *tabla, int pagina, int es_clock) {
	int i;
	for(i = 0 ; i < list_size(tabla->list_pagina_direccion) ; i++) {
		pagina_direccion *pagina_aux = list_get(tabla->list_pagina_direccion, i);

		if (!es_clock){
			if(pagina_aux->en_uso && pagina_aux->nro_pagina == pagina)
				return pagina_aux->nro_marco;
		}
		else
		{
			if(pagina_aux->nro_pagina == pagina){
				pagina_aux->en_uso = 1;
				return pagina_aux->nro_marco;
			}
		}
	}
	return -1;
}

// en tabla de paginas
void poneme_en_uso_la_entrada(tabla_paginas *tabla, int pagina) {
	int i;
	for(i = 0 ; i < list_size(tabla->list_pagina_direccion) ; i++) {
		pagina_direccion *pagina_aux = list_get(tabla->list_pagina_direccion, i);
		if(pagina_aux->nro_pagina == pagina) {
			pagina_aux->en_uso = true;
			break;
		}
	}
}

void poneme_en_modificado_la_entrada(tabla_paginas *tabla, int pagina) {
	int i;
	for(i = 0 ; i < list_size(tabla->list_pagina_direccion) ; i++) {
		pagina_direccion *pagina_aux = list_get(tabla->list_pagina_direccion, i);
		if(pagina_aux->en_uso && pagina_aux->nro_pagina == pagina) {
			pagina_aux->fue_modificado = true;
			break;
		}
	}
}

bool estan_los_frames_ocupados(t_list *tabla_paginas, bool es_clock) {
	int i = 0;
	pagina_direccion *pagina = list_get(tabla_paginas, 0);
	while(pagina) {
		if (!es_clock){
			if(!pagina->en_uso)
				return false;
		}
		else
		{
			/*si es -1 la entrada en la tabla de paginas no esta cargada*/
			if (pagina->nro_pagina == -1)
				return false;
		}
		i++;
		pagina = list_get(tabla_paginas, i);
	}
	return true;
}

int dame_un_marco_libre(t_list *lista_tabla_de_paginas, int cantidad_marcos, bool es_clock) {
	int i, j, k;
	for(i = 0 ; i < cantidad_marcos ; i++) {
		bool libre = true;
		for(j = 0 ; j < list_size(lista_tabla_de_paginas) ; j++) {
			bool ocupado = false;
			tabla_paginas *tabla = list_get(lista_tabla_de_paginas, j);
			for(k = 0 ; k < list_size(tabla->list_pagina_direccion) ; k++) {
				pagina_direccion *pagina = list_get(tabla->list_pagina_direccion, k);
				if((pagina->nro_marco == i && pagina->en_uso) || (pagina->nro_marco == i  && es_clock)) {
					ocupado = true;
					libre = false;
					break;
				}
			}
			if (ocupado)
				break;
		}
		if(libre)
			return i;
	}
	return -1;
}

char * dame_mensaje_de_memoria(char **memoria, int nro_marco, int tamanio_marco) {
	char *mensaje_memoria = *memoria + nro_marco * tamanio_marco;
	char *mensaje = malloc(tamanio_marco);
	strcpy(mensaje, mensaje_memoria);
	return mensaje;
}

void avisar_a_cpu(char cod_op, char cod_aux, int pid, int paginas, char *mensaje, int socket_cli_cpu) {
	tprotocolo_memoria_cpu memoria_cpu;
	armar_estructura_protocolo_a_cpu(&memoria_cpu, cod_op, cod_aux, pid, paginas, mensaje);
	void * buffer = serializar_a_cpu(&memoria_cpu);
	send(socket_cli_cpu, buffer, memoria_cpu.tamanio_mensaje + 15, 0);
	free(buffer);
}

void avisar_a_swap(char cod_op, int pid, int paginas, char *mensaje, int socket_ser_swap) {
	tprotocolo_desde_cpu_y_hacia_swap paquete_a_swap;
	armar_estructura_desde_cpu_y_hacia_swap(&paquete_a_swap, cod_op, pid, paginas, mensaje);
	void * buffer = serializar_a_swap(&paquete_a_swap);
	send(socket_ser_swap, buffer, paquete_a_swap.tamanio_mensaje + 13, 0);
	free(buffer);
}

t_list * inicializar_tlb(int nro_entradas) {
	t_list * tlb = list_create();
	int i;
	for(i = 0 ; i < nro_entradas ; i++) {
		cache_13 *entrada = malloc(sizeof(cache_13));
		entrada->esta_en_uso = false;
		entrada->nro_marco = -1;
		entrada->pid = -1;
		list_add(tlb, entrada);
	}
	return tlb;
}

int dame_el_marco_de_la_pagina_en_la_tlb(t_list ** tlb, int pid, int nro_pagina) {
	int i;
	for(i = 0; i < list_size(*tlb); i++) {
		cache_13 * aux = list_get(*tlb, i);
		if(aux->pid == pid && aux->nro_pagina == nro_pagina)
			return aux->nro_marco;
	}
	return -1;
}

void eliminar_entrada_tlb(t_list ** tlb, int pid, int nro_pagina) {
	int i;
	for(i = 0; i < list_size(*tlb) ; i++) {
		cache_13 * aux = list_get(*tlb, i);
		if(aux->esta_en_uso && aux->pid == pid && aux->nro_pagina == nro_pagina) {
			free(list_remove(*tlb, i));
			cache_13 * new = malloc(sizeof(cache_13));
			new->esta_en_uso = false;
			new->nro_marco = -1;
			new->nro_pagina = -1;
			new->pid = -1;
			list_add(*tlb, new);
		}
	}
}

char actualizame_la_tlb(t_list ** tlb, int pid, int nro_marco, int nro_pagina) {
	int i;
	for(i = 0; i< list_size(*tlb); i++) {
		cache_13 * aux = list_get(*tlb, i);
		if(!aux->esta_en_uso) {
			aux->nro_marco = nro_marco;
			aux->esta_en_uso = true;
			aux->pid = pid;
			aux->nro_pagina = nro_pagina;
			return 'e'; // e = encontro una entrada
		}
	}
	// esta lleno, sacar por fifo
	free(list_remove(*tlb, 0));
	cache_13 * nueva_entrada = malloc(sizeof(cache_13));
	nueva_entrada->nro_marco = nro_marco;
	nueva_entrada->esta_en_uso = true;
	nueva_entrada->nro_pagina = nro_pagina;
	nueva_entrada->pid = pid;
	list_add(*tlb, nueva_entrada);
	return 'f'; // f = fifo
}

/*
void borrame_las_entradas_del_proceso(int pid, t_list ** tlb) {
	int i,j,k;
	t_list * entradas_a_borrar = list_create();
	int size = list_size(*tlb);
	for(i = 0; i < size ; i++) {
		cache_13 * aux = list_get(*tlb, i);
		if(aux->pid == pid && aux->esta_en_uso) {
			int * aux = malloc(sizeof(int));
			*aux = i;
			list_add(entradas_a_borrar, aux);
		}
	}
	int cant_entradas_a_borrar = list_size(entradas_a_borrar);
	for(j = 0; j < cant_entradas_a_borrar ; j++) {
		int * list = list_remove(entradas_a_borrar, list_size(entradas_a_borrar) - 1);
		free(list_remove(*tlb, *list));
		free(list);
	}
	for(k = 0 ; k < cant_entradas_a_borrar ; k++) {
		cache_13 * nueva_entrada = malloc(sizeof(cache_13));
		nueva_entrada->esta_en_uso = false;
		nueva_entrada->pid = -1;
		list_add(*tlb, nueva_entrada);
	}
}
*/
/*
void borrame_las_entradas_del_proceso(int pid, t_list ** tlb) {
	int i = 0, size = list_size(*tlb);
	while(i < size) {
		cache_13 * aux = list_get(*tlb, i);
		if(aux->pid == pid && aux->esta_en_uso) {
			list_remove(*tlb, i);
			cache_13 * nueva_entrada = malloc(sizeof(cache_13));
			nueva_entrada->pid = -1;
			nueva_entrada->esta_en_uso = false;
			list_add(*tlb, nueva_entrada);
		}
		else
			i++;
	}
}
*/

void borrame_las_entradas_del_proceso(int pid, t_list ** tlb) {
	int entradas_borradas = 0;
	int i;
	for (i = list_size(*tlb) - 1; i >= 0 ; i--){
		cache_13 * aux = list_get(*tlb, i);
		if(aux->pid == pid && aux->esta_en_uso){
			free(list_remove(*tlb, i));
			entradas_borradas++;
		}
	}
	for(i = 0 ; i < entradas_borradas ; i++) {
		cache_13 * nueva_entrada = malloc(sizeof(cache_13));
		nueva_entrada->esta_en_uso = false;
		nueva_entrada->pid = -1;
		list_add(*tlb, nueva_entrada);
	}
}

void limpiar_la_tlb(t_list ** tlb){
	int cant_entradas = list_size(*tlb);
	while(!list_is_empty(*tlb))
		free(list_remove(*tlb,0));
	*tlb = inicializar_tlb(cant_entradas);
}

void limpiar_memoria(t_list ** tabla_de_paginas, char * memoria, int tamanio_marco, int socket_swap) {
	int i;
	for(i = 0 ; i< list_size(*tabla_de_paginas) ; i++) {
		tabla_paginas * tabla = list_get(*tabla_de_paginas, i);
		int j;
		for (j = 0 ; j < list_size(tabla->list_pagina_direccion) ; j++) {
			pagina_direccion * pagina = list_get(tabla->list_pagina_direccion, j);
			if (pagina->nro_pagina != -1 && pagina->fue_modificado){
				char * mensaje = dame_mensaje_de_memoria(&memoria, pagina->nro_marco, tamanio_marco);
				avisar_a_swap('e',tabla->pid,pagina->nro_pagina,mensaje,socket_swap);
				free(mensaje);
			}
			pagina->en_uso = false;
			pagina->fue_modificado = false;
			pagina->nro_pagina = -1;
			pagina->nro_marco = -1;
		}
	}
}

void volcar_memoria(char * memoria, tconfig_memoria * config, t_log * logMem) {
	int buffer_memoria = 0;
	while (buffer_memoria < config->tamanio_marco * config->cantidad_marcos) {
		char * mensaje = malloc(config->tamanio_marco + 1);
		memcpy(mensaje, memoria + buffer_memoria, config->tamanio_marco);
		mensaje[config->tamanio_marco] = '\0';
		log_info(logMem, "Numero de marco %d: %s", buffer_memoria / config->tamanio_marco, mensaje);
		buffer_memoria += config->tamanio_marco;
	}
}

int dame_el_numero_de_entrada_de_la_tlb(t_list * tlb, int nro_marco) {
	int i;
	for(i = 0; i < list_size(tlb) ; i++) {
		cache_13 * aux = list_get(tlb, i);
		if(nro_marco == aux->nro_marco)
			return i;
	}
	return -1;
}

void aplicar_LRU(t_list **tabla_de_paginas, int nro_pagina) {
	int i;
	for(i = 0; i < list_size(*tabla_de_paginas) ; i++) {
		pagina_direccion * pagina = list_get(*tabla_de_paginas, i);
		if(pagina->nro_pagina == nro_pagina) {
			list_remove(*tabla_de_paginas, i);
			list_add(*tabla_de_paginas, pagina);
		}
	}
}

bool f_u_cero_m_cero(pagina_direccion * una_pagina) {
	return (una_pagina->en_uso == 0 && una_pagina->fue_modificado == 0);
}

bool f_u_cero_m_uno(pagina_direccion * una_pagina) {
	return (una_pagina->en_uso == 0 && una_pagina->fue_modificado == 1);
}

void reemplazar_pagina(int nro_pagina, int * nro_pagina_reemplazada, t_list * paginas, int pos, int * puntero, bool es_escritura) {

	pagina_direccion * pagina_objetivo = list_get(paginas, pos);
	if(!es_escritura) {
		pagina_objetivo->en_uso = 1;
		pagina_objetivo->fue_modificado = 0;
	}
	else {
		pagina_objetivo->en_uso = 1;
		pagina_objetivo->fue_modificado = 1;
	}

	*nro_pagina_reemplazada = pagina_objetivo->nro_pagina;
	pagina_objetivo->nro_pagina = nro_pagina;

	if (pos + 1 < list_size(paginas))
		(*puntero) = pos + 1;
	else
		(*puntero) = 0;
}

//si es true hay que llevar la pagina desalojada a la swap
bool aplicar_clock_modificado(int nro_pagina, int * nro_pagina_reemplazada, t_list * paginas, int * puntero, bool es_escritura){

	while(1){
		int i;
		/*recorro segunda mitad*/
		for(i = (*puntero); i < list_size(paginas); i++){
			if(f_u_cero_m_cero(list_get(paginas,i))){
				reemplazar_pagina(nro_pagina,nro_pagina_reemplazada,paginas,i,puntero,es_escritura);
				return false;
			}
		}
		/*recorro primera mitad*/
		for(i = 0; i < (*puntero); i++){
			if(f_u_cero_m_cero(list_get(paginas,i))){
				reemplazar_pagina(nro_pagina,nro_pagina_reemplazada,paginas,i,puntero,es_escritura);
				return false;
			}
		}

		/*recorro segunda mitad*/
		for(i = (*puntero); i < list_size(paginas); i++){
			if(f_u_cero_m_uno(list_get(paginas,i))){
				reemplazar_pagina(nro_pagina, nro_pagina_reemplazada,paginas,i,puntero,es_escritura);
				return true;
			}
			else
				((pagina_direccion*)list_get(paginas,i))->en_uso = false;
		}
		/*recorro primera mitad*/
		for(i = 0; i < (*puntero); i++){
			if(f_u_cero_m_uno(list_get(paginas,i))){
				reemplazar_pagina(nro_pagina, nro_pagina_reemplazada,paginas,i,puntero,es_escritura);
				return true;
			}
			else
				((pagina_direccion*)list_get(paginas,i))->en_uso = false;
		}
	}
}

void llevar_a_swap(int socket_Swap, char * memoria, pagina_direccion * pagina, int tamanio_marco, int pid){
	if (pagina->fue_modificado) {
		char * mensaje = dame_mensaje_de_memoria(&memoria, pagina->nro_marco, tamanio_marco);
		avisar_a_swap('e', pid, pagina->nro_pagina, mensaje, socket_Swap);
		free(mensaje);
	}
}

void traer_de_swap(int socket_Swap, char * memoria, int nro_marco, int nro_pagina, int tamanio_marco, int pid){
	avisar_a_swap('l', pid, nro_pagina, "vacio", socket_Swap);
	tprotocolo_swap_memoria swap_memoria;
	recibir_paquete_desde_swap(socket_Swap, &swap_memoria);
	//pasar la pagina desde el swap a la memoria
	memcpy(memoria + nro_marco * tamanio_marco, swap_memoria.mensaje, swap_memoria.tamanio);
	free(swap_memoria.mensaje);
}

void registrar_acceso(t_list ** lista, int num_pagina){
	int i;
	for (i = 0; i < list_size(*lista); i++){
		int *num = list_get(*lista,i);
		if (*num == num_pagina)
			return;
	}
	int * num = malloc(sizeof(int));
	*num = num_pagina;
	list_add(*lista,num);
}

bool hay_algun_marco_en_la_tabla_de_pagina(t_list * paginas){
	int i;
	for(i = 0; i < list_size(paginas); i++){
		pagina_direccion * pagina = list_get(paginas,i);
		if (pagina->nro_pagina != -1)
			return true;
	}
	return false;
}

void aplicar_algoritmos_a_la_tabla(tprotocolo_desde_cpu_y_hacia_swap paquete_desde_cpu, int socketClienteCPU,int socketClienteSWAP, tabla_paginas ** tabla_de_paginas, t_log * logMem,tconfig_memoria * config, t_list ** tlb,char ** memoria){
	if (config->algoritmo_reemplazo != 'C') {
		pagina_direccion *pagina_ocupada = list_remove(((tabla_paginas*)*tabla_de_paginas)->list_pagina_direccion, 0);
		pagina_direccion *pagina_nueva = malloc(sizeof(pagina_direccion));

		pagina_nueva->en_uso = true;
		pagina_nueva->fue_modificado = false;
		pagina_nueva->nro_pagina = paquete_desde_cpu.paginas;
		pagina_nueva->nro_marco = pagina_ocupada->nro_marco;

		char fifo = 'n';
		list_add(((tabla_paginas*)*tabla_de_paginas)->list_pagina_direccion, pagina_nueva);
		if(config->habilitadaTLB) {
			eliminar_entrada_tlb(tlb, paquete_desde_cpu.pid, pagina_ocupada->nro_pagina);
			fifo = actualizame_la_tlb(tlb, paquete_desde_cpu.pid, pagina_nueva->nro_marco, paquete_desde_cpu.paginas);
		}

		//la pagina ocupada la paso a la swap si esta modificada
		llevar_a_swap(socketClienteSWAP,*memoria,pagina_ocupada,config->tamanio_marco,paquete_desde_cpu.pid);

		traer_de_swap(socketClienteSWAP,*memoria,pagina_nueva->nro_marco,pagina_nueva->nro_pagina,config->tamanio_marco,paquete_desde_cpu.pid);

		char * operacion = fifo == 'n' ? "-" : (fifo == 'e' ? "encontro una entrada en la tlb" : "apĺico fifo en la tlb");
		int nro_tlb = dame_el_numero_de_entrada_de_la_tlb(((t_list*)*tlb), pagina_nueva->nro_marco);
		int nro_marco = obtener_marco_pagina(((tabla_paginas*)*tabla_de_paginas), paquete_desde_cpu.paginas,config->algoritmo_reemplazo == 'C');

		//avisar a la cpu
		if(paquete_desde_cpu.cod_op == 'l') {
			char * mensaje = dame_mensaje_de_memoria(memoria, nro_marco, config->tamanio_marco);
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
			memcpy(*memoria + nro_marco * config->tamanio_marco, paquete_desde_cpu.mensaje, paquete_desde_cpu.tamanio_mensaje);
			pagina_nueva->fue_modificado = true;
			avisar_a_cpu(paquete_desde_cpu.cod_op, 'i', paquete_desde_cpu.pid, paquete_desde_cpu.paginas, "nada", socketClienteCPU);
		}
		free(paquete_desde_cpu.mensaje);
		free(pagina_ocupada);
	}
	/*aplicamos clock*/
	else
	{
		int pagina_reemplazada = -1;
		bool llevarSwap = aplicar_clock_modificado(paquete_desde_cpu.paginas,&pagina_reemplazada,((tabla_paginas*)*tabla_de_paginas)->list_pagina_direccion,&(((tabla_paginas*)*tabla_de_paginas)->pos_puntero),paquete_desde_cpu.cod_op == 'e');

		int nro_marco = obtener_marco_pagina(((tabla_paginas*)*tabla_de_paginas), paquete_desde_cpu.paginas,config->algoritmo_reemplazo == 'C');

		char fifo = 'n';
		if(config->habilitadaTLB) {
			eliminar_entrada_tlb(tlb, paquete_desde_cpu.pid, pagina_reemplazada);
			fifo = actualizame_la_tlb(tlb, paquete_desde_cpu.pid, nro_marco, paquete_desde_cpu.paginas);
		}
		//la pagina ocupada la paso a la swap si esta modificada
		if (llevarSwap){
			pagina_direccion * pagina_ocupada = malloc(sizeof(pagina_direccion));
			pagina_ocupada->nro_pagina = pagina_reemplazada;
			pagina_ocupada->nro_marco = nro_marco;
			pagina_ocupada->fue_modificado = true;

			llevar_a_swap(socketClienteSWAP,*memoria,pagina_ocupada,config->tamanio_marco,paquete_desde_cpu.pid);
			free(pagina_ocupada);
		}

		traer_de_swap(socketClienteSWAP,*memoria,nro_marco,paquete_desde_cpu.paginas,config->tamanio_marco,paquete_desde_cpu.pid);

		char * operacion = fifo == 'n' ? "-" : (fifo == 'e' ? "encontro una entrada en la tlb" : "apĺico fifo en la tlb");
		int nro_tlb = dame_el_numero_de_entrada_de_la_tlb(((t_list*)*tlb), nro_marco);

		//avisar a la cpu
		if(paquete_desde_cpu.cod_op == 'l') {
			char * mensaje = dame_mensaje_de_memoria(memoria, nro_marco, config->tamanio_marco);
			pthread_mutex_lock(&mutexLog);
			log_lectura_escritura('l', operacion ,logMem, paquete_desde_cpu.pid, paquete_desde_cpu.paginas, nro_tlb, false, nro_marco, mensaje);
			pthread_mutex_unlock(&mutexLog);
			avisar_a_cpu(paquete_desde_cpu.cod_op, 'i', paquete_desde_cpu.pid, paquete_desde_cpu.paginas, mensaje, socketClienteCPU);
			free(mensaje);
		} else {
			pthread_mutex_lock(&mutexLog);
			log_lectura_escritura('e', operacion ,logMem, paquete_desde_cpu.pid, paquete_desde_cpu.paginas, nro_tlb, false, nro_marco, paquete_desde_cpu.mensaje);
			pthread_mutex_unlock(&mutexLog);
			memcpy(*memoria + nro_marco * config->tamanio_marco, paquete_desde_cpu.mensaje, paquete_desde_cpu.tamanio_mensaje);
			poneme_en_modificado_la_entrada(((tabla_paginas*)*tabla_de_paginas), paquete_desde_cpu.paginas);
			avisar_a_cpu(paquete_desde_cpu.cod_op, 'i', paquete_desde_cpu.pid, paquete_desde_cpu.paginas, "nada", socketClienteCPU);
		}
			free(paquete_desde_cpu.mensaje);
	}
}


