all: pruebas.c memoria_dinamica.o
	gcc -Wall -I. -g pruebas.c memoria_dinamica.o -save-temps -o pruebas
	
memoria_dinamica.o: memoria_dinamica.c memoria_dinamica.h
	gcc -Wall -g memoria_dinamica.c -save-temps -c
	
