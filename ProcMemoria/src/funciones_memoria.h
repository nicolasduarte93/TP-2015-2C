#ifndef SRC_FUNCIONES_MEMORIA_H_
#define SRC_FUNCIONES_MEMORIA_H_

#include "estructuras.h"
#include <commons/collections/list.h>

char * crear_memoria(int cantidad_marcos, int tamanio_marcos);
tabla_paginas * inicializar_tabla_de_paginas(int cantidad_maxima_marcos_por_proceso, int pid);
void eliminar_tabla_de_proceso(int pid, t_list ** lista_tabla_de_paginas);
tabla_paginas * dame_la_tabla_de_paginas(int pid, t_list ** lista_tabla_de_paginas);
int obtener_marco_pagina(tabla_paginas *tabla, int pagina, int es_clock);
bool estan_los_frames_ocupados(t_list *tabla_paginas, bool es_clock);
int dame_un_marco_libre(t_list *lista_tabla_de_paginas, int cantidad_marcos, bool es_clock);
char * dame_mensaje_de_memoria(char **memoria, int nro_marco, int tamanio_marco);
void avisar_a_cpu(char cod_op, char cod_aux, int pid, int paginas, char *mensaje, int socket_cli_cpu);
void avisar_a_swap(char cod_op, int pid, int paginas, char *mensaje, int socket_ser_swap);
t_list * inicializar_tlb(int nro_entradas);
int dame_el_marco_de_la_pagina_en_la_tlb(t_list ** tlb, int pid, int nro_pagina);
void eliminar_entrada_tlb(t_list ** tlb, int pid, int nro_pagina);
char actualizame_la_tlb(t_list ** tlb, int pid, int direccion_posta, int nro_marco);
void borrame_las_entradas_del_proceso(int pid, t_list ** tlb);
void limpiar_la_tlb(t_list ** tlb);
void limpiar_memoria(t_list ** tabla_de_paginas, char * memoria, int tamanioMarco, int socketSwap);
void volcar_memoria(char * memoria, tconfig_memoria * config, t_log * logMem);
void poneme_en_modificado_la_entrada(tabla_paginas *tabla, int pagina);
int dame_el_numero_de_entrada_de_la_tlb(t_list * tlb, int nro_marco);
void aplicar_LRU(t_list **tabla_de_paginas, int nro_pagina);
void reemplazar_pagina(int nro_pagina, int * nro_pagina_reemplazada, t_list * paginas, int pos, int * puntero, bool es_escritura);
bool aplicar_clock_modificado(int nro_pagina, int * nro_pagina_reemplazada, t_list * paginas, int * puntero, bool es_escritura);
bool f_u_cero_m_cero(pagina_direccion * una_pagina);
bool f_u_cero_m_uno(pagina_direccion * una_pagina);
void llevar_a_swap(int socket_Swap, char * memoria, pagina_direccion * pagina, int tamanio_marco, int pid);
void traer_de_swap(int socket_Swap, char * memoria, int nro_marco, int nro_pagina, int tamanio_marco, int pid);
void poneme_en_uso_la_entrada(tabla_paginas *tabla, int pagina);
void registrar_acceso(t_list ** lista, int num_pagina);
bool hay_algun_marco_en_la_tabla_de_pagina(t_list * paginas);
void aplicar_algoritmos_a_la_tabla(tprotocolo_desde_cpu_y_hacia_swap paquete_desde_cpu, int socketClienteCPU,int socketClienteSWAP, tabla_paginas ** tabla_de_paginas, t_log * logMem,tconfig_memoria * config, t_list ** tlb,char ** memoria);

#endif /* SRC_FUNCIONES_MEMORIA_H_ */
