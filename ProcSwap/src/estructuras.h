#ifndef SRC_ESTRUCTURAS_H_
#define SRC_ESTRUCTURAS_H_
#include <stdbool.h>

// estructura para leer el archivo de configuracion
typedef struct {
	char* puertoEscucha;
	char* nombreSwap;
	int cantidadPaginas;
	int tamanioPagina;
	int retardo;
	int retardo_swap;
} tconfig_swap;

typedef struct {
	int pid;
	char codigo_op;
	int cantidad_pagina;
	int tamanio_mensaje;
	char *mensaje;
}  __attribute__((packed)) tprotocolo_memoria_swap;

typedef struct {
	char codAux;
	int pid;
	int tamanio;
	char *mensaje;
}  __attribute__((packed)) tprotocolo_swap_memoria;

typedef struct{
	int pid;
	int num_pagina;
	bool escrita;
	bool leida;
} tpagina_procesada;

typedef struct{
	int pid;
	int comienzo;
	int paginas_ocupadas;
} tlista_ocupado;

typedef struct{
	int comienzo;
	int paginas_vacias;
} tlista_vacio;

typedef struct{
	int pid;
	int tamanio;
	char *buffer;
} tdatos_paginas;

#endif /* SRC_ESTRUCTURAS_H_ */
