/**
	\file kernel/pcb.h
	\brief Biblioteca de funciones y constantes para acceso a los PCB 
*/
#ifndef _PCB_H_
#define _PCB_H_

#include <kernel/shm.h>
#include <kernel/tiempo.h>

#define PROC_NO_DEFINIDO	-1	/*!< Valor de proceso no definido */
#define	PROC_EJECUTANDO		0 /*!< Valor de proceso ejecutando */
#define PROC_LISTO		1 /*!< Valor de proceso listo */
#define PROC_ESPERANDO		2 /*!< Valor de proceso Esperando */
#define PROC_DETENIDO		3 /*!< Valor de proceso Detenido */
#define PROC_ELIMINADO		4 /*!< Valor de proceso eliminado */
#define PROC_ZOMBIE		5 /*!< Valor de proceso Zombie */
#define PROC_WOTRACER		-1 /*!< Valor de proceso traceable */

typedef struct _stuPCB_{
  unsigned long int ulId;         /*!< id del proceso */
  unsigned int uiIndiceGDT_CS;         /*!< indice del descriptor de segmento de codigo de este proc en la GDT */
  unsigned int uiIndiceGDT_DS;         /*!< indice del descriptor de segmento de datos de este proc en la GDT */
  unsigned int uiIndiceGDT_TSS;         /*!< indice de la TSS de este proc en la GDT */
  void (* vFnFuncion) ();         /*!< puntero a funcion (proceso) */

  unsigned long ulParentId;   /*!< id del padre */
  unsigned long ulUsuarioId;  /*!< id del usuario */
  int iPrioridad;                 /*!< para futuro uso */
  int iEstado;                    /*!< ver estados mas arriba (PROC_XXX) */
  unsigned long lNHijos;
  int iExitStatus;
  unsigned long ulLugarTSS;
  char stNombre[25];
  unsigned int uiTamProc;
  struct stuTablaPagina * pstuTablaPaginacion;
  unsigned int uiDirBase,   /*!< direccion base de memoria (absoluta) */
	       uiLimite;        /*!< LONGITUD de memoria (NO existe granularidad) */
  stuMemoriasAtachadas memoriasAtachadas[MAXSHMEMPORPROCESO];

  //agregado
  unsigned long ulTiempoEspera;		/*!< Tiempos para algoritmos tipo HRN */
  unsigned long ulTiempoServicio;	/*!< Tiempo de servicio del proceso */
  //fin agregado	

  /*!< Estructura usada para guardar los tiempos de proceso usados para la syscall "times"*/	
  struct stuPCB* pstuPcbSiguiente;
  /*!< Array con los contadores utilizados como temporizadores para la SC "times"*/
  tms stuTmsTiemposProceso;
   /*!< Tiempo durante el cual el proceso tiene que permanecer "dormido" */
  itimerval timers[3];
  /*!< Puntero a donde se guardar� el resto del tiempo que deb�a estar detenido el proceso y no lo hizo*/
  long lNanosleep;
   /*!< Indica si el proceso esta siendo rastreado.*/  
  unsigned int *puRestoDelNanosleep;
   /*!< Indica si el proceso esta siendo rastreado.*/  
  long lPidTracer;
}stuPCB;


#endif 
