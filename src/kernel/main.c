#include <video.h>
#include <kernel/pcb.h>
#include <kernel/gdt.h>
#include <kernel/sched.h>
#include <kernel/definiciones.h>
#include <kernel/mem/memoria_s.h>
#include <kernel/mem/memoria_k.h>
#include <kernel/mem/paginas.h>
#include <kernel/syscall.h>
#include <kernel/system.h>
#include <shell/shell.h>
#include <shell/teclado.h>
#include <usr/reloj.h>
#include <kernel/semaforo.h>
#include <fs/ramfs.h>

unsigned int uiTamanioMemoria;
unsigned int uiTamanioMemoriaBaja;
unsigned int uiTamanioMemoriaBios;
unsigned int uiUnidadBoot;
unsigned int uiTamanioBSS;
unsigned int uiTamanioKernel;
unsigned int uiModoMemoria;

extern stuPCB pstuPCB[CANTMAXPROCS];

extern short int bPlanificador;
extern short int bActivarTeclado;

//Puntero a la GDT (Global Descriptor Table)
dword pdwGDT;

//Puntero a la IDT (Interrupt Descriptor Table)
//stuIDT *pstuIdtIDT;

unsigned long ulMemoriaBase = MEM_BASE;
unsigned long ulMemoriaTope = MEM_TOPE;


// void vFnTareaNula (); ahora es proceso usuario
int iFnProcShell ();
int iFnSistema ();
int iFnProceso1 ();

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int
main ()
{
  uliClockTick = 0;
  asm ("cli");

  vFnImprimir ("\nBienvenidos al SODIUM...\n");

  vFnImprimir ("\nModelo de memoria elegido: ");
  if (uiModoMemoria == MODOPAGINADO)
    vFnImprimir ("Paginado	");
  else
    vFnImprimir ("Segmentado	");
  vFnImprimirOk (55);

  vFnImprimir ("\nIniciando IDT ...                           ");
  vFnIniciarIDT ();
  vFnImprimirOk (55);

  vFnImprimir ("\nIniciando el kernel heap ...                ");
  vFnIniciarKMem ();
  vFnImprimirOk (55);
  
  vFnImprimir ("\nIniciando los discos RAM ...                ");
  if (iFnRamFsInit()==0)
	  vFnImprimirOk (55);
  else
	  vFnImprimirNOk(55);

  vFnImprimir ("\nIniciando los Manejadores de Excepciones ...");
  vFnIniciarExcepciones ();
  vFnImprimirOk (55);

  vFnImprimir ("\nIniciando Tablas de Memoria ...              ");

  if (uiModoMemoria == MODOPAGINADO)
    vFnIniciarMapaBits (ulMemoriaBase, ulMemoriaTope);	//Paginado
/* La memoria segmentada ya no se simula, por lo que no hace falta esto:
    else
    vFnParticionarMemoria ();	//Segmentado
 */
  vFnImprimirOk (55);

  vFnImprimir ("\nIniciando GDT ...                           ");
  vFnGdtInicializar (pdwGDT);
  vFnImprimirOk (55);


  vFnImprimir ("\nActivando Manejador del teclado ...         ");
  bActivarTeclado = 1;
  vFnImprimirOk (55);

  vFnImprimir ("\nActivando Planificador de tareas (RR) ...   ");
  lFnSysSchedSetScheduler(RR);
  vFnImprimirOk (55);

  vFnImprimir ("\nSeteando parametros de planificador RR ...  ");
  lFnSysSchedSetParam(QUANTUM);
  uliTimeSlice=0; //inicializo variable para contar tiempos
  vFnImprimirOk (55);

  vFnImprimir ("\nIniciando Tarea Nula ...                    ");
  iFnInstanciarIdle();
  vFnImprimirOk (55);
	
  vFnImprimir ("\nIniciando Proceso Shell ...                 ");
  iFnNuevaTareaEspecial (iFnProcShell, "PR_SHELL................");
  vFnImprimirOk (55);
  
  vFnImprimir ("\nIniciando Variables de Entorno ...       ");
  vFnInicializarEnv();
  vFnImprimirOk (55);

  vFnImprimir ("\nInicializar Memoria Compartida...           ");
  vFnInicializarShms(); /*Inicia el sistema de memoria compartida*/
  vFnImprimirOk (55);

  vFnImprimir ("\nInicializar Semaforos ...                   ");
  vFnInicializarSemaforos(); /*Inicia el sistema de semaforos*/
  vFnImprimirOk (55);

  vFnImprimir ("\nIniciando reloj del sistema ...          ");
  vFnInicializarReloj();
  vFnImprimirOk (55);

  iFnNuevaTareaEspecial (vFnLoopReloj, "PR_Reloj............");
  vFnImprimir ("\nIniciando Proceso Reloj ...                 ");
  vFnImprimirOk (55);

  vFnImprimir ("\nIniciando Task Register ...                 ");
  vFnLTR ((unsigned int) pstuPCB[0].uiIndiceGDT_TSS * 8);
  vFnImprimirOk (55);

  vFnImprimir ("\nMemoria total del sistema: %d MB",
	       uiTamanioMemoriaBios / 1024 / 1024);
  vFnImprimir ("\nHabilitando Interrupciones ...              ");

  asm ("sti");

  vFnImprimirOk (55);


  while (1)
    {
    }

}


/**
 * @brief Simula un proceso de sistema
 * @param pid del proceso
 * @date 21/09/2004
 */
int iFnSistema (int iPid) {
    /*
    unsigned long int uliTiempoAlarma = uliClockTick + 200;
    
    while (1) {
        if (uliClockTick > uliTiempoAlarma)	{
            uliTiempoAlarma = uliClockTick + 200;
        }
    }
    */
    while (1);

    return (1);
}


/**
 * @brief Simula un proceso del usuario
 * @param pid del proceso
 * @date 21/09/2004
 */
int iFnProceso1 (int iPid) {
    unsigned long int uliTiempoAlarma = uliClockTick + 1;
    unsigned long int uliNroBucles = 0;
    int iPCB;
  
    iPCB=iFnBuscaPosicionProc(iPid);
    while (1) {
        if (uliClockTick > uliTiempoAlarma)	{
            /* Envia un mensaje a pantalla por cada clocktick transcurrido
             * (generalmente son bastantes mas de 1, debido a la multitarea)
             */
            uliTiempoAlarma = uliClockTick + 1; 
      
            vFnImprimirVentana (HWND_PROCESOS,
                    "\ngetpid(): %d, getppid(): %d, %s. ClockTicks=%d, "
                    "MBucles.Seg=%d (aprox)",
                    lFnSysGetPid(), 
                    lFnSysGetPPid(), 
                    pstuPCB[iPCB].stNombre, 
                    uliClockTick,
                    (uliNroBucles * 18 + uliNroBucles / 10 * 2)/1000000);
      
            uliNroBucles = 0;
        }
        uliNroBucles++;      
    }
  return (1);
}


/***************************************************************************
 Funcion: prShell
 Descripcion: El shell corre como un proceso independiente. Ejecuta un
 loop principal que va obteniendo caracteres del buffer del teclado mediante
 un getchar bloqueante en espera activa. A su vez el shell va almacenando
 los caracteres que recupera en su propio buffer, que interpretará luego de
 encontrar el caracter 13.

 Parametros: Pid del proceso
 Valor devuelto: Ninguno

 Ultima modificacion: 09/04/2006
***************************************************************************/
int
iFnProcShell (int iPid)
{ char* strTexto={"\nProceso Shell iniciado, PID=%d"};

  if(iFnCambiaTecladoS("us") != 0)
     iFnCambiaTecladoI(1);

  vFnImprimir (strTexto, iPid);
  vFnImprimirContexto ();
  vFnImprimirPrompt ();
  vFnLoopPrincipalShell ();

  return (1);
}
