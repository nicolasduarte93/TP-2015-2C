#ifndef SRC_MANEJO_LISTAS_ARCHIVO_SWAP_H_
#define SRC_MANEJO_LISTAS_ARCHIVO_SWAP_H_

#include <commons/collections/list.h>


int get_comienzo_espacio_asignado(t_list * lista_ocupado, int pid);
int dame_si_hay_espacio(t_list** lista_vacia, int paginas_pedidas, int *comienzo);
int espacio_total_disponible(t_list* lista_vacia);
t_list *pasar_ocupada_a_lista_auxiliar(FILE **swap, t_list **lista_ocupada, int tamanio_pagina);
void reinicar_archivo_swap(FILE **swap, t_list **lista_ocupada);
void compactar_lista_vacia(t_list **lista_vacia, FILE **swap, int tamanio_pagina ,int total_de_paginas);
void compactar_swap(FILE ** swap, t_list** lista_vacia, t_list** lista_ocupada,int tamanio_pagina, int total_de_paginas);
void arreglar_lista_vacia(t_list ** lista_vacia);
void asignar_espacio(int pid, int comienzo, int cantidad_pagina, t_list **lista_ocupado, t_log **log_swap, int tamanio_pagina);
void avisar_a_memoria(char cod_aux, int pid, char * pag_data, int socket_memoria);
void registrarOperacion(t_list ** lista_procesado, int pid, int num_pagina, bool es_lectura);

#endif /* SRC_MANEJO_LISTAS_ARCHIVO_SWAP_H_ */
