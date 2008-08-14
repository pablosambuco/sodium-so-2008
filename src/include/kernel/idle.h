/**
	\file kernel/idle.h
	\brief funciones relativas al proceso usuario IDLE
*/
#ifndef __IDLE_H
#define   __IDLE_H

unsigned int __idle_size(); /*!< devuelve el tamanio del IDLE */
unsigned char *__idle_begin(); /*!< devuelve la direccion de comienzo del idle */

#endif
