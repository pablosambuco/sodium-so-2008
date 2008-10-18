#include <video.h>
#include <kernel/definiciones.h>
#include <kernel/system.h>
#include <kernel/sched.h>
#include <kernel/pcb.h>
#include <kernel/gdt.h>

extern unsigned long ulProcActual;

/**
/brief Planificador Round - Robin
/date 16/10/2008
*/
void vFnPlanificadorRR() {
    unsigned int uiVectorDescriptorAuxiliarTarea[2];
    int iNUltimaTarea = -1;

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

            asm("clts\t\n" "ljmp *%0": :"m"(*uiVectorDescriptorAuxiliarTarea));

        }
    }

    //Habilitacion de interrupciones
    //Si estaban habilitadas, se vuelven a habilitar
    if (bInterrupcionesHabilitadas)
      __asm__ ("sti"::);
}
