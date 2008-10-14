#include <video.h>
#include <kernel/definiciones.h>
#include <kernel/system.h>
#include <kernel/sched.h>
#include <kernel/pcb.h>
#include <kernel/gdt.h>

extern unsigned long ulProcActual;

/**
 /brief Planificador Round - Robin
*/
void vFnPlanificadorRR() {

    unsigned int uiVectorDescriptorAuxiliarTarea[2];
    int iNUltimaTarea = -1;

    iNUltimaTarea = staiN;
    staiN++;

    while (staiN < CANTMAXPROCS && pstuPCB[staiN].iEstado != PROC_LISTO) {
	    staiN++;
    }

    // Si llegamos al final del vector volvemos al primer elemento (distinto de
    // la tarea nula)
    if (staiN == CANTMAXPROCS) { 
    	staiN = 1;
    	while (staiN < iNUltimaTarea && pstuPCB[staiN].iEstado != PROC_LISTO) {
    	    staiN++;
        }
    
        if (staiN == iNUltimaTarea &&
            (pstuPCB[staiN].iEstado == PROC_ELIMINADO ||
             pstuPCB[staiN].iEstado == PROC_NO_DEFINIDO) ) {
                // Definitivamente ningun proceso listo para la ejecucion
                staiN = 0;  //Ejecuto la tarea nula
        }
    }

    if (pstuPCB[staiN].iEstado == PROC_LISTO) {
    	if (staiProcesoAnterior != staiN) {
    	    vFnImprimirContextSwitch(ROJO, pstuPCB[staiN].ulId,
                    pstuPCB[staiN].stNombre,
                    pstuPCB[staiN].uiIndiceGDT_TSS);

            if (pstuPCB[staiProcesoAnterior].iEstado != PROC_ELIMINADO &&
                pstuPCB[staiProcesoAnterior].iEstado != PROC_ZOMBIE &&
                pstuPCB[staiProcesoAnterior].iEstado != PROC_ESPERANDO &&
                pstuPCB[staiProcesoAnterior].iEstado != PROC_DETENIDO &&
                staiProcesoAnterior != -1) {
                    //paso el proceso que estaba en ejecución a listo 
                    pstuPCB[staiProcesoAnterior].iEstado = PROC_LISTO;
            }

            //paso al actual de listo a running
            pstuPCB[staiN].iEstado = PROC_EJECUTANDO;
            staiProcesoAnterior = staiN;

            ulProcActual = staiN;

            /* uiVectorDescriptorAuxiliarTarea[0] = offset.
             * Este parametro no hace falta cargarlo porque es ingnorado al
             * momento del salto a un descriptor, lo unico que interesa el el
             * selector en si mismo */

            /* multiplicamos el indice por 8 para tener el offset en bytes desde
             * el inicio de la gdt hasta el selector que nos interesa */
            uiVectorDescriptorAuxiliarTarea[1] =
                pstuPCB[staiN].uiIndiceGDT_TSS * 8;




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



            asm("clts\t\n" "ljmp *%0": :"m"(*uiVectorDescriptorAuxiliarTarea));
        }
    }
}
