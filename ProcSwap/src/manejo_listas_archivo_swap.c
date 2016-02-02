#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include "estructuras.h"
#include "paquetes.h"
#include <string.h>
#include <commons/log.h>
#include "config.h"
#include <sys/types.h>
#include <sys/socket.h>

void reinicar_archivo_swap(FILE **swap, t_list **lista_ocupada) {
	fclose(*swap);
	*swap = iniciar_archivo_swap();
	list_destroy_and_destroy_elements(*lista_ocupada, free);
	*lista_ocupada = list_create();
}

int get_comienzo_espacio_asignado(t_list * lista_ocupado, int pid) {
	int i;
	for (i = 0; i < list_size(lista_ocupado); i++){
		tlista_ocupado * ocupado = list_get(lista_ocupado, i);
		if (ocupado->pid == pid)
			return ocupado->comienzo;
	}
	return -1; // si no encuentra el pid en la lista
}

int dame_si_hay_espacio(t_list** lista_vacia, int paginas_pedidas, int* comienzo) {
	int i;
	tlista_vacio *aux;
	for (i = 0 ; i < list_size(*lista_vacia) ; i++) {
		aux = list_get(*lista_vacia, i);
		if (aux->paginas_vacias >= paginas_pedidas) {
			*comienzo = aux->comienzo;
			if (aux->paginas_vacias - paginas_pedidas > 0) {
				//actualizar el hueco vacio si sigue existiendo un hueco
				aux->comienzo = paginas_pedidas + aux->comienzo;
				aux->paginas_vacias = aux->paginas_vacias - paginas_pedidas;
			}
			else
				free(list_remove(*lista_vacia, i));
			return 1;
		}
	}
	return  0;
}

int espacio_total_disponible(t_list* lista_vacia) {
	int tamanio = 0, i;
	tlista_vacio *aux;
	for (i = 0; i < list_size(lista_vacia); i++) {
		aux = list_get(lista_vacia, i);
		tamanio += aux->paginas_vacias;
	}
	return tamanio;
}

void compactar_lista_vacia(t_list **lista_vacia, FILE **swap, int tamanio_pagina ,int total_de_paginas) {
	list_destroy_and_destroy_elements(*lista_vacia, free);
	int comienzo = (int)ftell(*swap) / tamanio_pagina;
	tlista_vacio *vacio = malloc(sizeof(tlista_vacio));
	vacio->comienzo = comienzo;
	vacio->paginas_vacias = total_de_paginas - comienzo;
	*lista_vacia = list_create();
	list_add(*lista_vacia, vacio);
}

t_list *pasar_ocupada_a_lista_auxiliar(FILE **swap, t_list **lista_ocupada, int tamanio_pagina) {
	t_list *lista_aux = list_create();
	tlista_ocupado *elem;
	while (!list_is_empty(*lista_ocupada)) {
		elem = list_remove(*lista_ocupada, 0);
		tdatos_paginas *data = malloc(sizeof(tdatos_paginas));
		data->pid = elem->pid;
		data->tamanio = elem->paginas_ocupadas*tamanio_pagina; //en bytes
		data->buffer = (char*)malloc(data->tamanio); // puede haber igual o menor del tamanio
		// leemos los datos
		fseek(*swap, elem->comienzo*tamanio_pagina, SEEK_SET);
		fread(data->buffer, sizeof(char), tamanio_pagina*elem->paginas_ocupadas , *swap); // lee y guarda en buffer pero tiene ceros al final
		list_add(lista_aux, data);
		free(elem);
	}
	return lista_aux;
}

void compactar_swap(FILE ** swap, t_list** lista_vacia, t_list** lista_ocupada,int tamanio_pagina, int total_de_paginas) {
	t_list *lista_aux = pasar_ocupada_a_lista_auxiliar(swap, lista_ocupada, tamanio_pagina);
	reinicar_archivo_swap(swap, lista_ocupada);
	int cont_pagina = 0;
	while (!list_is_empty(lista_aux)) {
		tdatos_paginas *elem;
		elem = list_remove(lista_aux, 0);
		/*copiamos a la swap*/
		fwrite(elem->buffer, 1,elem->tamanio, *swap);
		/*actualizamos lista ocupada*/
		tlista_ocupado *elem_ocupada = malloc(sizeof (tlista_ocupado));
		elem_ocupada->pid = elem->pid;
		elem_ocupada->comienzo = cont_pagina;
		elem_ocupada->paginas_ocupadas = elem->tamanio/tamanio_pagina;
		cont_pagina += elem_ocupada->paginas_ocupadas;
		list_add(*lista_ocupada, elem_ocupada);
		free(elem->buffer);
		free(elem);
	}
	compactar_lista_vacia(lista_vacia, swap, tamanio_pagina ,total_de_paginas);
}

bool comparacion(void * a, void * b) {
	return ((tlista_vacio*)a)->comienzo < ((tlista_vacio*)b)->comienzo;
}

void posicion_ultimo_elemento_contiguo(t_list **lista_vacia, tlista_vacio* elem, int* paginas_vacias) {
	tlista_vacio * aux = list_get(*lista_vacia, 0);
	if (elem->comienzo + elem->paginas_vacias == aux->comienzo) {
		tlista_vacio *copia = malloc(sizeof(tlista_vacio));
		*copia = *aux;
		*paginas_vacias += copia->paginas_vacias;
		free(list_remove(*lista_vacia, 0));
		if(!list_is_empty(*lista_vacia))
			posicion_ultimo_elemento_contiguo(lista_vacia, copia, paginas_vacias);
		free(copia);
	}
}

void arreglar_lista_vacia(t_list ** lista_vacia) {
	list_sort(*lista_vacia, comparacion);
	t_list* list_aux = list_create();
	while(!list_is_empty(*lista_vacia)) {
		tlista_vacio *elem_uno = list_remove(*lista_vacia, 0);
		int paginas_vacias = elem_uno->paginas_vacias;
		if(!list_is_empty(*lista_vacia))
			posicion_ultimo_elemento_contiguo(lista_vacia, elem_uno, &paginas_vacias);
		tlista_vacio *elem_bloque_vacio = malloc(sizeof(tlista_vacio));
		elem_bloque_vacio->comienzo = elem_uno->comienzo;
		elem_bloque_vacio->paginas_vacias = paginas_vacias;
		list_add(list_aux, elem_bloque_vacio);
	}
	*lista_vacia = list_aux;
}

void asignar_espacio(int pid, int comienzo, int cantidad_pagina, t_list **lista_ocupado, t_log **log_swap, int tamanio_pagina) {
	//asignar el espacio solicitado
	tlista_ocupado *ocupado = malloc(sizeof(tlista_ocupado));
	ocupado->pid = pid;
	ocupado->comienzo = comienzo;
	ocupado->paginas_ocupadas = cantidad_pagina;
	list_add(*lista_ocupado, ocupado);
	log_inicializar(*log_swap, pid, comienzo, tamanio_pagina, cantidad_pagina);
}

void avisar_a_memoria(char cod_aux, int pid, char * pag_data, int socket_memoria) {
	tprotocolo_swap_memoria swap_memoria;
	armar_estructura_protocolo_a_memoria(&swap_memoria, cod_aux, pid, pag_data);
	void * buffer = serializar_a_memoria(&swap_memoria);
	send(socket_memoria, buffer, 9 + swap_memoria.tamanio, 0);
}

tpagina_procesada * buscar_pagina(t_list * lista_paginas_procesadas, int num_pagina, int pid){
	int i;
	for (i = 0; i < list_size(lista_paginas_procesadas); i++){
		tpagina_procesada * aux = list_get(lista_paginas_procesadas, i);
		if (aux->num_pagina == num_pagina && aux->pid == pid)
			return aux;
	}
	return 0;
}

void registrarOperacion(t_list ** lista_procesado, int pid, int num_pagina, bool es_lectura){

	tpagina_procesada * pag = buscar_pagina(*lista_procesado, num_pagina, pid);
	if (pag){
		if (es_lectura)
			pag->leida = true;
		else
			pag->escrita = true;
	}
	else{
		pag = malloc(sizeof(tpagina_procesada));
		pag->pid = pid;
		pag->num_pagina = num_pagina;
		pag->escrita = false;
		pag->leida = false;

		if (es_lectura)
			pag->leida = true;
		else
			pag->escrita = true;

		list_add(*lista_procesado, pag);
	}
}
