#include <video.h>
#include <kernel/gdt.h>
#include <kernel/syscall.h>
#include <kernel/system.h>
#include <kernel/sched.h>
#include <kernel/puertos.h>
#include <kernel/semaforo.h>
#include <kernel/shm.h>
#include <kernel/prog.h>	// para conocer el programa de prueba a levantar con execve()
#include <fs/ramfs.h>

#include <kernel/definiciones.h>
extern stuPCB pstuPCB[CANTMAXPROCS];
extern stuTSS stuTSSTablaTareas[CANTMAXPROCS];
extern unsigned long ulProcActual;
extern unsigned long ulUltimoProcesoEnFPU;
extern short int bPlanificador;

extern unsigned long uliQuantum;
extern unsigned long uliBTSQ;

/* Todas estas variables, ver declaracion en system.c */
extern unsigned long ulTiempo;
extern int iMinuteswest;
extern unsigned int iMilisegundosPorTick;
extern long lRelojOffset;
extern long lRelojFrequencia;
extern long lRelojMaxerror;
extern long lRelojEsterror;
extern long lRelojConstante;
extern int iRelojEstado;

extern void vFnIniciarTimer_Asm(int divisor);

/* macro que toma el DS del proceso usuario, y lo usa temporalmente
 * para guardar un valor 'val' en el offset dado dentro de ese segmento
 * (se usa cuando un syscall recibe un puntero a un dato, que no es mas
 * que un offset dentro del segmento de datos correspondiente).
 * esto se podria haber hecho tambien tomando la direccion base almacenada
 * en la PCB del invocador, pero era muy facil ;) */
#define	_copiar_a_userland( ds, offset, val ) 	\
	__asm__ volatile (			\
		 "pushl	%%ds		\n\t"	\
		 "movl	%0, %%ds	\n\t"	\
		 "movl	%2, (%1)	\n\t"	\
		 "popl	%%ds		\n\t"	\
		 :: "a"( ds ),			\
		    "b"( offset ),		\
		    "c"( val ) )


/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysExit(int status)
{
	int iPCBPadre =
	    iFnBuscaPosicionProc(pstuPCB[ulProcActual].ulParentId);

	/*vFnImprimir( "\n exit(): el proceso %d esta terminando con estado %d", 
	   pstuPCB[ ulProcActual ].ulId,
	   status ); */
	pstuPCB[ulProcActual].iEstado = PROC_ZOMBIE;
	pstuPCB[ulProcActual].iExitStatus = status;

	/*vFnImprimir( "\n exit(): mi padre %d %s (%d)", 
	   pstuPCB[ ulProcActual ].ulParentId, 
	   pstuPCB[ iPCBPadre ].iEstado == PROC_ESPERANDO ? "ME esta esperando" : "me abandono",
	   pstuPCB[ iPCBPadre ].iEstado ); */
	if (pstuPCB[iPCBPadre].iEstado == PROC_ESPERANDO) {
		/*vFnImprimir( "\n exit(): el proceso %d despierta a su padre %d", 
		   pstuPCB[ ulProcActual ].ulId,
		   pstuPCB[ iPCBPadre ].ulId ); */
		pstuPCB[iPCBPadre].iEstado = PROC_LISTO;
	}

	// Devuelve el prompt al shell.
	vFnImprimirPrompt();	

	// llamar al planificador, para que este proceso no se ejecute nunca mas,
	// ya que queda en estado ZOMBIE
	vFnPlanificador();

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysFork()
{
	int ret = iFnDuplicarProceso(ulProcActual);
	vFnImprimir("\nlFnSysFork(): iFnDuplicarProceso retorna: %d",ret);
	if (ret < 0)
		return ret;
	//XXX Para debuguear el estado del nuevo proceso lanzado (tss, stack, y pcb) podemos detenerlo antes de que tenga la oportunidad de ser ejecutado
	//pstuPCB[ret].iEstado=PROC_DETENIDO;
	return pstuPCB[ret].ulId;	// al padre le retornamos el PID del hijo
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysTest(long num)
{
	vFnImprimir("%d", num);
	return num;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysRead(int fd, void *buf, size_t count)
{
	return -ENOSYS;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysWrite(int fd, const void *buf, size_t count)
{
	/* ignoramos por el momento el descriptor fd y el parametro 'count' */
	vFnImprimir("%s",
		    (unsigned char *) (pstuPCB[ulProcActual].uiDirBase +
				       (unsigned int) buf));
	return count;		/* supuestamente devuelve la cantidad de bytes escritos */
}


/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysWaitPid(int pid, int *status, int options)
{
	int iPCBHijo = iFnBuscaPosicionProc(pid);

	/* si no encontramos el proceso por PID, devolvemos ECHILD, para
	 * ser compatibles con POSIX */
	if (iPCBHijo < 0) {
		return -ECHILD;
	}

	/*vFnImprimir( "\n waitpid(): mi hijo %d %s", 
	   pid, pstuPCB[ iPCBHijo ].iEstado != PROC_ZOMBIE ? "NO esta zombie" : "Ya esta zombie" ); */
	while (pstuPCB[iPCBHijo].iEstado != PROC_ZOMBIE) {
		/*vFnImprimir( "\n waitpid(): esperando que %d termine", pid ); */
		pstuPCB[ulProcActual].iEstado = PROC_ESPERANDO;
		/* llamamos al planificador para que nos quite y pueda
		 * ejecutar el hijo, de forma tal que en algun momento
		 * termine, y se pueda volver */
		vFnPlanificador();
		/*vFnImprimir( "\n waitpid(): %d me desperto", pid ); */
	}

	/* si estamos aca, significa que el hijo ya esta zombie, o bien
	 * porque ya lo estaba ni bien arranco la syscall, o porque lo esperamos
	 * cambiando nuestro estado a PROC_ESPERANDO (despues de varias esperas
	 * posiblemente) */

	/* ahora que termino, recolectamos su estado, copiandolo
	 * al proceso padre en 'status' */

	/*_copiar_a_userland( stuTSSTablaTareas[ ulProcActual ].ds,
			    status,
			    pstuPCB[ iPCBHijo ].iExitStatus );*/

	*(int *) (pstuPCB[ulProcActual].uiDirBase +
		  (unsigned char *) status) =
	    pstuPCB[iPCBHijo].iExitStatus;

	/* ahora si lo marcamos como eliminado, y liberamos todo lo que estaba utilizando */
	iFnEliminarProceso(iPCBHijo);

	return pid;
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysExecve(const char *filename, char *const argv[],
		  char *const envp[])
{
	stEntradaLS *pstEntLs;
	stDirectorio *pstDirBusqueda;
	char* strNombreArchivo =(char*)((unsigned int)pstuPCB[ulProcActual].uiDirBase + (unsigned int)filename);

	if (iFnObtenerDirectorio("/mnt/usr", &pstDirBusqueda) < 0) {
		vFnImprimir
		    ("\nSodium Dice: Error! El directorio no existe");
		return -ENOENT;
	}

	if (iFnBuscarArchivo(pstDirBusqueda, strNombreArchivo, &pstEntLs) < 0) {
		vFnImprimir
		    ("\nSodium Dice: Error! Archivo no encontrado (%s)",
		     strNombreArchivo);
		return -ENOENT;
	}
	iFnReemplazarProceso(ulProcActual,
			     (unsigned char*) DIR_LINEAL(pstEntLs->wSeg, pstEntLs->wOff), 
			     pstEntLs->dwTamanio);

//      iFnReemplazarProceso( ulProcActual, __prog_begin(), __prog_size() );

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysTime(long *lT)
{
	long lTime = 17;

	/* XXX buscar un valor para devolver realmente */
	vFnImprimir
	    ("\n time(): el proceso actual %d usa DS=%x guardamos %d en %x",
	     ulProcActual, stuTSSTablaTareas[ulProcActual].ds, lTime, lT);

	_copiar_a_userland(stuTSSTablaTareas[ulProcActual].ds, lT, lTime);

	return lTime;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysGetPid()
{
	return pstuPCB[ulProcActual].ulId;
}


/**
 * \brief Envia una senal a un proceso
 * \param pid Pid del proceso
 * \param sig Numero de senal
 * \returns 0 si se completo con exito, < 0 si ocurrio un error
 */
long lFnSysKill(int pid, int sig)
{
	int iPosicionPCB = iFnBuscaPosicionProc(pid), iPCBPadre = 0;

	/* si no encontramos el proceso por PID, devolvemos ESRCH
	 * (requerido por POSIX) */
	if (iPosicionPCB < 0 || pstuPCB[iPosicionPCB].iEstado == PROC_ZOMBIE) {
		return -ESRCH;
	}

	switch (sig) {
    case (SIGFPE):
        //TODO - Realizar tratamiento de la senial
        vFnImprimir(
            "\nSe recibio SIGFPE (Error en operacion de punto flotante)");
        //break;
	case (SIGTERM):
	case (SIGKILL):
		/* buscamos al padre para despertarlo, de ser necesario */
		iPCBPadre = iFnBuscaPosicionProc(pstuPCB[iPosicionPCB].ulParentId);
		/* forzamos la salida del proceso, dejandolo en estado zombie, y 
		 * almacenando -1 como valor de salida (valor arbitrario, deberian
		 * implementarse las macros que separan valores de salida del estado
		 * de salida forzado por senal) */
		pstuPCB[iPosicionPCB].iEstado = PROC_ZOMBIE;
		pstuPCB[iPosicionPCB].iExitStatus = -1;
		/* despertamos al padre, si estaba esperando */
		if (pstuPCB[iPCBPadre].iEstado == PROC_ESPERANDO) {
			pstuPCB[iPCBPadre].iEstado = PROC_LISTO;
		}
		break;
	case (SIGSTOP):
		if (pstuPCB[iPosicionPCB].iEstado == PROC_LISTO ||
		    pstuPCB[iPosicionPCB].iEstado == PROC_EJECUTANDO) {
			vFnImprimir
			    ("\n pasando el proceso %d (en estado %d) a estado DETENIDO ",
			     pid, pstuPCB[iPosicionPCB].iEstado);
			pstuPCB[iPosicionPCB].iEstado = PROC_DETENIDO;
		}
		break;
	case (SIGCONT):
		if (pstuPCB[iPosicionPCB].iEstado == PROC_DETENIDO) {
			pstuPCB[iPosicionPCB].iEstado = PROC_LISTO;
        }
		break;
	default:
		return -EINVAL;
	}

	return 0;
}


/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysGetPPid()
{
	return pstuPCB[ulProcActual].ulParentId;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysReboot(int flag)
{
	outb(0xfe, 0x64);
	return -ENOSYS;
}

// ****************     TP 3 - 2007 - Syscalls de IPC    *****************

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
//long lFnSysSemInit( sem_t *sem, int pshared, unsigned int value ){
long lFnSysSemInit( sem_t *sem, sem_init_params * params ){
	params = (sem_init_params *)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)params);
	return iFnSemInit(sem, params->pshared, params->value);
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysSemPost( sem_t *sem ){
	return iFnSemPost(sem);
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysSemWait( sem_t *sem ){
	return iFnSemWait(sem);
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysSemClose( sem_t *sem ){
	return iFnSemClose(sem);
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysShmGet(key_t key, size_t size){
	return iFnShmGet(key, size);
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysShmAt( int shmid, void * shmAddr ){
	return iFnShmAt(shmid, shmAddr);
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysShmDt( key_t key ){
	return iFnShmDt( key );
}


// ****************     TP 3 - 2007 - Syscalls de CLONE    *****************

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnSysSumar(int a, int b,int * res)
{

	_copiar_a_userland( stuTSSTablaTareas[ ulProcActual ].ds,res,a+b);
	
	return *res;
}

/*Función: lFnSysNice
 * Parámetros: 
 *	iIncremento (int): valor que quiero sumar a la prioridad del proceso 
 *	actual.
 *Valor de retorno: 0
 *
 * Permite incrementar la prioridad del proceso actual en un valor pasado como 
 * parámetro.
 */

long lFnSysNice(int iIncremento)
{
	long lNuevaPrio;


	lNuevaPrio = pstuPCB[ulProcActual].iPrioridad + iIncremento;

	if(lNuevaPrio < -20)
		lNuevaPrio = -20;

	if(lNuevaPrio > 19)
		lNuevaPrio = 19;
	

	pstuPCB[ulProcActual].iPrioridad  = lNuevaPrio;

	return 0;

}

/*Función: lFnSysGetpriority
 *Parámetros: 
 *	which (int): indica cual es la prioridad que deseo conocer.
 *	Las opciones son: PRIO_PROCESS, PRIO_PGRP y PRIO_USER (las dos últimas
 *	no están implementadas todavía).
 *	who (int) : id del proceso.
 *Valor de retorno: devuelve un entero largo. Para evitar devolver un número  
 *negativo, se retornan valores en un rango de 40 .. 1, en lugar de -20..19.
 *Esto se logra al restarle a 20 la prioridad del proceso.
 */

long lFnSysGetpriority(int which,int who)
{
	int iCont = 0;
	int iPrio;
	
	if(which > 2 || which < 0)
		return -EINVAL;
	
	if(which == PRIO_PROCESS)
	{
		if(who == 0)
		{	
			iPrio = 20 - pstuPCB[ulProcActual].iPrioridad;
			return iPrio;
		}
		else
		{	/*busca el id en el vector de procesos*/
			for(iCont = 0; iCont < CANTMAXPROCS;iCont++)
			{
			        /*chequea que who sea un indice valido*/
				if(pstuPCB[iCont].ulId == who && 
				pstuPCB[iCont].iEstado != PROC_NO_DEFINIDO &&
				pstuPCB[iCont].iEstado != PROC_ELIMINADO)
				{
				iPrio = 20 - pstuPCB[iCont].iPrioridad;

				return iPrio;
				}
			}
		}
	}
	else
	{
	vFnImprimir("Prioridad de grupos y de usuarios en desarrollo\n");
	}

	/*No se encontró ningún proceso con el id pasado en who*/
	return -ESRCH;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysSetpriority(int which,int who,int iPrioridad)
{
	int iCont;

	if(which > 2 || which < 0)
		return -EINVAL;

	if(iPrioridad < -20)
		iPrioridad = -20;
	if(iPrioridad > 19)
		iPrioridad = 19;

	if(which == PRIO_PROCESS)
	{
		if(who == 0)
		{
		pstuPCB[ulProcActual].iPrioridad = iPrioridad;
		return 0;
		}
		else
		{
			for(iCont = 0; iCont < CANTMAXPROCS;iCont++)
			{
			        /*chequea que who sea un indice valido*/
				if(pstuPCB[iCont].ulId == who && 
				pstuPCB[iCont].iEstado != PROC_NO_DEFINIDO &&
				pstuPCB[iCont].iEstado != PROC_ELIMINADO)
				{
				 pstuPCB[iCont].iPrioridad = iPrioridad;

				return 0;
				}
			}
		}
	}
	else
	{
	vFnImprimir("Prioridad de grupos y de usuarios en desarrollo\n");
	}

	return -ESRCH;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysClone(stuRegs * regs,int flags)
{
	long error;

	if(flags != CLONE_VM && flags != CLONE_PID) 		
		error = -EINVAL;

	
	if(flags == CLONE_VM)
	{
		return iFnClonarProceso();	
	}
	else //elegi CLONE_PID
	{
	
	}

	return error;

}

// ****************     TP 3 - 2007 - Syscalls de tiempo    *****************
//Funciones
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
unsigned uiFnPasarBCD(int iValor){
	unsigned iValorAlto, iValorBajo;
	iValorAlto = iValor /10;
	iValorBajo = iValor - (iValorAlto * 10);
	iValorAlto <<= 4;
	return (iValorAlto |= iValorBajo);
} 
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysSettime(timeval tp){
	// Contiene la info de a�o-mes-dia-hora-minuto-segundo (en ese orden)
	int iFecha[6];
	int iAnioActual = ANIO_INICIO;
	int iCantidadDiasAnio = DIAS_ANIO_INICIO;

	// Vectores que indican la secuencia de dias transcurridos asociados a los
	// meses que pertenecen dependiendo de si el a�o es bisiesto o no
	int iSecuenciaAnioBisiesto[13] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};
	int iSecuenciaAnioNoBisiesto[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
	int i;

	// A partir de los segundos recibidos como parametro
	// obtenemos la cantidad de dias transcurridos
	long liTiempo = tp.tv_sec;
	int iCantidadDiasTranscurridos = liTiempo/86400;

	// Mantenemos la cantidad de segundos sobrantes
	liTiempo -= iCantidadDiasTranscurridos*86400;

	// Seteamos el a�o
	while(iCantidadDiasTranscurridos >= iCantidadDiasAnio)
	{
		iCantidadDiasTranscurridos -= iCantidadDiasAnio;

		iAnioActual ++;
		if (((iAnioActual % 4 == 0) && ((iAnioActual % 100 != 0) || (iAnioActual % 400 == 0))) == 1)
			iCantidadDiasAnio = 366;
		else
			iCantidadDiasAnio = 365;
	}

	// Seteamos el mes y el dia
	if (((iAnioActual % 4 == 0) && ((iAnioActual % 100 != 0) || (iAnioActual % 400 == 0))) == 1){
		for(i = 0; iCantidadDiasTranscurridos >= iSecuenciaAnioBisiesto[i]; i++);
	}

	else{
		for(i = 0; iCantidadDiasTranscurridos >= iSecuenciaAnioNoBisiesto[i]; i++);
	}

	if (((iAnioActual % 4 == 0) && ((iAnioActual % 100 != 0) || (iAnioActual %400 == 0))) == 1)	{
		iCantidadDiasTranscurridos -= iSecuenciaAnioBisiesto[i-1];
	}

	else{
		iCantidadDiasTranscurridos -= iSecuenciaAnioNoBisiesto[i-1];
	}

   // Almacenamos el a�o en el vector temporal
   // Del a�o solo nos interesan los �ltimos 2 digitos (asi lo solicita el CMOS)
   // Analizar problema producido para fechas mayores al 2099.
	iFecha[0] = iAnioActual - ANIO_INICIO;

   // Almacenamos dia y mes en el vector temporal
	iFecha[1] = i;
	iFecha[2] = iCantidadDiasTranscurridos + 1;

	// Almacenamos la hora
	iFecha[3] = liTiempo / 3600;
	liTiempo -= iFecha[3] * 3600;

	// Almacenamos los minutos
	iFecha[4] = liTiempo / 60;
	liTiempo-=iFecha[4] * 60;

	// Almacenamos los segundos
	iFecha[5] = liTiempo;

	outb(9, 0x70);
	outb(uiFnPasarBCD(iFecha[0]), 0x71);
	outb(8, 0x70);
	outb(uiFnPasarBCD(iFecha[1]), 0x71);
	outb(7, 0x70);
	outb(uiFnPasarBCD(iFecha[2]), 0x71);
	outb(4, 0x70);
	outb(uiFnPasarBCD(iFecha[3]), 0x71);
	outb(2, 0x70);
	outb(uiFnPasarBCD(iFecha[4]), 0x71);
	outb(0, 0x70);
	outb(uiFnPasarBCD(iFecha[5]), 0x71);

	// Seteamos la variable  ulTiempo que contiene la hora actual del sistema	
	ulTiempo = tp.tv_sec * 1000 + tp.tv_usec;

	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysStime(time_t *newtime){
	
	timeval timervalTime;
	long lRetorno;

	time_t *tiempo = (time_t *)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)newtime);
	timervalTime.tv_sec = *tiempo;
	timervalTime.tv_usec = 0;

	lRetorno = lFnSysSettime(timervalTime);
	return lRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysTimes(tms *tmsBuffer){
	tms *tmsTiempos = (tms *)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)tmsBuffer);
	tms tmsTiemposReales = pstuPCB[ ulProcActual ].stuTmsTiemposProceso;
	tmsTiempos->tms_utime = tmsTiemposReales.tms_utime / 2;
	tmsTiempos->tms_stime = tmsTiemposReales.tms_stime / 2;
	tmsTiempos->tms_cutime = tmsTiemposReales.tms_cutime / 2;
	tmsTiempos->tms_cstime = tmsTiemposReales.tms_cstime / 2;
	return uliClockTick;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysGettimeofday(timeval *timervalTp, timezone *timezoneTzp){
	timeval *timevalTime = (timeval *)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)timervalTp);
	timezone *timezoneZona = (timezone *)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)timezoneTzp);

	unsigned long ulMilisegundos = ulTiempo;
	timevalTime->tv_sec = ulMilisegundos/1000;
	timevalTime->tv_usec = ulMilisegundos - (timevalTime->tv_sec*1000);
	timevalTime->tv_sec += (iMinuteswest*60);
	timezoneZona->tz_minuteswest = iMinuteswest;
	timezoneZona->tz_dsttime = 0;
	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/

long lFnSysGettimer(int iWhich, itimerval *itimervalValue){
	itimerval *itimervalTimer;
	
	if (iWhich != ITIMER_REAL && iWhich != ITIMER_VIRT && iWhich != ITIMER_PROF)
		return -EINVAL;

	itimervalTimer = (itimerval *)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int) itimervalValue);
	itimervalTimer->it_interval = pstuPCB[ ulProcActual ].timers[iWhich].it_interval;
	itimervalTimer->it_value = pstuPCB[ ulProcActual ].timers[iWhich].it_value;
	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysSettimeofday(timeval *timevalTp, timezone *timezoneTzp){
	timeval timevalTime = *(timeval *)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)timevalTp);
	timezone timezoneZona = *(timezone *)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)timezoneTzp);
	
	iMinuteswest = timezoneZona.tz_minuteswest;
	return lFnSysSettime(timevalTime);
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysSettimer(int which, itimerval const *value, itimerval *ovalue){
	itimerval *timerViejo;
	itimerval *timer;
	if (which != ITIMER_REAL && which != ITIMER_VIRT && which != ITIMER_PROF){
		return -EINVAL;
	}

	timerViejo = (itimerval *)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int) ovalue);
	timer = (itimerval *)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int) value);
	timerViejo->it_interval = pstuPCB[ ulProcActual ].timers[which].it_interval;
	timerViejo->it_value = pstuPCB[ ulProcActual ].timers[which].it_value;
	pstuPCB[ ulProcActual ].timers[which].it_value = timer->it_value;
	pstuPCB[ ulProcActual ].timers[which].it_interval = timer->it_interval;
	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysAdjtimex(timex *timexBuffer){
	timex *timexTimeVars;
	int modo, iNewFreq;
	
	timexTimeVars = (timex *)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int) timexBuffer);
	modo = timexTimeVars->modes;

	if (modo){
		if (modo & ADJ_STATUS){
			return -EINVAL;
			//iRelojEstado = timexTimeVars->status;
		}
		if (modo & ADJ_FREQUENCY){
			return -EINVAL;
			//lRelojFrequencia = timexTimeVars->freq;
		}
		if (modo & ADJ_MAXERROR){
			return -EINVAL;
			//lRelojMaxerror = timexTimeVars->maxerror;
		}
		if (modo & ADJ_ESTERROR){
			return -EINVAL;
			//lRelojEsterror = timexTimeVars->esterror;
		}
		if (modo & ADJ_TIMECONST){
			return -EINVAL;
			//lRelojConstante = timexTimeVars->constant;
		}
		if (modo & ADJ_OFFSET){
			return -EINVAL;
			/*if (modo & ADJ_OFFSET_SINGLESHOT){
			}*/
		}
		if (modo & ADJ_TICK){
			if (timexTimeVars->tick < 1)
				return -EINVAL;
			iNewFreq = (timexTimeVars->tick * TIMER_FREQ_REAL) / 1000;
			vFnImprimir("\nDivisor = %d", iNewFreq);
			vFnIniciarTimer_Asm(iNewFreq);
			iMilisegundosPorTick = timexTimeVars->tick;
			lRelojMaxerror = (((1000000 * iNewFreq) / TIMER_FREQ_REAL) - (iMilisegundosPorTick * 1000) * TIMER_TICKS_UPDATE_DEFAULT) / 1000;
		}
	}

	timexTimeVars->offset = lRelojOffset;
	timexTimeVars->freq = lRelojFrequencia;
	timexTimeVars->maxerror = lRelojMaxerror;
	timexTimeVars->esterror = lRelojEsterror;
	timexTimeVars->constant = lRelojConstante;
	timexTimeVars->precision = PRECISION_RELOJ;
	timexTimeVars->time.tv_sec = ulTiempo / 1000;
	timexTimeVars->time.tv_usec = ulTiempo - (timexTimeVars->time.tv_sec * 1000);
	timexTimeVars->tick = iMilisegundosPorTick;
	
	return iRelojEstado;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysNanosleep(timespec const *timespecRequested_time, timespec *timespecRemaining){
	timespec timespecTiempoRequerido;
	
	timespecTiempoRequerido = *(timespec *)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int) timespecRequested_time);
	
	if (timespecTiempoRequerido.tv_sec < 0 || timespecTiempoRequerido.tv_nsec < 0 || timespecTiempoRequerido.tv_nsec > 999999999)
		return -EINVAL;
	pstuPCB[ ulProcActual ].iEstado = PROC_DETENIDO;
	timespecTiempoRequerido.tv_nsec/= 1000000;
	pstuPCB[ ulProcActual ].lNanosleep = (timespecTiempoRequerido.tv_sec * 1000) + timespecTiempoRequerido.tv_nsec;
	pstuPCB[ ulProcActual ].puRestoDelNanosleep = (unsigned int *)timespecRemaining;
	vFnPlanificador();
	return 0;
}

/*SysCall Planificacion Sodix*/
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysIdle() {
    unsigned int uiVectorDescriptorAuxiliarTarea[2];
    
    vFnImprimirContextSwitch (ROJO, pstuPCB[iTareaNula].ulId,
            pstuPCB[iTareaNula].stNombre,
            pstuPCB[iTareaNula].uiIndiceGDT_TSS);
  
    if (pstuPCB[staiN].iEstado != PROC_ELIMINADO &&
            pstuPCB[staiN].iEstado != PROC_ZOMBIE &&
            pstuPCB[staiN].iEstado != PROC_ESPERANDO &&
            pstuPCB[staiN].iEstado != PROC_DETENIDO) {
                  //paso el proceso que estaba en ejecucion a listo
                  pstuPCB[staiN].iEstado = PROC_LISTO;
    }

    //Paso el IDLE a running
    pstuPCB[iTareaNula].iEstado = PROC_EJECUTANDO;
	
    staiProcesoAnterior = staiN;
    ulProcActual = iTareaNula;

    /* uiVectorDescriptorAuxiliarTarea[0] = offset.
     * Este parametro no hace falta cargarlo porque es ingnorado al momento del
     * salto a un descriptor, lo unico que interesa el el selector en si mismo.
     */

    /* Multiplicamos el indice por 8 para tener el offset en bytes desde el
     * inicio de la gdt hasta el selector que nos interesa */
    uiVectorDescriptorAuxiliarTarea[1]= pstuPCB[iTareaNula].uiIndiceGDT_TSS * 8;

    asm ("clts\t\n" "ljmp *%0": :"m" (*uiVectorDescriptorAuxiliarTarea));

    return 0;
}


/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysSchedSetParam(int p)
{
    if(lFnSysSchedGetScheduler()==FIFO){
	}
	if(lFnSysSchedGetScheduler()==BTS){
		uliBTSQ = p;
	}
	if(lFnSysSchedGetScheduler()==RR){
		uliQuantum = p;
	}
	
	return lFnSysSchedSetScheduler(p);
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysSchedSetScheduler(int p)
{
    if(p==FIFO)//0
    {
    	bPlanificador=FIFO;
       return FIFO;
    }
    if(p==RR)//1
    {
       uliQuantum=QUANTUM;
	   bPlanificador=RR;
       return RR;
    }
    if(p==BTS)//1
    {		
       uliQuantum=QBTS;
       bPlanificador=BTS;
       return BTS;
    }
    return -1;  
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysSchedGetParam()
{
    if(bPlanificador==RR){
       return uliQuantum;
    }
    if(bPlanificador==FIFO){
       return -1;
    }
	if(bPlanificador==BTS){
	   return uliBTSQ;
	}

	return -1;
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysSchedGetScheduler()
{
	return bPlanificador;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysSchedYield()
{
	//llamamos al planificador y ya,
	//esto saca al proceso de ejecucion y pone al siguiente
	vFnPlanificador();
	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysSchedGetPriorityMax()
{
    if(bPlanificador==BTS){
		return 1; //calcular la prioridad para BTS, sin no es, devolver -1
	}
	return -1;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysSchedGetPriorityMin()
{
    if(bPlanificador==BTS){
		return CANTMAXPROCS; //calcular la prioridad para BTS, sin no es, devolver -1
	}
	return -1;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysSchedRrGetInterval()
{
    return uliTimeSlice;
}

/***************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long lFnSysPtrace( int iRequest, void *pvDirParam ) {	
	
	__ptrace_param *pstuParam;

	//void*   pvTssParam;
	stuRegs* pstuTss;
	
	//estructura con los registros del FPU
	stuFpu* pstuFpu;

	/* Necesario para acceder a las variables de un proceso particular (indireccion) */
	pvDirParam = (void *)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)pvDirParam );
	pstuParam = (__ptrace_param *)pvDirParam;

	if ( iFnBuscaPosicionProc(pstuParam->iPid) < 0 || pstuPCB[ iFnBuscaPosicionProc(pstuParam->iPid) ].iEstado == PROC_ZOMBIE ){
		return -ESRCH;
	}

	/* Valido que el pedido a ejecutar tenga los permisos adecuados */
	if ( (iRequest != PTRACE_ATTACH) && (iRequest != PTRACE_TRACEME) ) // 13 y 0 respectivamente
	{
		if ( (pstuPCB[iFnBuscaPosicionProc(pstuParam->iPid)].lPidTracer == PROC_WOTRACER) || (pstuPCB[iFnBuscaPosicionProc(pstuParam->iPid)].lPidTracer != pstuPCB[ulProcActual].ulId) )
		{
			return -EPERM;
		}
	}

	switch ( iRequest ) {
		case( PTRACE_TRACEME ):
			/* Relaciono el Hijo con el Padre usando solo el paràmetro REQUEST */
			if (pstuPCB[ ulProcActual ].lPidTracer == PROC_WOTRACER)
				pstuPCB[ulProcActual].lPidTracer = pstuPCB[ulProcActual].ulParentId;
			else
				return -EPERM;
			break;
		case( PTRACE_PEEKTEXT ):
			pstuParam->pvData = (void*)(*(int*)( pstuPCB[ iFnBuscaPosicionProc(pstuParam->iPid) ].uiDirBase + (unsigned)pstuParam->pvAddr) );
			break;
		case( PTRACE_PEEKDATA ):
			pstuParam->pvData = (void*)(*(int*)( pstuPCB[ iFnBuscaPosicionProc(pstuParam->iPid) ].uiDirBase + (unsigned)pstuParam->pvAddr) );
			break;
		case( PTRACE_PEEKUSER ):
			pstuParam->pvData = (void*)(*(int*)( pstuPCB[ iFnBuscaPosicionProc(pstuParam->iPid) ].uiDirBase + (unsigned)pstuParam->pvAddr) );
			break;
		case( PTRACE_POKETEXT ):
			*(int*)( pstuPCB[ iFnBuscaPosicionProc(pstuParam->iPid) ].uiDirBase + (unsigned)pstuParam->pvAddr ) = (int)pstuParam->pvData;
			break;
		case( PTRACE_POKEDATA ):	
			*(int*)( pstuPCB[ iFnBuscaPosicionProc(pstuParam->iPid) ].uiDirBase + (unsigned)pstuParam->pvAddr ) = (int)pstuParam->pvData;
			break;
		case( PTRACE_POKEUSER ):
			*(int*)( pstuPCB[ iFnBuscaPosicionProc(pstuParam->iPid) ].uiDirBase + (unsigned)pstuParam->pvAddr ) = (int)pstuParam->pvData;
			break;
		case( PTRACE_GETREGS ):
			pstuTss = (stuRegs*)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)pstuParam->pvData );
			pstuTss->eax = stuTSSTablaTareas[ iFnBuscaPosicionProc(pstuParam->iPid) ].eax;
			pstuTss->ebx = stuTSSTablaTareas[ iFnBuscaPosicionProc(pstuParam->iPid) ].ebx;
			pstuTss->ecx = stuTSSTablaTareas[ iFnBuscaPosicionProc(pstuParam->iPid) ].ecx;
			pstuTss->edx = stuTSSTablaTareas[ iFnBuscaPosicionProc(pstuParam->iPid) ].edx;
			break;
		case( PTRACE_GETFPREGS ):
			pstuFpu = (stuFpu*)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)pstuParam->pvData );
			
			/* Si el ultimo proceso en usar la FPU es el proceso a "tracear",
			 * su TSS estara desactualizada
			 * y los valores habra que sacarlos directamente el FPU
			 */
			if( ulUltimoProcesoEnFPU == pstuParam->iPid ) {
			  //Se guarda el estado actual de la FPU, en la TSS del proceso hijo
			  asm volatile(
					  "fnsave %0\n" 
					  "fwait	\n" 
					  "frstor %0\n" 
					  : "=m" (*pstuFpu));
			} else {
				//La TSS del proceso a "tracear" esta actualizada
				//asi que tomamos los valores de la misma
				ucpFnCopiarMemoria( (unsigned char*) pstuFpu,
									(unsigned char*) &stuTSSTablaTareas[iFnBuscaPosicionProc(pstuParam->iPid)].fpu,
									sizeof(stuFpu));
			}
			break;
		case( PTRACE_SETREGS ):	
			pstuTss = (stuRegs*)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)pstuParam->pvData );
			stuTSSTablaTareas[ iFnBuscaPosicionProc(pstuParam->iPid) ].eax = pstuTss->eax;
			stuTSSTablaTareas[ iFnBuscaPosicionProc(pstuParam->iPid) ].ebx = pstuTss->ebx;
			stuTSSTablaTareas[ iFnBuscaPosicionProc(pstuParam->iPid) ].ecx = pstuTss->ecx;
			stuTSSTablaTareas[ iFnBuscaPosicionProc(pstuParam->iPid) ].edx = pstuTss->edx;
			break;
		case( PTRACE_SETFPREGS ):
			pstuFpu = (stuFpu*)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)pstuParam->pvData );
			
			//La TSS del proceso a "tracear" se sobreescribe con los valores recibidos
			ucpFnCopiarMemoria( (unsigned char*) &stuTSSTablaTareas[iFnBuscaPosicionProc(pstuParam->iPid)].fpu,
								(unsigned char*) pstuFpu,
								sizeof(stuFpu));
			
			/* Si el ultimo proceso en utilizar el FPU fue el proc. a "tracear", 			 * se fuerza a que la proxima vez que se use el FPU se tomen
			 * los datos de la TSS recien actualizada
			 */ 
			if( ulUltimoProcesoEnFPU == pstuParam->iPid ) {
				ulUltimoProcesoEnFPU = 0;
			}

			break;
		case( PTRACE_CONT ):
			return lFnSysKill( pstuParam->iPid, SIGCONT);
			break;
		case( PTRACE_KILL ):
			return lFnSysKill( pstuParam->iPid, SIGKILL);
			break;
		case( PTRACE_ATTACH ):
			if ( pstuPCB[ iFnBuscaPosicionProc( pstuParam->iPid ) ].lPidTracer == PROC_WOTRACER )
 				pstuPCB[iFnBuscaPosicionProc(pstuParam->iPid)].lPidTracer = pstuPCB[ulProcActual].ulId;
			else
				return -EPERM;
			break;
		case( PTRACE_DETACH ):
			/* Desvinculo al Hijo del proceso que lo rastrea */
			pstuPCB[iFnBuscaPosicionProc(pstuParam->iPid)].lPidTracer = PROC_WOTRACER;
			break;
		default:
			return -EINVAL;
	}

	return 0l;
}

// ***************************************************************************

