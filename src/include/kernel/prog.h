/**
	\file kernel/prog.h
	\brief funciones relativas al proceso usuario PROG
*/
#ifndef __PROG_H
#define   __PROG_H

unsigned int __prog_size(); /*!< devuelve el tamanio del PROG */
unsigned char *__prog_begin(); /*!< devuelve la direccion de comienzo del PROG */

#endif
