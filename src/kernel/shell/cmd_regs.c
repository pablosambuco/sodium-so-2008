#include <kernel/definiciones.h>
#include <kernel/system.h>
#include <kernel/system_asm.h>
#include <kernel/pcb.h>
#include <kernel/gdt.h>
#include <kernel/libk/string.h>

#include <shell/shell.h>
#include <shell/teclado.h>
#include <shell/sys_video.h>
#include <video.h>

extern stuTSS stuTSSTablaTareas[CANTMAXPROCS];
extern stuPCB pstuPCB[CANTMAXPROCS];
extern dword pdwGDT;
extern stuEstructuraGdt *pstuTablaGdt;

/******************************************************************************
Función: vFnMenuPCB
Descripción: 	Muestra el estado de la PCB del proceso indicado.
		Para ello antes lo detiene. Luego de mostrar todo lo devuelve
		Al estado anterior. 

Recibe:   *Posición inicial del comando en el buffer de Caracteres. Con esta
           información se podrán recuperar los parámetros pasados desde la
           línea de comandos.
Devuelve: *Nada por variables, el estado de finalización se puede leer desde
          la pantalla.

Fecha última modificación: 11/11/2007
*******************************************************************************/
void vFnMenuPCB(iComandoPos)
{
	int iPid, iTSS, iPCB;
	int iN, iEstadoAnterior;
	char stArg1[6];
	dword *pdwVolcado;

	if (iFnGetArg(iComandoPos, 1, stArg1, 4) == 1) {
		iPid = iFnXtoi(stArg1);
	} else {
		vFnImprimir
		    ("\nError, argumento(s) incorrecto(s).\nForma de uso: Cmd>regs [pcb]");
		return;
	}

	iPCB = iFnBuscaPosicionProc(iPid);
	if (iPCB < 0) {
		vFnImprimir ("\nEl pid indicado no pertenece a un abonado en servicio...");
		return;
	}

	iEstadoAnterior = pstuPCB[iPCB].iEstado;
	vFnImprimir ("\nIntentando detener al Proceso %d", iPid);
	if (iEstadoAnterior != PROC_EJECUTANDO) {
		pstuPCB[iPCB].iEstado = PROC_DETENIDO;
		vFnImprimirOk(55);

	} else {
		vFnImprimirNOk(55);
		vFnImprimir ("\nJejeje... Hablando del principio de incertidumbre...");
		return;
	}
  
	vFnImprimir (	"\nMostrando contenido del PCB:");
	
	vFnImprimir (	"\n ulId:\t\t\t%d"		\
			"\n uiIndiceGDT_CS:\t%d"	\
			"\n uiIndiceGDT_DS:\t%d"	\
			"\n uiIndiceGDT_TSS:\t%d"	\
			"\n vFnFuncion():\t\t%x"	\
			"\n ulParentId:\t\t%d"		\
			"\n ulUsuarioId:\t\t%d"		\
			"\n iPrioridad:\t\t%d"		\
			"\n iEstado:\t\t%d"		\
			"\n lNHijos:\t\t%d"		\
			"\n iExitStatus:\t\t%d"		\
			"\n ulLugarTSS:\t\t%d"		\
			"\n stNombre[25]:\t\t%s"	\
			"\n uiTamProc:\t\t%d"		\
			"\n pstuTablaPaginacion:\t0x%x"	\
			"\n uiDirBase:\t\t0x%x"		\
			"\n uiLimite:\t\t0x%x"		\
			"\n pstPcbSiguiente:\t0x%x",	\
			pstuPCB[iPCB].ulId,	 	\
			pstuPCB[iPCB].uiIndiceGDT_CS, 	\
			pstuPCB[iPCB].uiIndiceGDT_DS, 	\
			pstuPCB[iPCB].uiIndiceGDT_TSS,	\
			pstuPCB[iPCB].vFnFuncion,	\
			pstuPCB[iPCB].ulParentId, 	\
			pstuPCB[iPCB].ulUsuarioId,	\
			pstuPCB[iPCB].iPrioridad,	\
			pstuPCB[iPCB].iEstado,		\
			pstuPCB[iPCB].lNHijos,		\
			pstuPCB[iPCB].iExitStatus,	\
			pstuPCB[iPCB].ulLugarTSS,	\
			pstuPCB[iPCB].stNombre,		\
			pstuPCB[iPCB].uiTamProc,	\
			pstuPCB[iPCB].pstuTablaPaginacion,	\
			pstuPCB[iPCB].uiDirBase,	\
			pstuPCB[iPCB].uiLimite,		\
			pstuPCB[iPCB].pstuPcbSiguiente);

	vFnImprimir ("\nReanudando Proceso %d", iPid);
	pstuPCB[iPCB].iEstado = iEstadoAnterior;
	vFnImprimirOk(55);
}



