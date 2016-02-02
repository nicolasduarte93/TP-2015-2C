/*
 * config.c
 *
 *  Created on: 12/9/2015
 *      Author: utnso
 */

#include <stdlib.h>
#include "estructuras.h"
#include <commons/config.h>

tconfig_planif *leerConfiguracion(){
	tconfig_planif *datosPlanif = malloc(sizeof(tconfig_planif));
	t_config *config;
	config = config_create("../src/planificador.cfg");
	datosPlanif->puertoEscucha = config_get_string_value(config,"PUERTO_ESCUCHA");
	datosPlanif->algoritmo = config_get_string_value(config,"ALGORITMO_PLANIFICACION")[0];
	datosPlanif->quantum = config_get_int_value(config,"QUANTUM");
	return datosPlanif;
}
