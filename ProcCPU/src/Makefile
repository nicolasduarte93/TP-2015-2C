# makefile para cpu

cpu : cpu.o config.o funcionesCPU.o libSocket.o logueo.o serializacion.o
	gcc -o cpu cpu.o config.o funcionesCPU.o libSocket.o logueo.o serializacion.o -lcommons -pthread

config.o : config.c estructuras.h 
	gcc -c config.c -lcommons

funcionesCPU.o : funcionesCPU.c estructuras.h logueo.h serializacion.h
	gcc -c funcionesCPU.c -lcommons

libSocket.o : libSocket.c
	gcc -c libSocket.c

logueo.o : logueo.c estructuras.h
	gcc -c logueo.c -lcommons

serializacion.o : serializacion.c estructuras.h
	gcc -c serializacion.c

clean:
	rm -f cpu \
		cpu.o config.o funcionesCPU.o libSocket.o logueo.o serializacion.o
