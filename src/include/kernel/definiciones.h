/**
 * \file kernel/definiciones.h
 * \brief Definiciones generales.
 */

#ifndef _DEFINICIONES_H_
#define _DEFINICIONES_H_

#include <usr/tipos.h> /* Inclusion de tipos de datos */

#define VERSION_SODIUM "0.82" /*!< Version Sodium*/

#define NULL 0

#define NOALIGN __attribute__((packed))

#define CLOCK 1193180
#define QUANTUM 1  /*!<quantum de RR */
#define QBTS 2	/*!< quantum de BTS */
#define INTERVALO 0xFFFF /*!< Resultado de ( CLOCK / ( 1000/QUANTUM ) ) */

#define STRPROMPTLINE "\nCmd>" /*!< Texto del prompt*/
#define CANTMAXPROCS  50 /*!<Procesos maximos */
#define INDICEGDTBASE 3
#define TAMANIOPAGINA 4096
#define MODOPAGINADO 1
#define MODOSEGMENTADO 0

#define MEM_BASE 1048576    /*!< 1Mb */
#define MEM_TOPE 6291456    /*!< 6Mb */

#define TSS_TAMANIO_STACK_R0 1024 /*!< tamanio real del stack >=1024 para todos los rings de ejecucion. */
/**
 * \note Limitado temporalmente para facilitar el desarrollo del SO.
 * Si se vuelve a 1024, debera limitarse la cantidad maxima de procesos a 6 o 7.
 */

#define TSS_TAMANIO_STACK_R1 16  
#define TSS_TAMANIO_STACK_R2 16


#define ANIO_INICIO 2000	  /*!< A�o de inicio del sistema */
#define DIAS_ANIO_INICIO 366  /*!< Cantidad de dias del a�o de inicio */
//---------------------------------------------------------------

#define  LOG_HISTORIA   25 /*!<Define la cantidad de lineas de profundidad del buffer del log. */

// La memoria ALTA comienza en 1MB, la BAJA comienza donde terminan las estructuras del kernel
#define INICIO_MEMORIA_ALTA 0x100000


#endif //_DEFINICIONES_H_
