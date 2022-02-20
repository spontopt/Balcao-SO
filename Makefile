all: balcao cliente medico

balcao: balcao.o 
	gcc balcao.o -o balcao -pthread

balcao.o: balcao.c balcao.h
	gcc -c balcao.c

cliente: cliente.o
	gcc cliente.o -o cliente

cliente.o: cliente.c  cliente.h
	gcc -c cliente.c 

medico: medico.o
	gcc medico.o -o medico -pthread

medico.o: medico.c medico.h
	gcc -c medico.c balcao.c


clean: 
	rm *.o balcao cliente medico *_fifo

