# makefile para memoria

memoria : memoria.o configuracion.o funciones_memoria.o libSocket.o paquetes.o log_memoria.o
	gcc -o memoria memoria.o configuracion.o funciones_memoria.o libSocket.o paquetes.o log_memoria.o -lcommons -pthread

memoria.o : memoria.c libSocket.h estructuras.h paquetes.h configuracion.h funciones_memoria.h log_memoria.h
	gcc -c memoria.c -lcommons -pthread

log_memoria.o : log_memoria.c
	gcc -c log_memoria.c -lcommons

configuracion.o : configuracion.c libSocket.h estructuras.h paquetes.h 
	gcc -c configuracion.c -lcommons

funciones_memoria.o : funciones_memoria.c estructuras.h paquetes.h 
	gcc -c funciones_memoria.c -lcommons

libSocket.o : libSocket.c 
	gcc -c libSocket.c

paquetes.o : paquetes.c estructuras.h configuracion.h
	gcc -c paquetes.c -lcommons

clean:
	rm -f memoria \
		memoria.o configuracion.o funciones_memoria.o libSocket.o paquetes.o log_memoria.o log.txt
