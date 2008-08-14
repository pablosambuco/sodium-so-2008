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
void vFnPlanificadorRR(){
	
  unsigned int uiVectorDescriptorAuxiliarTarea[2];
  int iNUltimaTarea = -1;	/*,iFlags; */

      iNUltimaTarea = staiN;
      staiN++;
      while (staiN < CANTMAXPROCS && pstuPCB[staiN].iEstado != PROC_LISTO)
	{
	  staiN++;
	}
      if (staiN == CANTMAXPROCS)	//si llegamos al final del vector volvemos al primer
	{			//elemento (distinto de la tarea nula)
	  staiN = 1;
	  while (staiN < iNUltimaTarea
		 && pstuPCB[staiN].iEstado != PROC_LISTO)
	    {
	      staiN++;
	    }
	  if (staiN == iNUltimaTarea &&
	      (pstuPCB[staiN].iEstado == PROC_ELIMINADO
	       || pstuPCB[staiN].iEstado == PROC_NO_DEFINIDO))
	    //definitivamente ningun proceso
	    {			//listo para la ejecucion
	      staiN = 0;	//ejecuto la tarea nula
	    }
	}


      if (pstuPCB[staiN].iEstado == PROC_LISTO)
	{
	  if (staiProcesoAnterior != staiN)
	    {
	      vFnImprimirContextSwitch (ROJO, pstuPCB[staiN].ulId,
					pstuPCB[staiN].stNombre,
					pstuPCB[staiN].uiIndiceGDT_TSS);
	      if (pstuPCB[staiProcesoAnterior].iEstado != PROC_ELIMINADO
		  && pstuPCB[staiProcesoAnterior].iEstado != PROC_ZOMBIE
		  && pstuPCB[staiProcesoAnterior].iEstado != PROC_ESPERANDO
		  && pstuPCB[staiProcesoAnterior].iEstado != PROC_DETENIDO
		  && staiProcesoAnterior != -1)
		  	pstuPCB[staiProcesoAnterior].iEstado = PROC_LISTO;	//paso el proceso que estaba en ejecución a listo                
		  pstuPCB[staiN].iEstado = PROC_EJECUTANDO;	//paso al actual de listo a running
	      staiProcesoAnterior = staiN;

	      ulProcActual = staiN;

	      /* uiVectorDescriptorAuxiliarTarea[0] = offset. Este parámetro no hace falta 
	         cargarlo porque es ingnorado al momento del salto a 
	         un descriptor, lo único que interesa el el selector 
	         en sí mismo */

	      uiVectorDescriptorAuxiliarTarea[1] = pstuPCB[staiN].uiIndiceGDT_TSS * 8;	/* multiplicamos el indice por 
											   8 para tener el offset en bytes 
											   desde el inicio de la gdt hasta 
											   el selector que nos interesa */

	    asm ("clts\t\n" "ljmp *%0": :"m" (*uiVectorDescriptorAuxiliarTarea));

	    }
	}
}
