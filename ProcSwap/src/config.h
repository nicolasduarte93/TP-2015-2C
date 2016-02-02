#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_
#include <commons/log.h>
#include <commons/collections/list.h>

tconfig_swap* leerConfiguracion();
void log_lectura(t_log *log, int pid, int nro_pagina_inicial, int tamanio_pagina, int pagina_a_leer, char* contenido);
void log_proc_rechazado(t_log *log, int pid);
void log_finalizar(t_log *log, t_list * lista_procesado, int pid, int pag_ocupadas, int tamanio_pagina);
void log_inicializar(t_log *log, int pid, int nro_pagina_inicial, int tamanio_pagina, int paginas_asignadas);
void log_escritura(t_log *log, int pid, int nro_pagina_inicial, int tamanio_pagina, int pagina_a_escribir, char* contenido);

#endif /* SRC_CONFIG_H_ */
