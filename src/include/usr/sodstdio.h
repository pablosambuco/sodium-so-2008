/* ATENCION: Este archivo esta basado en include/video.h
 * Cualquier modificacion que se haga en este archivo debe ser tenida en cuenta
 * para ser incluida en el archivo original mencionado y viceversa.
 *
 * Se opto por aniadir el sufijo _usr a las funciones que aqui figuran para
 * diferenciarlas de las disponibles a nivel kernel. Esta decision no se
 * relaciona con ninguna limitacion tecnica, solo se hace para evitar
 * confusiones a quienes las utilicen.
 * 
 * Puede que muchas de las funciones y partes del codigo de hallen comentadas.
 * Esto no quiere decir que no funcionen o sean inapropiadas, es solo que al
 * momento de generar este archivo no se tuvo el tiempo necesario para probarlas
 */

/**
 * \file sodstdio.h
 * Biblioteca que contiene todas las funciones de entrada/salida estandar
 * utiles a nivel usuario
 */
#ifndef __SODSTDIO_H_
#define __SODSTDIO_H_

#include <usr/libsodium.h>
#include <usr/sodstdlib.h>

/**
 * \brief Imprime la cadena que recibe como argumento
 */
int iFnImprimirCadenas_usr (const char*);

/**
 * \brief Imprime un numero entero por pantalla
 */
int iFnImprimirEntero_usr (const int);

/**
 * \brief Imprime un numero flotante por pantalla
 * \brief Recibe la cantidad de decimales por parametro
 */
int iFnImprimirFloat_usr (const double, const int);

/**
 * \brief Imprime un numero entero por pantalla en formato hexa
 */
int vFnImprimirHexa_usr (const int);

/**
 * \brief Imprime un numero de 16 bits por pantalla en formato hexa
 */
int vFnImprimirWordHexa_usr (const word);

/**
 * \brief Imprime un numero de 8 bits por pantalla en formato hexa
 */
int vFnImprimirByteHexa_usr (const byte);

/**
 * \brief Imprime una cadena por pantalla, aceptando argumentos variables
 * ( analoga a printf )
 */
int iFnImprimir_usr (char*, ... );

#endif
