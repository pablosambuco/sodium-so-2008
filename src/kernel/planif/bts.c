#include <video.h>
#include <kernel/definiciones.h>
#include <kernel/system.h>
#include <kernel/sched.h>
#include <kernel/pcb.h>
#include <kernel/gdt.h>

extern unsigned long ulProcActual;
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnPlanificadorBTS()//balance tiempo de espera y servicio
{
    static int stabIniciar=0;
    static int staiServicio=1;
    static int staiEspera=2;
    unsigned int uiVectorDescriptorAuxiliarTarea[2];
      //int iNUltimaTarea = -1;   
    //agregado//
    unsigned long ulC,ulPrioridad,ulMaxP,ulProcesoSeleccionado,uiContadorListos=0;
    unsigned long uliQuantumGeneral;

    int flag=0;    //ulProcesoSeleccionado se puede cambiar por ulProcActual para optimizar,
                //por ahora dejo esta variable por si la otra desaparece o la toketean
    //Pongo el proceso actual de ejecucion a listo (si se lo merece)
   
   // pstuPCB[1].iEstado=PROC_LISTO;
   
    if(stabIniciar==0)//esto se puede optimizar en una funcion ke se llame kuando se activa el uso del BTS
    {                  //junto kon la optimizacion de recorrer la lista de procesos
        uliQuantumGeneral=2;//1 es aprox un micro segundo kon un micro de 200MHZ
        uliQuantum=2;
		pstuPCB[0].iEstado=PROC_LISTO;
        pstuPCB[1].iEstado=PROC_LISTO;
		pstuPCB[2].iEstado=PROC_LISTO;
        stabIniciar=1;
		ulC=0;
    }
	else
		ulC=1;
    if (pstuPCB[staiProcesoAnterior].iEstado != PROC_ELIMINADO &&
        pstuPCB[staiProcesoAnterior].iEstado != PROC_ZOMBIE &&
        pstuPCB[staiProcesoAnterior].iEstado != PROC_ESPERANDO &&
        pstuPCB[staiProcesoAnterior].iEstado != PROC_DETENIDO &&
        staiProcesoAnterior != -1)
        pstuPCB[staiProcesoAnterior].iEstado = PROC_LISTO;    //paso el proceso que estaba en ejecución a listo                       
    else
    {
        pstuPCB[staiProcesoAnterior].ulTiempoServicio=1;
        pstuPCB[staiProcesoAnterior].ulTiempoEspera=1;
    }
    pstuPCB[staiProcesoAnterior].ulTiempoServicio+=uliQuantum;// /staiServicio;
    //Para evitar el overhead se puede crear un vector con los sub indices de los
    //procesos listos     
    for(ulC;ulC<CANTMAXPROCS;ulC++)//busca de todos los procesos listos kual tiene mas prioridad
    {
        if(pstuPCB[ulC].iEstado == PROC_LISTO)//solo aumentara la prioridad progresivamente
                                          //para los listos (que esperen mucho)
        {            
            uiContadorListos++;
            if(staiProcesoAnterior!=ulC)//sino es el proceso que se ejecutaba
                pstuPCB[ulC].ulTiempoEspera+=uliQuantum/staiEspera;//hacer la diferencia de tiempo en vez de 1   
           
            ulPrioridad=pstuPCB[ulC].ulTiempoEspera/(pstuPCB[ulC].ulTiempoServicio+1);
            if(ulPrioridad>ulMaxP || flag==0)//Busca el de mayor prioridad
            {
                flag=1;
                ulMaxP=ulPrioridad;
                ulProcesoSeleccionado=ulC;               
            }           
        }    
    }   
    if(ulProcesoSeleccionado==0 || ulProcesoSeleccionado==1 || ulProcesoSeleccionado==2)
    {
        pstuPCB[ulProcesoSeleccionado].ulTiempoServicio=2*uiContadorListos;//Para decrementar la prioridad
        pstuPCB[ulProcesoSeleccionado].ulTiempoEspera=0;
		//if(ulProcesoSeleccionado==0)
			//pstuPCB[ulProcesoSeleccionado].ulTiempoServicio=5+10*uiContadorListos;
    }   
	uiContadorListos=0;
    if(pstuPCB[ulProcesoSeleccionado].iEstado==PROC_LISTO)
    if (staiProcesoAnterior != ulProcesoSeleccionado)//si es el mismo no hago nada
    {                                                 //(asi estaba en el "RR")

        vFnImprimirContextSwitch (ROJO, pstuPCB[ulProcesoSeleccionado].ulId,
                                  pstuPCB[ulProcesoSeleccionado].stNombre,
                                  pstuPCB[ulProcesoSeleccionado].uiIndiceGDT_TSS);       
       
        pstuPCB[ulProcesoSeleccionado].iEstado = PROC_EJECUTANDO;    //paso al actual de listo a running
       
        staiProcesoAnterior=ulProcesoSeleccionado;//marco al ke se va a ejecutar komo ultimo
                                                  //para la siguiente pasada

        staiN=ulProcActual = ulProcesoSeleccionado;//ulProcActual es global       
        //uiVectorDescriptorAuxiliarTarea[0] = offset.
        //Este parámetro no hace falta
        //cargarlo porque es ingnorado al momento del salto a
        //un descriptor, lo único que interesa el el selector
        //en sí mismo
        uiVectorDescriptorAuxiliarTarea[1] = pstuPCB[ulProcesoSeleccionado].uiIndiceGDT_TSS * 8;   
        //multiplicamos el indice por 8 para tener el offset en bytes
        //desde el inicio de la gdt hasta
        //el selector que nos interesa
        asm ("clts\t\n" "ljmp *%0": :"m" (*uiVectorDescriptorAuxiliarTarea));       
    } 
}
//************ Fin agregado ****************//
