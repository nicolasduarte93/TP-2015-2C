#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <commons/string.h>

void log_inicializar(t_log *log, int pid, int paginas_asignadas) {
	char * str = malloc(30);
	strcpy(str, "inicializar-> pid: ");
	string_append(&str, string_itoa(pid));
	string_append(&str, ", paginas_asignadas: ");
	string_append(&str, string_itoa(paginas_asignadas));
	string_append(&str, "\n");
	log_info(log,str);
}

void log_lectura_escritura(char operacion, char * resultado, t_log *log, int pid, int nro_pagina, int numero_entrada_en_tlb, bool acierto_tlb, int nro_marco_resultante, char * mensaje) {
	char * str = malloc(30);
	strcpy(str, operacion == 'e' ? "escritura-> pid: " : "lectura-> pid: ");
	string_append(&str, string_itoa(pid));
	string_append(&str, ", acierto en la tlb: ");
	string_append(&str, acierto_tlb ? "si" : "no");
	string_append(&str, ", numero de entrada en la tlb: ");
	string_append(&str, acierto_tlb ? string_itoa(numero_entrada_en_tlb) : "-");
	string_append(&str, ", numero de pagina: ");
	string_append(&str, string_itoa(nro_pagina));
	string_append(&str, ", mensaje: ");
	string_append(&str, mensaje);
	string_append(&str, ", numero de marco: ");
	string_append(&str, string_itoa(nro_marco_resultante));
	string_append(&str, ", resultado del algoritmo: ");
	string_append(&str, resultado );
	string_append(&str, "\n");
	log_info(log,str);
}

void log_acceso_memoria(t_log * log, int pid, int pagina, int nro_marco) {
	char * str = string_new();
	string_append(&str, "acceso a memoria -> pid: ");
	string_append(&str, string_itoa(pid));
	string_append(&str, ", numero de pagina: ");
	string_append(&str, string_itoa(pagina));
	string_append(&str, ", numero de marco: ");
	string_append(&str, string_itoa(nro_marco));
	string_append(&str, "\n");
	log_info(log,str);
}

void log_acceso_a_swap(t_log * log, int pid, int pagina) {
	char * str = string_new();
	string_append(&str, "fallo de pagina -> pid: ");
	string_append(&str, string_itoa(pid));
	string_append(&str, ", numero de pagina: ");
	string_append(&str, string_itoa(pagina));
	string_append(&str, "\n");
	log_info(log,str);
}
