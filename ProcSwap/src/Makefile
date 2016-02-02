# makefile para swap

swap: swap.o config.o libSocket.o manejo_listas_archivo_swap.o paquetes.o
	gcc -o swap swap.o config.o libSocket.o manejo_listas_archivo_swap.o paquetes.o -lcommons

manejo_listas_archivo_swap.o: manejo_listas_archivo_swap.c estructuras.h paquetes.h config.h
	gcc -c manejo_listas_archivo_swap.c -lcommons

swap.o: swap.c libSocket.h estructuras.h manejo_listas_archivo_swap.h config.h paquetes.h
	gcc -c swap.c -lcommons

config.o: config.c libSocket.h estructuras.h paquetes.h
	gcc -c config.c -lcommons

libSocket.o: libSocket.c
	gcc -c libSocket.c

paquetes.o: paquetes.c estructuras.h config.h
	gcc -c paquetes.c


clean: 
	rm -f swap \
		swap.o config.o libSocket.o manejo_listas_archivo_swap.o paquetes.o swap.dat log.txt
