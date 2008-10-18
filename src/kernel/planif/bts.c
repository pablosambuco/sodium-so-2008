#include <video.h>
#include <kernel/definiciones.h>
#include <kernel/system.h>
#include <kernel/sched.h>
#include <kernel/pcb.h>
#include <kernel/gdt.h>

extern unsigned long ulProcActual;


/**
 * \brief Planificador BTS (Balance Tiempo de espera y Servicio)
 */
void vFnPlanificadorBTS ()
{
    static int stabIniciar = 0;
    static int staiEspera = 2;
    unsigned int uiVectorDescriptorAuxiliarTarea[2];

    //static int staiServicio=1; //No se esta usando 
    //int iNUltimaTarea = -1;   
    
    //agregado//
    unsigned long ulC, ulPrioridad, ulMaxP, ulProcesoSeleccionado,
	uiContadorListos = 0;
    unsigned long uliQuantumGeneral;

    int flag = 0;
    //ulProcesoSeleccionado se puede cambiar por ulProcActual para optimizar,

    unsigned short int bInterrupcionesHabilitadas = 0;
    int iFlags;

    /* 16/10/2008 - Se agrega la deshablitacion de interrupciones
     *
     * Si no se deshabiltian las interrupciones, puede darse el caso en que
     * mientras se ejecuta el planificador, se presente una interrupcion que
     * tambien llame al planificador. Ambos planificadores llamaran intentaran
     * poner en ejecucion a la misma tarea; cuando el segundo de ellos lo haga, 
     * causara una Excepcion de Proteccion General, por querer ejecutar una
     * tarea marcada como ocupada.
     * Al deshabilitar las interrupciones se evitan estos problemas.
     */

    //Deshabilitacion de interrupciones
    __asm__ ("pushf\n pop %%eax": "=a" (iFlags):);
    // si estaban habilitadas, aqui se deshabilitan
    if (iFlags & 0x200){
        __asm__ ("cli"::);
        bInterrupcionesHabilitadas = 1;
    }


    //por ahora dejo esta variable por si la otra desaparece o la toketean
    //Pongo el proceso actual de ejecucion a listo (si se lo merece)

    // pstuPCB[1].iEstado=PROC_LISTO;

    // Esto se puede optimizar en una funcion que se llame cuando se activa el
    // uso del BTS junto kon la optimizacion de recorrer la lista de procesos
    if (stabIniciar == 0)
    {
        //1 es aprox un micro segundo kon un micro de 200MHZ
    	uliQuantumGeneral = 2;
    	uliQuantum = 2;
    	pstuPCB[0].iEstado = PROC_LISTO;
    	pstuPCB[1].iEstado = PROC_LISTO;
    	pstuPCB[2].iEstado = PROC_LISTO;
	    stabIniciar = 1;
    	ulC = 0;
    } else {
    	ulC = 1;
    }

    if (pstuPCB[staiProcesoAnterior].iEstado != PROC_ELIMINADO &&
       	pstuPCB[staiProcesoAnterior].iEstado != PROC_ZOMBIE &&
       	pstuPCB[staiProcesoAnterior].iEstado != PROC_ESPERANDO &&
       	pstuPCB[staiProcesoAnterior].iEstado != PROC_DETENIDO &&
      	staiProcesoAnterior != -1) {
            //paso el proceso que estaba en ejecución a listo
    	    pstuPCB[staiProcesoAnterior].iEstado = PROC_LISTO;
    } else {
        pstuPCB[staiProcesoAnterior].ulTiempoServicio = 1;
        pstuPCB[staiProcesoAnterior].ulTiempoEspera = 1;
    }

    pstuPCB[staiProcesoAnterior].ulTiempoServicio += uliQuantum; // /staiServicio;

    //Para evitar el overhead se puede crear un vector con los sub indices de
    //los procesos listos     
    
    //Busca de todos los procesos listos kual tiene mas prioridad
    while (ulC < CANTMAXPROCS) {
        // Solo aumentara la prioridad progresivamente para los listos (que
        // esperen mucho)
        if (pstuPCB[ulC].iEstado == PROC_LISTO) {
            uiContadorListos++;
            if (staiProcesoAnterior != ulC) {
                //sino es el proceso que se ejecutaba
                pstuPCB[ulC].ulTiempoEspera += uliQuantum / staiEspera;
                //hacer la diferencia de tiempo en vez de 1   
            }

            ulPrioridad = pstuPCB[ulC].ulTiempoEspera /
                                        (pstuPCB[ulC].ulTiempoServicio + 1);
    
            //Busca el de mayor prioridad
            if (ulPrioridad > ulMaxP || flag == 0) {
                flag = 1;
                ulMaxP = ulPrioridad;
                ulProcesoSeleccionado = ulC;
            }
        }
        ulC++;
    }
    if (ulProcesoSeleccionado == 0 ||
            ulProcesoSeleccionado == 1 ||
            ulProcesoSeleccionado == 2) {
        
        //Para decrementar la prioridad
        pstuPCB[ulProcesoSeleccionado].ulTiempoServicio = 2 * uiContadorListos;	
        pstuPCB[ulProcesoSeleccionado].ulTiempoEspera = 0;
    
        //if(ulProcesoSeleccionado==0)
        //pstuPCB[ulProcesoSeleccionado].ulTiempoServicio=5+10*uiContadorListos
    }

    uiContadorListos = 0;
    if (pstuPCB[ulProcesoSeleccionado].iEstado == PROC_LISTO) {
    	if (staiProcesoAnterior != ulProcesoSeleccionado) {
            //si es el mismo no hago nada
            //(asi estaba en el "RR")

    	    vFnImprimirContextSwitch (ROJO,
                    pstuPCB[ulProcesoSeleccionado].ulId,
                    pstuPCB[ulProcesoSeleccionado].stNombre,
                    pstuPCB[ulProcesoSeleccionado].uiIndiceGDT_TSS);
    
            //paso al actual de listo a running
            pstuPCB[ulProcesoSeleccionado].iEstado = PROC_EJECUTANDO;

            //marco al ke se va a ejecutar komo ultimo para la siguiente pasada
            staiProcesoAnterior = ulProcesoSeleccionado;

            //ulProcActual es global       
            staiN = ulProcActual = ulProcesoSeleccionado;
            
            /* uiVectorDescriptorAuxiliarTarea[0] = offset.
             * Este parámetro no hace falta cargarlo porque es ingnorado al
             * momento del salto a un descriptor, lo unico que interesa el el
             * selector en sí mismo.
             */

            /* Multiplicamos el indice por 8 para tener el offset en bytes desde
             * el inicio de la gdt hasta el selector que nos interesa */
            uiVectorDescriptorAuxiliarTarea[1] =
                pstuPCB[ulProcesoSeleccionado].uiIndiceGDT_TSS * 8;

            asm ("clts\t\n" "ljmp *%0": :"m"(*uiVectorDescriptorAuxiliarTarea));
        }
    }

    //Habilitacion de interrupciones
    //Si estaban habilitadas, se vuelven a habilitar
    if (bInterrupcionesHabilitadas)
      __asm__ ("sti"::);
}
//************ Fin agregado ****************//
