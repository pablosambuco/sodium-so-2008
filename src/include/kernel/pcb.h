/**
	\file kernel/pcb.h
	\brief Biblioteca de funciones y constantes para acceso a los PCB 
*/
#ifndef _PCB_H_
#define _PCB_H_

#include <kernel/shm.h>
#include <kernel/tiempo.h>

#define PROC_NO_DEFINIDO    -1  /*!< Valor de proceso no definido */
#define PROC_EJECUTANDO     0   /*!< Valor de proceso ejecutando */
#define PROC_LISTO          1   /*!< Valor de proceso listo */
#define PROC_ESPERANDO      2   /*!< Valor de proceso Esperando */
#define PROC_DETENIDO       3   /*!< Valor de proceso Detenido */
#define PROC_ELIMINADO      4   /*!< Valor de proceso eliminado */
#define PROC_ZOMBIE         5   /*!< Valor de proceso Zombie */
#define PROC_WOTRACER       -1  /*!< Valor de proceso traceable */

typedef struct _stuPCB_{
    unsigned long int ulId;         /*!< id del proceso */
    unsigned int uiIndiceGDT_CS;    /*!< posicion descriptor codigo en la GDT */
    unsigned int uiIndiceGDT_DS;    /*!< posicion descriptor datos  en la GDT */
    unsigned int uiIndiceGDT_TSS;   /*!< posicion descriptor TSS    en la GDT */
    void (* vFnFuncion) ();         /*!< puntero a funcion (proceso) */
    unsigned long ulParentId;       /*!< id del padre */
    unsigned long ulUsuarioId;      /*!< id del usuario */
    int iPrioridad;                 /*!< para futuro uso */
    int iEstado;                    /*!< ver estados mas arriba (PROC_XXX) */
    unsigned long lNHijos;
    int iExitStatus;
    unsigned long ulLugarTSS;
    char stNombre[25];
    unsigned int uiTamProc;         //TODO! Deprecar por uiLimite
    struct stuTablaPagina * pstuTablaPaginacion;
    unsigned int uiDirBase,         /*!< direccion base de memoria (absoluta) */
	             uiLimite;  /*!< LONGITUD de memoria (NO existe granularidad). Se usa tambien como direccion de BRK (primer posicion no direccionable por el proceso) */
    stuMemoriasAtachadas memoriasAtachadas[MAXSHMEMPORPROCESO];
  
    //agregado
    unsigned long ulTiempoEspera;   /*!< Tiempos para algoritmos tipo HRN */
    unsigned long ulTiempoServicio; /*!< Tiempo de servicio del proceso */
    //fin agregado	

    struct stuPCB* pstuPcbSiguiente;
    tms stuTmsTiemposProceso;   /*!< Estructura usada para guardar los tiempos de proceso usados para la syscall "times"*/
    itimerval timers[3];        /*!< Array con los contadores utilizados como temporizadores para la SC "times"*/
    long lNanosleep;            /*!< Tiempo durante el cual el proceso tiene que permanecer "dormido" */
    unsigned int *puRestoDelNanosleep;  /*!< Puntero a donde se guardo el resto del tiempo que debia estar detenido el proceso y no lo hizo*/
    long lPidTracer;            /*!< Indica si el proc esta siendo rastreado.*/  
    //Agregado 2008 - GRUPO SEGMENTACION PURA
    unsigned int uiTamanioTexto;/*!< Bytes de codigo ejecutable */
    unsigned int uiTamanioDatosInicializados;/*!< Bytes de datos globales inicializados (incluye BSS al final)*/
    unsigned int uiTamanioStack;/*!< Bytes de stack (libres+usados) */

    unsigned int uiTamanioOverhead;     /*!< Bytes de Overhead (frag. interna)*/

    //Fin Agregado 2008
}stuPCB;


#endif 
