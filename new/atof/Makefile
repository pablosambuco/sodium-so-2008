all: pruebas.c funciones.asm
	nasm funciones.asm -f elf -ofunciones.o -l funciones.lst
	gcc -Wall -g pruebas.c funciones.o -save-temps -o pruebas
