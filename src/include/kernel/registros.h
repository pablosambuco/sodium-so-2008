/**
 * \file kernel/registros.h
 * \brief esta biblioteca contiene los headers de funciones de bajo nivel
 * que obtienen los valores de los registros del CPU
*/

#ifndef _REGISTROS_H_
#define _REGISTROS_H_

#include <kernel/definiciones.h>

/**
 * \brief Devuelve el Stack Segment
 */
word wFnGetSS();

/**
 * \brief Devuelve el Stack Segment
 */
word wFnGetES();

/**
 * \brief Devuelve el Segmento de Pila
 */
word wFnGetCS();

/**
 * \brief Devuelve el El segmento de Datos
 */
word wFnGetDS();

/**
 * \brief Devuelve el Segmento F
 */
word wFnGetFS();

/**
 * \brief Devuelve el Segmento G
 */
word wFnGetGS();

/**
 * \brief Devuelve el Puntero de Pila (ESP)
 */
dword wFnGetESP();

void vFnLTR(dword);

dword uivFnSTR();

#endif //_REGISTROS_H_
