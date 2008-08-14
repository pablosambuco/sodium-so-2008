/** 
	\file kernel/tiempo.h
	\brief Biblioteca de funciones de la syscall de tiempos
*/
#ifndef __TIEMPO_H
#define __TIEMPO_H

#include <kernel/definiciones.h>

/**
* \defgroup timerConst Constantes para los tipos de timers
*/
/*@{*/
#define ITIMER_REAL 		0
#define ITIMER_VIRT 		1
#define ITIMER_PROF 		2
/*@}*/

#define EINTR 			1

/**
* \defgroup timerMode Constantes para el modo de la syscall adjtimex
*/
/*@{*/
#define ADJ_OFFSET		0X0001
#define ADJ_FREQUENCY		0X0002
#define ADJ_MAXERROR		0X0004
#define ADJ_ESTERROR		0X0008
#define ADJ_STATUS		0X0010
#define ADJ_TIMECONST		0X0020
#define ADJ_TICK		0X4000
#define ADJ_OFFSET_SINGLESHOT	0X8001
/*@}*/

/**
* \defgroup timerState Estado del reloj devuelto por adjtimex
*/
/*@{*/
#define TIME_OK			0
#define TIME_INS		1
#define TIME_DEL		2
#define TIME_OOP		3
#define TIME_WAIT		4
#define TIME_BAD		5
/*@}*/

/*!< Cantidad de milisegundos que es capas de identificar el reloj 
(Aunque el timer no est� seteado para hacerlo)*/
#define PRECISION_RELOJ		1


// Tipos de dato
typedef unsigned long int time_t;
typedef unsigned long int clock_t;

typedef struct{
	/*!< Tiempo de usuario */
        clock_t tms_utime;
	
	/*!< Tiempo de sistema */
        clock_t tms_stime;
	
	/*!< Tiempo de usuario del hijo */
        clock_t tms_cutime;
	
	/*!< Tiempo de sistema del hijo */
        clock_t tms_cstime;
} tms;

typedef struct{
	/*!< Minutos al O. de Greenwich*/
        int tz_minuteswest;
	
	/*!< Tipo de correcci�n horaria invierno/verano */
        int tz_dsttime;
} timezone;

typedef struct{
	/*!< Segundos */
        unsigned long int tv_sec;
	
	/*!< Microsegundos */
        unsigned long int tv_usec;
} timeval;

typedef struct{
	/*!< Valor pr�ximo */
        timeval it_interval;
	
	/*!< Valor actual */
        timeval it_value;
} itimerval;

typedef struct{
	/*!< Selector de modo */
        int modes;
	
	/*!< Ajuste de la hora (usec)*/
	long offset;
	
	/*!< Ajuste de la frequencia (ppm escalada)*/
	long freq;
	
	/*!< Error m�ximo (usec) */
	long maxerror;
	
	/*!< Error estimado (usec) */
	long esterror;
	
	/*!<  Estado del reloj */
	int status;
	
	/*!< pll constante de tiempo */
	long constant;
	
	/*!< Precisi�n del reloj (usec) (Solo lectura) */
	long precision;
	
	/*!< Tolerancia de la frecuencia del reloj (ppm) (Solo lectura) */
	long tolerance;
	
	/*!< Hora actual (Solo lectura) */
	timeval time;
	
	/*!< usec entre ticks de reloj */
	long tick;
} timex;

typedef struct{
	/*!< Segundos */
        long int tv_sec;
	
	/*!< Nanosegundos */
        long int tv_nsec;
} timespec;


#endif
