# makefile para planificador

planificador : planificador.o config.o funcionesPlanificador.o libSocket.o logueo.o selector.o
	gcc -o planificador planificador.o config.o funcionesPlanificador.o libSocket.o logueo.o selector.o -lcommons -pthread

config.o : config.c estructuras.h
	gcc -c config.c -lcommons

funcionesPlanificador.o : funcionesPlanificador.c estructuras.h 
	gcc -c funcionesPlanificador.c -lcommons -pthread

libSocket.o : libSocket.c
	gcc -c libSocket.c

logueo.o : logueo.c funcionesPlanificador.h
	gcc -c logueo.c -lcommons

selector.o : selector.c estructuras.h funcionesPlanificador.h libSocket.h
	gcc -c selector.c -lcommons -pthread

clean:
	rm -f planificador \
		planificador.o config.o funcionesPlanificador.o libSocket.o logueo.o selector.o
