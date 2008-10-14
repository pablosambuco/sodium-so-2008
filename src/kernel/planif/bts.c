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




/* PARCHE 13/10/2008 */
/* Al parecer, a veces se presenta la situacion en la que dos hilos de ejecucion
 * del kernel invocan a esta funcion, en la forma de una condicion de carrera,
 * volviendo inestable al sistema.
 *
 * Se observo que cuando se esta ejecutando esta funcion (la llamaremos
 * planificador1) sin que halla sido la interrupcion del reloj del sistema la
 * que causo su invocacion, se puede producir una interrupcion del reloj, la
 * cual, si es mas prioritaria que la tarea actual, se atendera inmediatamente.
 * La rutina de atencion del reloj podra llamar a esta funcion (la llamaremos
 * planificador2), pasando la proxima tarea LISTA a EJECUTANDO. Cuando la
 * ejecucion vuelva a planificador1, si esta ya habia seleccionado el proceso
 * para pasar a EJECUTANDO (el cual sera el mismo si el algoritmo del
 * planificador es deterministico), se intentara poner en ejecucion un proceso
 * que ya esta ejecutando (bit BUSY de la TSS del proceso en 1), generando una
 * GPE (Excepcion de Proteccion General, 13).
 *
 * Nuestro parche consite en poner en 0 el bit BUSY de la TSS del proceso al que
 * saltemos (no importando como estaba antes).
 *
 * TODO: IMPLEMENTAR UNA BUENA SOLUCION
 * ESTA NO ES LA SOLUCION OPTIMA, solamente es un parche (mejor seria evitar las
 * condiciones de carrera en la invocacion del planificador)
 */

// Ponemos el NOVENO bit del SEGUNDO DOUBLE-WORD la TSS del nuevo proceso (BUSY)
// EN 0 (no-ocupado)
*( (long int*) (& ( pstuTablaGdt->stuGdtDescriptorDescs[
                            pstuPCB[staiN].uiIndiceGDT_TSS] ) ) + 1 ) &=
                                                                    0xFFFFFDFF; 
/* FIN PARCHE 13/10/2008 */



            asm ("clts\t\n" "ljmp *%0": :"m"(*uiVectorDescriptorAuxiliarTarea));
        }
    }
}
//************ Fin agregado ****************//
