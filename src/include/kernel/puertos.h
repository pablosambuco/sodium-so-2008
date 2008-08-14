/**
	\file kernel/puertos.h
	\brief Funciones de biblioteca de acceso a los puertos
*/	
#ifndef _PUERTOS_H_
#define _PUERTOS_H_

#include <kernel/definiciones.h>

/**
	\brief Macro que llama a la funcion outb, intercambiando los parametros
	\param puerto puerto al que pasar el valor
	\param valor valor a pasar
*/
#define outport( puerto, valor ) outb( valor, puerto )

/**
 * \brief funcion que lee un byte de un puerto determinado (implementada en 
 * assembler) 
 *
 * \param word (word) numero de puerto a leer
 *
 * \return (byte) lectura realizada
 * \sa puertos.asm
 */
byte inb(word);

/**
 * \brief funcion que lee una palabea de un puerto determinado (implementada en 
 * assembler)
 *
 * \param word (word) numero de puerto a leer
 *
 * \return (word) lectura realizada
 *
 * \sa puertos.asm
 */
word inw(word);

/**
 * \brief funcion que escribe un byte en un puerto determinado (implementada en
 * assembler)
 *
 * \param valor (byte) valor a escribir en el puerto
 *
 * \param puerto (word) numero de puerto donde escribir
 *
 * \sa puertos.asm
 */
void outb(const byte valor, const word puerto);

/**
 * \brief funcion que escribe una palabra en un puerto determinado (implementada
 * en assembler)
 *
 * \param valor (word) valor a escribir en el puerto
 *
 * \param puerto (word) numero de puerto donde escribir
 * \sa puertos.asm
 */
void outw(const word valor, const word puerto);

#endif //_PUERTOS_H_
