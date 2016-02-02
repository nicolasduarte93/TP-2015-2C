/*
 * config.c
 *
 *  Created on: 11/9/2015
 *      Author: utnso
 */

#include <stdlib.h>
#include "estructuras.h"
#include <string.h>
#include <stdio.h>
#include <commons/config.h>

// funcion que obtiene los campos del archivo de configuracion del cpu
tipoConfiguracionCPU* leerConfiguracion() {
	tipoConfiguracionCPU* datosCPU = malloc(sizeof(tipoConfiguracionCPU));
	t_config* config;
	config = config_create("../src/cpu.cfg");
	datosCPU->cantidadHilos = config_get_int_value(config, "CANTIDAD_HILOS");
	datosCPU->ipPlanificador = config_get_string_value(config,
			"IP_PLANIFICADOR");
	datosCPU->ipMemoria = config_get_string_value(config, "IP_MEMORIA");
	datosCPU->puertoMemoria = config_get_string_value(config, "PUERTO_MEMORIA");
	datosCPU->puertoPlanificador = config_get_string_value(config,
			"PUERTO_PLANIFICADOR");
	datosCPU->retardo = config_get_int_value(config, "RETARDO");
	return datosCPU;
}
