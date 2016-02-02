#include "libSocket.h"
#include <commons/config.h>
#include <commons/log.h>
#include "estructuras.h"
#include "paquetes.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>

//Arma estructura del archivo en memoria
tconfig_memoria* leerConfiguracion() {
	tconfig_memoria* datosMemoria = malloc(sizeof(tconfig_memoria));
	t_config* config;
	config = config_create("../src/memoria.cfg");
	datosMemoria->puertoEscucha = config_get_string_value(config,"PUERTO_ESCUCHA");
	datosMemoria->ipSwap = config_get_string_value(config, "IP_SWAP");
	datosMemoria->puertoSwap = config_get_string_value(config, "PUERTO_SWAP");
	datosMemoria->maximoMarcosPorProceso = config_get_int_value(config,"MAXIMO_MARCOS_POR_PROCESO");
	datosMemoria->cantidad_marcos = config_get_int_value(config,"CANTIDAD_MARCOS");
	datosMemoria->tamanio_marco = config_get_int_value(config, "TAMANIO_MARCO");
	datosMemoria->entradasTLB = config_get_int_value(config, "ENTRADAS_TLB");
	datosMemoria->habilitadaTLB = config_get_string_value(config,"TLB_HABILITADA")[0];
	datosMemoria->retardoMemoria = config_get_int_value(config,"RETARDO_MEMORIA");
	datosMemoria->algoritmo_reemplazo = config_get_string_value(config, "ALGORITMO_REEMPLAZO")[0];
	return datosMemoria;
}

