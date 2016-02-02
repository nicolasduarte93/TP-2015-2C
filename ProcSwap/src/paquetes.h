#ifndef SRC_PAQUETES_H_
#define SRC_PAQUETES_H_

#include "estructuras.h"

FILE * iniciar_archivo_swap(void);
bool recibir_paquete_desde_memoria(int *socket_memoria, tprotocolo_memoria_swap *paquete_desde_memoria);
void* serializar_a_memoria(tprotocolo_swap_memoria *protocolo);
void armar_estructura_protocolo_a_memoria(tprotocolo_swap_memoria *protocolo,char codAux, int pid, char* mensaje);

#endif /* SRC_PAQUETES_H_ */
