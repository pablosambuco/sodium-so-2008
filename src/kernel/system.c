#include <video.h>
#include <kernel/definiciones.h>
#include <kernel/system.h>
#include <kernel/sched.h>
#include <kernel/system_asm.h>
#include <kernel/puertos.h>
#include <kernel/pcb.h>
#include <kernel/gdt.h>
#include <shell/shell.h>
#include <kernel/registros.h>
#include <kernel/shm.h>
#include <kernel/syscall.h>
#include <usr/reloj.h>

//short int bPlanificador = 0;pasado al system.h

short int siEjecutoSyscall = 0;
extern dword pdwGDT;

extern stuPCB pstuPCB[CANTMAXPROCS];
extern stuTSS stuTSSTablaTareas[CANTMAXPROCS];

extern unsigned long ulProcActual;
extern unsigned int uiUltimoPid;
extern unsigned long ulUltimoProcesoEnFPU;
short int bPlanificador = 1;

// Guarda el tiempo del sistema, representado como la cantidad de 
// milesimas de segundos desde una fecha determinada
unsigned long ulTiempo = 0;
// Guarda la diferencia de tiempo al Meridiano de Greenwich
// expresada en minutos.
int iMinuteswest = 0;

// Indica cuantos milisegundos hay que sumarle a la hora (ulTiempo)
// cada vez que se produce la interrupci�n del timer.
unsigned int iMilisegundosPorTick = TIMER_MILISEGUNDOS_POR_TICK_DEFAULT;
// Indica cuantos ticks de reloj deben pasar antes de sincronizar
// el reloj del SO con el de hard para reducir la dessincronizaci�n
unsigned int uiTicsCorrigeDesvio = TIMER_TICKS_UPDATE_DEFAULT;

long lRelojOffset = 0;
long lRelojFrequencia = 0;
long lRelojMaxerror = TIMER_MAXERROR_DEFAULT;
long lRelojEsterror = 0;
long lRelojConstante = 0;
// Estado del reloj
int iRelojEstado = TIME_OK;

void vFnAsignarFuncionExcepcion (int iNumeroExcepcion,
				 void (*pvFnFuncion) ());

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnMostrarRegistrosvCPU ()
{

  stuGDTDescriptor *pstuGDTDescriptorDescriptor;

  unsigned int eax;
  unsigned int ecx;
  unsigned int edx;
  unsigned int ebx;
  unsigned int esp;
  unsigned int ebp;
  unsigned int esi;
  unsigned int edi;
  vFnImprimir ("\ntr=%x, ss= %x, esp= %x, cs= %x, ds= %x, fs= %x",
	       uiFnSTR (), wFnGetSS (), wFnGetESP (), wFnGetCS (),
	       wFnGetDS (), wFnGetFS (), wFnGetGS ());
asm ("mov %%eax, %0\n\t" "mov %%ecx, %1\n\t" "mov %%edx, %2\n\t" "mov %%ebx, %3\n\t" "mov %%esp, %4\n\t" "mov %%ebp, %5\n\t" "mov %%esi, %6\n\t" "mov %%edi, %7\n\t":"=m" (eax), "=m" (ecx), "=m" (edx), "=m" (ebx), "=m" (esp), "=m" (ebp),
       "=m" (esi),
       "=m" (edi));

  vFnImprimir
    ("\neax= 0x%xh, ecx= 0x%xh, edx= 0x%xh, ebx= 0x%xh\nesp= 0x%xh, ebp= 0x%xh, esi= 0x%xh, edi= 0x%xh",
     eax, ecx, edx, ebx, esp, ebp, esi, edi);

  pstuGDTDescriptorDescriptor =
    &(pstuTablaGdt->stuGdtDescriptorDescs[(uiFnSTR () / 8)]);

  vFnImprimir ("\n\nDescriptor GDT: ");
  vFnImprimir ("\nlimit_low=%x", pstuGDTDescriptorDescriptor->usLimiteBajo);	/* limite bajo 0..15   */

  vFnImprimir ("\nbase_low=%x", pstuGDTDescriptorDescriptor->usBaseBajo);	/* base  bajo 0..15    */
  vFnImprimir ("\nbase_med=%x", pstuGDTDescriptorDescriptor->ucBaseMedio);	/* base  media 16..23   */
  vFnImprimir ("access=%x", pstuGDTDescriptorDescriptor->ucAcesso);
  vFnImprimir ("\nlimit_high=%x", pstuGDTDescriptorDescriptor->bitLimiteAlto);	/* limite alto 16..19   */
  vFnImprimir ("\ngranularity=%x",
	       pstuGDTDescriptorDescriptor->bitGranularidad);
  vFnImprimir ("\nbase_high=%x", pstuGDTDescriptorDescriptor->usBaseAlto);

}

#define ASM_CLI asm volatile ("cli"::)

#define PASAR_A_SELECTOR_DATOS_KERNEL 		\
  __asm__ volatile ( 				\
	"movl	%%ss, %%eax		\n\t"	\
	"movl	%%esp, %%ebx		\n\t"	\
	"pushw	$0x10 			\n\t" 	\
	"popw	%%ss 			\n\t" 	\
	"movl	$0x200000, %%esp	\n\t"	\
	"pushl	%%ebx 			\n\t" 	\
	"pushl	%%eax 			\n\t" 	\
	"pushw	$0x10	 		\n\t" 	\
	"popw	%%ds 			\n\t" 	\
	"pushw	$0x10 			\n\t" 	\
	"popw	%%es 			\n\t" 	\
	"pushw	$0x10 			\n\t" 	\
	"popw	%%fs 			\n\t" 	\
	"pushw	$0x10 			\n\t" 	\
	"popw	%%gs 			\n\t" :: )

#define ASM_HLT asm volatile ("hlt"::)

#define ASM_GETEIP( eip ) 				\
	__asm__ volatile( "movl  (%%esp), %%eax  \n\t"	\
			  "movl 4(%%esp), %%ebx  \n\t"	\
			  "movl	%%eax, %%ds	 \n\t"	\
			  "movl	4(%%ebx), %%eax	 \n\t"	\
			  "pushw $0x10		 \n\t" 	\
			  "popw	%%ds 		 \n\t" 	\
			  "movl	%%eax, %0	 \n\t" 	\
			  : "=m"( eip ) )
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnExcepcionCPU0 ()
{
  long eip;
  ASM_CLI;
  PASAR_A_SELECTOR_DATOS_KERNEL;
  ASM_GETEIP( eip );
  vFnImprimir ("\n \n Error de Division por Cero... (EIP: 0x%x)", eip );
  ASM_HLT;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnExcepcionCPU1 ()
{
  ASM_CLI;
  PASAR_A_SELECTOR_DATOS_KERNEL;
  vFnImprimir ("\nDebug Exception...");
  ASM_HLT;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnExcepcionCPU2 ()
{
  ASM_CLI;
  PASAR_A_SELECTOR_DATOS_KERNEL;
  vFnImprimir ("\nExcepcion Desconocida...");
  ASM_HLT;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnExcepcionCPU3 ()
{
  ASM_CLI;
  PASAR_A_SELECTOR_DATOS_KERNEL;
  vFnImprimir ("\nBreakpoint No Esperado...");
  ASM_HLT;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnExcepcionCPU4 ()
{
  ASM_CLI;
  PASAR_A_SELECTOR_DATOS_KERNEL;
  vFnImprimir ("\nError de Desbordamiento...");
  ASM_HLT;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnExcepcionCPU5 ()
{
  ASM_CLI;
  PASAR_A_SELECTOR_DATOS_KERNEL;
  vFnImprimir ("\nError en Chequeo de Limites...");
  ASM_HLT;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnExcepcionCPU6 ()
{
  ASM_CLI;
  PASAR_A_SELECTOR_DATOS_KERNEL;
  vFnImprimir ("\nOpCode Invalido...");
  ASM_HLT;
}


/**
\brief Atiende la Excepcion de CPU 7 (Primer uso de CPU despues de un TaskSwitch), salva el contexto de la FPU en la TSS del ultimo proceso que la uso
\note Esta funcion es llamada por vFnExcepcionCPU7_Asm
*/
void vFnExcepcionCPU7 () {
    stuTSS * stuTSSProcActual, * stuTSSProcAnterior;
  
    //Antes que nada, limpiamos TS
    asm volatile ("clts \n");

    //Hacemos 'guardado diferido' (lazy saving), solo se guarda el entorno de
    //la FPU cuando OTRO proceso necesita usarla
    if ( ulUltimoProcesoEnFPU != pstuPCB[ulProcActual].ulId ) {
        stuTSSProcActual =
            &stuTSSTablaTareas[ pstuPCB[ulProcActual].ulLugarTSS ];
        stuTSSProcAnterior =
            &stuTSSTablaTareas[pstuPCB[iFnBuscaPosicionProc(ulUltimoProcesoEnFPU)].ulLugarTSS ];

        if( ulUltimoProcesoEnFPU ) //TODO - Los procesos son > 0 ?
        {
            //Se guarda el estado actual de la FPU, en la TSS del proceso que la
            //uso la ultima vez 
            //Que pasa si ya termino?
            asm volatile(
                    "fnsave %0\n"
                    "fwait	\n"
                    : "=m" (stuTSSProcAnterior->fpu) );
        }

        //Se recupera el estado de la FPU del proceso que ejecuta ahora 
        //Si nunca habia usado la FPU, se cargaran los valores por defecto,
        //ya almacenados al crear la TSS del proceso
        asm volatile(
                "frstor %0\n"
                : : "m" (stuTSSProcActual->fpu) );
        
        ulUltimoProcesoEnFPU = pstuPCB[ulProcActual].ulId;
    }
}


/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnExcepcionCPU8 ()
{
  ASM_CLI;
  PASAR_A_SELECTOR_DATOS_KERNEL;
  vFnImprimir ("\nDouble Fault...");
  ASM_HLT;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnExcepcionCPU9 ()
{
  ASM_CLI;
  PASAR_A_SELECTOR_DATOS_KERNEL;
  vFnImprimir ("\nCoprocessor Segment Overrun...");
  ASM_HLT;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnExcepcionCPU10 ()
{
  ASM_CLI;
  PASAR_A_SELECTOR_DATOS_KERNEL;
  vFnImprimir ("\nTSS Invalida...");
  ASM_HLT;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnExcepcionCPU11 ()
{
  ASM_CLI;
  PASAR_A_SELECTOR_DATOS_KERNEL;
  vFnImprimir ("\nSegmento No Presente...");
  ASM_HLT;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnExcepcionCPU12 ()
{
  ASM_CLI;
  PASAR_A_SELECTOR_DATOS_KERNEL;
  vFnImprimir ("\nExcepcion de Stack...");
  ASM_HLT;
}


/**
\brief Atiende la Excepcion de CPU 13 (Excepcion de Proteccion General)
\note Esta funcion es llamada por vFnExcepcionCPU13_Asm
*/
void vFnExcepcionCPU13 () {
    //TODO - Borrar esta variable si no se esta usando
    //int iPCBPadre;
   
    vFnImprimir("\nExcepcion de Proteccion General (ExcepcionCPU13):");
    vFnMostrarRegistrosvCPU ();

    vFnLog("\nExcepcion de Proteccion General (ExcepcionCPU13):");
    vFnLog(" Proceso actual PID=%d \"%s\" ", pstuPCB[ulProcActual].ulId,
            pstuPCB[ulProcActual].stNombre);

    /* TODO - Determinar, si es posible, a que se debe la Excepcion (una
     * violacion de segmento es solo una de las posibles razones) y solo enviar
     * SIGSEGV si corresponde.
     */

    /* Enviamos la senal que corresponde a Segmentation Fault al proceso actual
     * y llamamos al planificador para que retire al proceso actual de la CPU
     */
    lFnSysKill( pstuPCB[ulProcActual].ulId, SIGSEGV );  
    vFnPlanificador();
}


/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnExcepcionCPU14 ()
{
  ASM_CLI;
  PASAR_A_SELECTOR_DATOS_KERNEL;
  vFnImprimir ("\nPage Fault...");
  ASM_HLT;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnExcepcionCPU15 ()
{
  ASM_CLI;
  PASAR_A_SELECTOR_DATOS_KERNEL;
  vFnImprimir ("\nExcepcion Desconocida...");
  ASM_HLT;
}


/**
\brief Atiende la Excepcion de CPU 16 (Excepcion en el uso de la FPU)
\note Esta funcion es llamada por vFnExcepcionCPU16_Asm
*/
void vFnExcepcionCPU16 () {
    u16 usiEstadoFPU;
    u16 usiControlFPU;
  
    //guardamos las palabras de estado y control del FPU
    asm volatile( "fnstsw %0\n"
            "fnstcw %1\n"
            : "=m" (usiEstadoFPU),
            "=m" (usiControlFPU));
    
    //Se limpian las excepciones
    asm volatile( "fnclex" );
  
    //ignoramos las excepciones enmascaradas
    usiEstadoFPU = (~usiControlFPU) & usiEstadoFPU;
    usiEstadoFPU &= 0x003F;
  
    //identificamos el origen de la excepcion e imprimimos un mensaje acorde
    //TODO esto deberia hacerse por el manejador de señales pero hoy en dia solo se estan recibiendo dos parametros
    switch (usiEstadoFPU) {
        case 0x0001:
            vFnImprimir("\nFPU: Error de Operacion Invalida");
            break;  
        case 0x0002:
            vFnImprimir("\nFPU: Error de Operando Denormalizado");
            break;
        case 0x0004:
            vFnImprimir("\nFPU: Error de Division por Cero");
            break;  
        case 0x0008:
            vFnImprimir("\nFPU: Error de Overflow");
            break;
        case 0x0010:
            vFnImprimir("\nFPU: Error de Underflow");
            break;  
        case 0x0020:
            vFnImprimir("\nFPU: Error de Precision");
            break;
        default: 
            vFnImprimir("\nFPU: Error no especificado");
            break;
    } 		  
    
    //Se envia la senial
    lFnSysKill(pstuPCB[ulProcActual].ulId,SIGFPE);  
}


/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnIniciarExcepciones ()
{
  int iN = 17;

  vFnAsignarFuncionExcepcion (0, vFnExcepcionCPU0);
  vFnAsignarFuncionExcepcion (1, vFnExcepcionCPU1);
  vFnAsignarFuncionExcepcion (2, vFnExcepcionCPU2);
  vFnAsignarFuncionExcepcion (3, vFnExcepcionCPU3);
  vFnAsignarFuncionExcepcion (4, vFnExcepcionCPU4);
  vFnAsignarFuncionExcepcion (5, vFnExcepcionCPU5);
  vFnAsignarFuncionExcepcion (6, vFnExcepcionCPU6);
  vFnAsignarFuncionExcepcion (7, vFnExcepcionCPU7_Asm);
  vFnAsignarFuncionExcepcion (8, vFnExcepcionCPU8);
  vFnAsignarFuncionExcepcion (9, vFnExcepcionCPU9);
  vFnAsignarFuncionExcepcion (10, vFnExcepcionCPU10);
  vFnAsignarFuncionExcepcion (11, vFnExcepcionCPU11);
  vFnAsignarFuncionExcepcion (12, vFnExcepcionCPU12);
  vFnAsignarFuncionExcepcion (13, vFnExcepcionCPU13_Asm);
  vFnAsignarFuncionExcepcion (14, vFnExcepcionCPU14);
  vFnAsignarFuncionExcepcion (15, vFnExcepcionCPU15);
  vFnAsignarFuncionExcepcion (16, vFnExcepcionCPU16_Asm);

  for (; iN < 31; iN++)
    {
      vFnAsignarFuncionExcepcion (iN, vFnExcepcionCPU15);
    }
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnAsignarFuncionExcepcion (int iNumeroExcepcion, void (*pvFnFuncion) ())
{
  struct stuDireccion
  {
    unsigned short uwDireccionBaja;
    unsigned short uwDireccionAlta;
  };
  union
  {
    void (*pvFnFuncion) ();
    struct stuDireccion stuDireccionFuncion;
  } unDireccionFuncion;

  unDireccionFuncion.pvFnFuncion = pvFnFuncion;

  pstuIDT->IDTDescriptor[iNumeroExcepcion].InterruptGate.Offset1 =
    unDireccionFuncion.stuDireccionFuncion.uwDireccionBaja;
  pstuIDT->IDTDescriptor[iNumeroExcepcion].InterruptGate.SegmentSelector =
    0x8;
  pstuIDT->IDTDescriptor[iNumeroExcepcion].InterruptGate.Type = 0x70;
  pstuIDT->IDTDescriptor[iNumeroExcepcion].InterruptGate.DPL = 0;
  pstuIDT->IDTDescriptor[iNumeroExcepcion].InterruptGate.P = 1;
  pstuIDT->IDTDescriptor[iNumeroExcepcion].InterruptGate.Offset2 =
    unDireccionFuncion.stuDireccionFuncion.uwDireccionAlta;

}



/*****************************************************************************
* Funcion: vFnIniciarIDT                                                     *
* Descripcion: Inicializa los descriptores de la IDT                         *
* Parametros: Ninguno                                                        *
* Valor devuelto: Ninguno                                                    *
* Ultima Modificacion: 26/05/2004                                            *
******************************************************************************/

void
vFnIniciarIDT ()
{

  int iN;
  void *pvFnFuncion;
  dword dwAux, dwAux2;

  pvFnFuncion = (void *) vFnHandlerGenerico_Asm;
  //pvFnFuncion = (void *)prueba_handler;

  dwAux = (word) ((dword) pvFnFuncion & 0x0000FFFF);
  dwAux2 = (word) ((dword) pvFnFuncion >> 16);

  //Creo los descriptores genericos
  for (iN = 0; iN < sizeof(stuIDT) / sizeof(stuIDTDescriptor); iN++)
    {
      pstuIDT->IDTDescriptor[iN].InterruptGate.Offset1 = dwAux;
      pstuIDT->IDTDescriptor[iN].InterruptGate.SegmentSelector = 0x8;
      pstuIDT->IDTDescriptor[iN].InterruptGate.Type = 0x70;
      pstuIDT->IDTDescriptor[iN].InterruptGate.DPL = 0;
      pstuIDT->IDTDescriptor[iN].InterruptGate.P = 1;
      pstuIDT->IDTDescriptor[iN].InterruptGate.Offset2 = dwAux2;
    }

  //Modifico el descriptor del timer (IRQ0 = INT 0x20) para que apunte
  //al handler correcto
  pvFnFuncion = (void *) vFnHandlerTimer_Asm;
  dwAux = (word) ((dword) pvFnFuncion & 0x0000FFFF);
  dwAux2 = (word) ((dword) pvFnFuncion >> 16);
  pstuIDT->IDTDescriptor[INT_TIMER].InterruptGate.Offset1 = dwAux;
  pstuIDT->IDTDescriptor[INT_TIMER].InterruptGate.Offset2 = dwAux2;

  //Modifico el descriptor del teclado (IRQ1 = INT 0x21) para que apunte
  //al handler correcto
  pvFnFuncion = (void *) vFnHandlerTeclado_Asm;
  dwAux = (word) ((dword) pvFnFuncion & 0x0000FFFF);
  dwAux2 = (word) ((dword) pvFnFuncion >> 16);
  pstuIDT->IDTDescriptor[INT_KEYBOARD].InterruptGate.Offset1 = dwAux;
  pstuIDT->IDTDescriptor[INT_KEYBOARD].InterruptGate.Offset2 = dwAux2;

  //Modifico el descriptor de las syscall (INT 0x80) para que apunte
  //al handler correcto
  pvFnFuncion = (void *) vFnHandlerSyscall_Asm;
  dwAux = (word) ((dword) pvFnFuncion & 0x0000FFFF);
  dwAux2 = (word) ((dword) pvFnFuncion >> 16);
  pstuIDT->IDTDescriptor[INT_SYSCALL].InterruptGate.Offset1 = dwAux;
  pstuIDT->IDTDescriptor[INT_SYSCALL].InterruptGate.Offset2 = dwAux2;

  //Remapeo el PIC para que las IRQ esten a partir de la int 0x20
  vFnRemapearPIC_Asm ();

  //limpio el buffer del teclado antes de comenzar a atender interrupciones.
  vFnIniciarTeclado();
  
  //Seteo el timer para que trabaje en modo 2
  //vFnIniciarTimer_Asm (TIMER_DIVISOR_FREQ_DEFAULT);
  vFnIniciarTimer_Asm (0);
  
  //Seteo el IDTR
  vFnIniciarIDT_Asm (sizeof(stuIDT), pstuIDT);

}


/*****************************************************************************
* Funcion: HandlerGenerico                                                   *
* Descripcion: Handler Generico de las interrupciones                        *
* Parametros: Ninguno                                                        *
* Valor devuelto: Ninguno                                                    *
* Ultima Modificacion:27/05/2004                                             *
******************************************************************************/

void
vFnHandlerGenerico ()
{
  vFnImprimir
    ("\nEl manejo de esta interrupcion todavia no esta desarrollado\n");
}

/*****************************************************************************
* Funcion: lFnHandlerSyscall                                                 *
* Descripcion: Punto de entrada para todos los syscall                       *
* Parametros: Registros capturados por el handler de la int 0x80             *
* Valor devuelto: Lo devuelto por el syscall en si                           *
* Ultima Modificacion:7/11/2006                                              *
******************************************************************************/
long
lFnHandlerSyscall ( long eax, long ebx, long ecx, long edx )
{
/*	vFnLog("\nPid:%d\tT:%d\tHandler_Syscalls. eax=%x, ebx=%x, ecx=%x, edx=%x", 
						lFnSysGetPid(), 
						uliClockTick,
						eax, ebx, ecx, edx);
*/

	if (pstuPCB[ulProcActual].lPidTracer == 1) // si esta attached
	{
				
		vFnLog("El proceso %d (bit: %d, padre: %d) pidio la system call %d.\n",ulProcActual,pstuPCB[ulProcActual].lPidTracer,pstuPCB[ulProcActual].ulParentId,eax);	
		lFnSysKill(pstuPCB[ulProcActual].ulParentId,SIGCONT); //manda una senial al padre
		lFnSysKill(pstuPCB[ulProcActual].ulId,SIGSTOP); //detiene al hijo
	}
						
	switch( eax ){
		case( __NR_exit ):    return lFnSysExit( (int)ebx );
		case( __NR_fork ):    return lFnSysFork();
		case( __NR_test ):    return lFnSysTest( ebx );
		case( __NR_read ):    return lFnSysRead( (int)ebx, 
						      	 (void*)ecx,
							 (size_t)edx );
		case( __NR_write ):   return lFnSysWrite( (int)ebx, 
						      	  (const void*)ecx, 
							  (size_t)edx );
		case( __NR_waitpid ): return lFnSysWaitPid( (int) ebx, 
						      	    (int*) ecx, 
							    (int) edx );
		case( __NR_execve ):  return lFnSysExecve( (const char*) ebx, 
						      	    (char** const) ecx, 
							    (char** const) edx );
		case( __NR_time ):    return lFnSysTime( (long*) ebx );
		case( __NR_getpid ):  return lFnSysGetPid();
		case( __NR_kill ):    return lFnSysKill( (int) ebx, (int) ecx );
		case( __NR_getppid ): return lFnSysGetPPid();
		case( __NR_reboot ):  return lFnSysReboot( (int) ebx );
			
		case( __NR_seminit ):  return lFnSysSemInit( (sem_t *) ebx, (sem_init_params *) ecx);
		case( __NR_sempost ):  return lFnSysSemPost( (sem_t *) ebx );
		case( __NR_semwait ):  return lFnSysSemWait( (sem_t *) ebx );
		case( __NR_semclose ):  return lFnSysSemClose( (sem_t *) ebx );
		case( __NR_shmget ):  return lFnSysShmGet( (key_t) ebx, (size_t) ecx );
		case( __NR_shmat ):  return lFnSysShmAt( (int) ebx, (void *) ecx );
		case( __NR_shmdt ):  return lFnSysShmDt( (int) ebx );
		case( __NR_sumar ):  return iFnSysSumar( (int) ebx,(int) ecx,(int *) edx  );

		case( __NR_setpriority ):  return lFnSysSetpriority( (int) ebx,(int) ecx,(int ) edx  );
		case( __NR_getpriority ):  return lFnSysGetpriority( (int) ebx,(int) ecx);
		case( __NR_nice ):  return lFnSysNice( (int) ebx );
		case( __NR_clone ): return lFnSysClone((stuRegs *) ebx, (int) edx);
			
		case( __NR_stime ):  return lFnSysStime( (time_t *) ebx );
		case( __NR_times ):  return lFnSysTimes( (tms *) ebx );
		case( __NR_gettimeofday ):  return lFnSysGettimeofday( (timeval *) ebx, (timezone *) ecx );
		case( __NR_settimeofday ):  return lFnSysSettimeofday( (timeval *) ebx, (timezone *) ecx );
		case( __NR_gettimer ):  return lFnSysGettimer( (int) ebx, (itimerval *) ecx);
		case( __NR_settimer ):  return lFnSysSettimer( (int) ebx, (itimerval *) ecx, (itimerval*) edx );
		case( __NR_adjtimex ):  return lFnSysAdjtimex( (timex *) ebx );
		case( __NR_nanosleep ):  return lFnSysNanosleep( (timespec *) ebx, (timespec *) ecx );

		/*SysCall planificacion*/
		case( __NR_idle): return lFnSysIdle();	     
		case( __NR_sched_setparam): return lFnSysSchedSetParam( (int)ebx );
		case( __NR_sched_getparam): return lFnSysSchedGetParam(); 
		case( __NR_sched_setscheduler): return lFnSysSchedSetScheduler( (int) ebx);
		case( __NR_sched_getscheduler): return lFnSysSchedGetScheduler(); 
		case( __NR_sched_yield): return lFnSysSchedYield();		      
		case( __NR_sched_get_priority_max): return lFnSysSchedGetPriorityMax ();		      
		case( __NR_sched_get_priority_min): return lFnSysSchedGetPriorityMin ();		      
		case( __NR_sched_rr_get_interval): return lFnSysSchedRrGetInterval ();
  	
  	/*SysCall ptrace*/		      
		case( __NR_ptrace ):  return lFnSysPtrace( (int) ebx, (void *) ecx );	
    
  	/*SysCall brk*/		      
		case( __NR_brk ):  return ulFnSysBrk( (unsigned long) ebx );	
  }
	
	return -ENOSYS;
}

/*****************************************************************************
* Funcion: ConvertirAUnsignedLong                                            *
* Descripcion: Convierte una estructura "timeval" a long sumando los segundos*
* multiplicados por 1000 con los milisegundos                                *
* Parametros:	timevalTiempo	Estructura a transformar en long             *
* Valor devuelto: Un long que representa los milisegundos transcurridos hasta*
* 		  el tiempo indicado en stuTiempo                            *
* Ultima Modificacion:25/10/2007                                             *
******************************************************************************/
long lFnConvertirAUnsignedLong(timeval timevalTiempo){
	long lMilisegs = timevalTiempo.tv_sec * 1000;
	lMilisegs+= timevalTiempo.tv_usec;
	return lMilisegs;
}

/*****************************************************************************
* Funcion: ConvertirATimeval                                                 *
* Descripcion: Convierte un long a la estructura "timeval", separando el long*
* en segundos y milisegundos                                                 *
* Parametros:	lMilisegs	Milisegundos a transformar                   *
* Valor devuelto: Una estructura "timeval" que representa el valor pasado en *
* 		  ulMilisegs en segundos y milisegundos                      *
* Ultima Modificacion:25/10/2007                                             *
******************************************************************************/
timeval timevalFnConvertirATimeval(long lMilisegs){
	timeval timevalTiempo;
	timevalTiempo.tv_sec = lMilisegs / 1000;
	timevalTiempo.tv_usec = lMilisegs - (timevalTiempo.tv_sec * 1000);
	return timevalTiempo;
}

/*****************************************************************************
* Funcion: DecrementarTimer                                                  *
* Descripcion: Decrementa el timer pasado por par�metro en una cantidad que  *
* tambi�n se pasa por par�metro, a su vez, si el timer llega a 0 se envia la *
* se�al correspondiente la cual tambi�n se le pasa por par�metro             *
* Parametros:	pstuProc	La estructrua de la PCB de el proceso        *
* 		iTimer		El "nombre" del timer a modificar            *
* 		iSenial		La se�al a mandar cuando el timer llegue a 0 *
* 		iMilisPorTick	Cantidad de milisegundos a restarle al timer *
* Valor devuelto: Ninguno                                                    *
* Ultima Modificacion:25/10/2007                                             *
******************************************************************************/
void vFnDecrementarTimer(stuPCB *pstuProc, int iTimer, int iSenial, int iMilisPorTick){
    long lTimer = lFnConvertirAUnsignedLong(pstuProc->timers[iTimer].it_value);
    
    if (pstuProc->iEstado>PROC_NO_DEFINIDO && pstuProc->iEstado<PROC_ELIMINADO && lTimer > 0){
      lTimer-= iMilisPorTick;
      if (lTimer <= 0){
        // Enviar la se�al pasada por parametro
        lTimer = lFnConvertirAUnsignedLong(pstuProc->timers[iTimer].it_interval);
	lFnSysKill(pstuProc->ulId, iSenial);
      }
      pstuProc->timers[iTimer].it_value = timevalFnConvertirATimeval(lTimer);
    }

}

/*****************************************************************************
* Funcion: HandlerTimer                                                      *
* Descripcion: Handler para el manejo del timer (interrupcion 0x20)          *
* Parametros: Ninguno                                                        *
* Valor devuelto: Ninguno                                                    *
* Ultima Modificacion:09/09/2004                                             *
******************************************************************************/

void
vFnHandlerTimer ()
{
  int i;
  stuPCB *pstuProc;
  uliClockTick++;

  //##############################################
  ulTiempo+=iMilisegundosPorTick;
  uiTicsCorrigeDesvio--;
  if (uiTicsCorrigeDesvio == 0)
  {
 	vFnInicializarReloj();
	uiTicsCorrigeDesvio = TIMER_TICKS_UPDATE_DEFAULT;
  }
  for(i=0; i<CANTMAXPROCS; i++){
    pstuProc = &pstuPCB[i];
  
    // Decrementa el valor de la variable que setea "nanosleep"
    if (pstuProc->lNanosleep > 0 && pstuProc->iEstado == PROC_DETENIDO && pstuPCB->ulId < uiUltimoPid){
      pstuProc->lNanosleep-=iMilisegundosPorTick;
      if (pstuProc->lNanosleep <= 0){
		      pstuProc->iEstado  = PROC_LISTO;
	      pstuProc->puRestoDelNanosleep = NULL;
      }
    }

    // Decrementa el timer REAL
    vFnDecrementarTimer(pstuProc, ITIMER_REAL, SIGALRM, iMilisegundosPorTick);

    if (ulProcActual == i){ //pstuProc->ulId){
      // Decrementa el timer PROF
      vFnDecrementarTimer(pstuProc, ITIMER_PROF, SIGPROF, iMilisegundosPorTick);

      // Decrementa el timer VIRT
      if (siEjecutoSyscall == 1){
// stuTmsTiemposProceso
        pstuProc->stuTmsTiemposProceso.tms_utime++;
        pstuProc->stuTmsTiemposProceso.tms_stime++;
        vFnDecrementarTimer(pstuProc, ITIMER_VIRT, SIGVTALRM, iMilisegundosPorTick / 2);
  	siEjecutoSyscall = 0;
      } else {
        vFnDecrementarTimer(pstuProc, ITIMER_VIRT, SIGVTALRM, iMilisegundosPorTick);
        pstuProc->stuTmsTiemposProceso.tms_utime+=2;
      }
    }
  }
//#######################################################################
//seccion de planificacion de interrupcion de timer
  uliTimeSlice++;	//ventana de tiempo de ejecucion del proceso

	//vFnImprimirClockTicks(clockTick,60,0,VERDE_CLARO);
 /*  __asm__ ("pushf\n pop %%eax": "=a" (iFlags):);
 vFnImprimir(" %d ",iFlags);*/
 if(bPlanificador==RR){
	if(uliTimeSlice>=uliQuantum){
	  uliTimeSlice=0;	//el quantum se calcula en interrupciones de timer
	  vFnPlanificador();
   }
 }
 if(bPlanificador==BTS){
	if(uliTimeSlice>=uliBTSQ){
	  uliTimeSlice=0;	//el quantum se calcula en interrupciones de timer
	  vFnPlanificador();
   }
 }
	

}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void 
vFnPlanificador()
{
  // Simula la memoria compartida por software(shm)  
  vFnCopiarVariablesCompartidas();
  //funciones de planificadores. Inserte su funcion aqui
  if(bPlanificador==RR){
	  vFnPlanificadorRR();
	  //vFnPlanificadorBTS();
	  return;
  }
  if(bPlanificador==FIFO){
	  uliTimeSlice=0; //reinicio el timeslice para el prox proceso
	  vFnPlanificadorRR();
	  return;
	  //cuando estamos en FIFO, el planificador se llama solo 
	  //cuando una tarea finaliza o pasa a bloqueada
	  //por lo tanto, no necesitamos crear otro planificador
	  //usamos el mismo que RR y ya
  }
  if(bPlanificador==BTS){
	  vFnPlanificadorBTS();
	  return;
  }	

}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnIniciarTeclado()
{
//Esto se deberia ejecutar antes de desenmascarar la atencion del teclado.
ucFnObtenerScanCode();
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
unsigned char ucFnObtenerScanCode()
{
  unsigned char ucScanCode, val;
  unsigned int i;

  ucScanCode = inb(0x60);
  for (i=0;i<1000;i++);
  val = inb(0x61);
  for (i=0;i<1000;i++);
  outb(val|0x80,0x61);
  for (i=0;i<1000;i++);
  outb(val,0x61);
  
  // Comentado deja al log poco entendible
  //vFnLog("\nPid:%d\tT:%d\tScanCode:%x", lFnSysGetPid(), uliClockTick,ucScanCode);
  
return ucScanCode;
}

/*****************************************************************************
* Funcion: vFnHandlerTeclado                                                 *
* Descripcion: Handler para el manejo del teclado (interrupcion 0x21)        *
* Parametros: Ninguno                                                        *
* Valor devuelto: Ninguno                                                    *
* Ultima Modificacion: 06/11/2004                                            *
*****************************************************************************/


void vFnHandlerTeclado ()
{
  vFnManejadorTecladoShell (ucFnObtenerScanCode());
}


