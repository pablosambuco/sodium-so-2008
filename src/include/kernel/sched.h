/**
	\file kernel/sched.h
	\brief valores de los planificadores
*/
#ifndef _SCHED_H_
#define _SCHED_H_

#define FIFO 0 /*!< Valor del planificador FIFO*/
#define RR 1 /*!< Valor del planificador Round Robin*/
#define BTS 2 /*!< Valor del planificador Balance Tiempo-espera servicio (inventado por alumnos, version Gamma)*/ 

/**
	\brief estructura POSIX para prioridad que se usa como parametro de varias syscall POSIX
*/
typedef struct
{
	int sched_priority; /*!<Define la prioridad del proceso actual*/
}sched_param;

/*!< mantiene el timeSlice de un proceso, su tiempo de quantum usado*/
unsigned long int uliTimeSlice; 

/*!<inicialmente es igual al Q en definiciones.h*/
unsigned long int uliQuantum; 
/*!inicialmente no tiene valor, SETEAR PARA USAR!! */
unsigned long int uliBTSQ; 

/*!<Variable que mantiene el valor del proceso actual, usada por los planificadores*/
static int staiN = 0;
/*!<Variable que mantiene el valor del proceso anterior*/
static int staiProcesoAnterior = -1;

/*!< El valor de la tarea nula*/
int iTareaNula;

void vFnPlanificadorFIFO();
void vFnPlanificadorRR();
void vFnPlanificadorBTS();

#endif
