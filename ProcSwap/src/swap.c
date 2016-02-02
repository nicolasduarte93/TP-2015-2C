#include <sys/types.h>
#include <sys/socket.h>
#include "libSocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include "estructuras.h"
#include "manejo_listas_archivo_swap.h"
#include <stdbool.h>
#include "config.h"
#include <string.h>
#include "paquetes.h"
#include <unistd.h>
#include <ctype.h>

int main(void) {
	system("clear");
	t_log *logSwap = log_create("log.txt", "swap.c", false, LOG_LEVEL_INFO);
 	tconfig_swap* config_swap = leerConfiguracion();
	t_list * lista_ocupado = list_create();
	t_list * lista_vacia = list_create();
	t_list * lista_procesado = list_create();
	FILE *swap = iniciar_archivo_swap();

	tlista_vacio *vacio = malloc(sizeof(tlista_vacio));
	vacio->comienzo = 0;
	vacio->paginas_vacias = config_swap->cantidadPaginas;
	list_add(lista_vacia, vacio);

	//Crea el socket para escuchar
	int server_socket;
	server_init(&server_socket, config_swap->puertoEscucha);
	printf("SWAP listo...\n");

	//Crea el socket para recibir a la memoria
	int socket_memoria;
	server_acept(server_socket, &socket_memoria);
	printf("Memoria aceptada...\n");

	tprotocolo_memoria_swap paquete_de_memoria;
	while(recibir_paquete_desde_memoria(&socket_memoria, &paquete_de_memoria)) {
		sleep(config_swap->retardo_swap);
		printf("pid-> %d operacion %c\n", paquete_de_memoria.pid, toupper(paquete_de_memoria.codigo_op));
		switch(paquete_de_memoria.codigo_op){
			case 'i': {
				int comienzo = -1;
				char cod_aux = 'i';
				/*reservamos un pedaso de memoria si es posible*/
				if (!dame_si_hay_espacio(&lista_vacia, paquete_de_memoria.cantidad_pagina, &comienzo)){
					/*el espacio total libre es mayor a lo pedido*/
					if (espacio_total_disponible(lista_vacia) >= paquete_de_memoria.cantidad_pagina) {
						//compactamos, y retorno el comienzo del espacio vacio
						log_info(logSwap,"compactacion iniciada /n");
						printf("compactacion...\n");
						sleep(config_swap->retardo);
						compactar_swap(&swap, &lista_vacia, &lista_ocupado, config_swap->tamanioPagina, config_swap->cantidadPaginas);
						log_info(logSwap,"compactacion finalizada /n");
						/*reservo*/
						dame_si_hay_espacio(&lista_vacia, paquete_de_memoria.cantidad_pagina, &comienzo);
					}
					else {
						cod_aux = 'a';
						log_proc_rechazado(logSwap, paquete_de_memoria.pid);
					}
				}
				if (cod_aux == 'i')
					asignar_espacio(paquete_de_memoria.pid, comienzo, paquete_de_memoria.cantidad_pagina, &lista_ocupado, &logSwap, config_swap->tamanioPagina);

				/*se avisa sobre el resultado*/
				avisar_a_memoria(cod_aux, paquete_de_memoria.pid, "-", socket_memoria);
			}
			break;

			case 'f': {
				int i;
				for (i = 0; i < list_size(lista_ocupado); i++) {
					tlista_ocupado * espacio_ocupado = list_get(lista_ocupado, i);
					if (espacio_ocupado->pid == paquete_de_memoria.pid) {
						//agrego a la lista vacia el espacio que voy a liberar
						tlista_vacio * espacio_vacio = malloc(sizeof(tlista_vacio));
						espacio_vacio->comienzo = espacio_ocupado->comienzo;
						espacio_vacio->paginas_vacias = espacio_ocupado->paginas_ocupadas;
						list_add(lista_vacia,espacio_vacio);

						//saco espacio de lista ocupado
						log_finalizar(logSwap,lista_procesado, espacio_ocupado->pid, espacio_ocupado->paginas_ocupadas, config_swap->tamanioPagina);
						free(list_remove(lista_ocupado, i));
						arreglar_lista_vacia(&lista_vacia);
					}
				}
			}
			break;

			case 'l': {
				int pag_inicio = get_comienzo_espacio_asignado(lista_ocupado, paquete_de_memoria.pid);
				//indica la pagina a leer
				int pag_leer = paquete_de_memoria.cantidad_pagina;

				//me posiciono sobre la pagina a leer
				int desplazamiento_en_bytes = (pag_inicio + pag_leer) * config_swap->tamanioPagina;
				fseek(swap, desplazamiento_en_bytes, SEEK_SET);

				char * lectura_pagina = malloc(config_swap->tamanioPagina);
				fread(lectura_pagina, config_swap->tamanioPagina, 1, swap);

				log_lectura(logSwap, paquete_de_memoria.pid, pag_inicio,config_swap->tamanioPagina, pag_leer, lectura_pagina);
				registrarOperacion(&lista_procesado,paquete_de_memoria.pid,pag_leer,true);

				avisar_a_memoria('i', paquete_de_memoria.pid, lectura_pagina, socket_memoria);
				free(lectura_pagina);
			}
			break;

			case 'e': {
				int pag_inicio = get_comienzo_espacio_asignado(lista_ocupado, paquete_de_memoria.pid);
				//indica la pagina a escribir
				int pagina_a_escribir = paquete_de_memoria.cantidad_pagina;

				//me posiciono sobre la pagina a escribir
				int desplazamiento_en_bytes = (pag_inicio + pagina_a_escribir) * config_swap->tamanioPagina;

				log_escritura(logSwap, paquete_de_memoria.pid, pag_inicio, config_swap->tamanioPagina, pagina_a_escribir,paquete_de_memoria.mensaje);
				registrarOperacion(&lista_procesado,paquete_de_memoria.pid,pagina_a_escribir,false);

				fseek(swap, desplazamiento_en_bytes, SEEK_SET);
				fwrite(paquete_de_memoria.mensaje, paquete_de_memoria.tamanio_mensaje, 1, swap);
			}
			break;
		}
		free(paquete_de_memoria.mensaje);
	}

	close(socket_memoria);
	close(server_socket);
	fclose(swap);
	free(config_swap);
	return 0;
}
