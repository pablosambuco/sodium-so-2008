/**
	\file kernel/init.h
	\brief funciones relativas al proceso usuario INIT
*/
#ifndef __INIT_H
#define   __INIT_H

unsigned int __init_size(); /*!< devuelve el tamanio del IDLE */
unsigned char *__init_begin(); /*!< devuelve la direccion de comienzo del idle */

#endif
