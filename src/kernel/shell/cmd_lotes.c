#include <shell/shell.h>
#include <shell/cmd_lotes.h>
#include <shell/teclado.h>
#include <shell/sys_video.h>
#include <kernel/libk/libk.h>
#include <kernel/libk/string.h>
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnMenuLotes(int iComandoPos)
{
	char strSubCMD[16];
	
  	if ((iFnGetArg (iComandoPos, 1, strSubCMD, sizeof(strSubCMD)) == 1)){
		if(iFnCompararCadenas(strSubCMD,"ayuda")==1){
			vFnSubCmdLoteAyuda();
		} else if(iFnCompararCadenas(strSubCMD,"cargar")==1){
			vFnSubCmdLoteCargar(iComandoPos);
		} else if(iFnCompararCadenas(strSubCMD,"editar")==1){
			vFnSubCmdLoteEditar(iComandoPos);
		} else if(iFnCompararCadenas(strSubCMD,"eliminar")==1){
			vFnSubCmdLoteEliminar(iComandoPos);
		} else if(iFnCompararCadenas(strSubCMD,"guardar")==1){
			vFnSubCmdLoteGuardar(iComandoPos);
		} else if(iFnCompararCadenas(strSubCMD,"iniciar")==1){
			vFnSubCmdLoteIniciar(iComandoPos);
		} else if(iFnCompararCadenas(strSubCMD,"ls")==1){
			vFnSubCmdLoteLs();
		} else if(iFnCompararCadenas(strSubCMD,"mostrar")==1){
			vFnSubCmdLoteMostrar(iComandoPos);
		} else if(iFnCompararCadenas(strSubCMD,"nuevo")==1){
			vFnSubCmdLoteNuevo();
		} else if(iFnCompararCadenas(strSubCMD,"pausar")==1){
			vFnSubCmdLotePausar();		
		} else if(iFnCompararCadenas(strSubCMD,"reanudar")==1){
			vFnSubCmdLoteReanudar();					
		} else {
			vFnImprimir("\n Uso incorrecto, tipee [lote ayuda] o [lote] para ver las opciones disponibles");
		}
	} else {
	    vFnImprimir( "\n uso: lote [subcomando] [parametros]"
			 "\n subcomandos: "
		      	 "\n  ayuda"
		         "\n  cargar [nombre_archivo]"
		         "\n  editar [ID]"
		         "\n  eliminar [ID]"
				 "\n  guardar [ID] [nombre_archivo]"
				 "\n  iniciar [ID]"
				 "\n  ls"		
				 "\n  mostrar [ID]"		
				 "\n  nuevo"
				 "\n  pausar"		
				 "\n  reanudar"		
		         );
	}
}
/******************************************************************************
Funciónes: Subcomandos de lotes
Descripción: permite la creacion de lotes de procesos para ejecutar, cargarlos, 
			editarlos, guardarlos, mostrarlos, pausar y reanudar muestras

Recibe:   posicion de comienzo del comando en el buffer del shell
Devuelve: nada

Fecha última modificación: 09/09/2007
******************************************************************************/
void vFnSubCmdLoteAyuda(){
	vFnImprimir( "\n uso: lote [subcomando] [parametros]"
				 "\n subcomandos: "
		      	 "\n  ayuda"
		         "\n  cargar [nombre_archivo]"
		         "\n  editar [ID]"
		         "\n  eliminar [ID]"
				 "\n  guardar [ID] [nombre_archivo]"
				 "\n  iniciar [ID]"
				 "\n  ls"		
				 "\n  mostrar [ID]"		
				 "\n  nuevo"
				 "\n  pausar"		
				 "\n  reanudar"		
		         );
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLoteCargar(int iComandoPos){
	char strPar[16];
	if(iFnGetArg (iComandoPos, 2, strPar, sizeof(strPar)) == 1){
		vFnImprimir("\nTODO lote cargar");//inserte su comando aqui usando [strPar] como argumento
	}
	else{
		vFnImprimir("\nIngrese el nombre del archivo de lotes!");
	}
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLoteEditar(int iComandoPos){
	char strPar[16];
	if(iFnGetArg (iComandoPos, 2, strPar, sizeof(strPar)) == 1){
		vFnImprimir("\nTODO lote editar");//inserte su comando aqui usando [strPar] como argumento
	}
	else{
		vFnImprimir("\nIngrese el ID del lote!");
	}
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLoteEliminar(int iComandoPos){
	char strPar[16];
	if(iFnGetArg (iComandoPos, 2, strPar, sizeof(strPar)) == 1){
		vFnImprimir("\nTODO lote eliminar");//inserte su comando aqui usando [strPar] como argumento
	}
	else{
		vFnImprimir("\nIngrese el ID del lote!");
	}
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLoteGuardar(int iComandoPos){
	char strId[16];
	char strNombreArch[16];
	if(iFnGetArg (iComandoPos, 2, strId, sizeof(strId)) == 1){
		if(iFnGetArg (iComandoPos, 3, strNombreArch, sizeof(strNombreArch)) == 1){
			vFnImprimir("\nId lote: %s Guardar en: %s",strId,strNombreArch);
			vFnImprimir("\nTODO lote guardar");//inserte su comando aqui usando [strId] y [strNombreArch] como argumento
		}
		else{
			vFnImprimir("\nIngrese el nombre de archivo donde guardar del lote!");
		}
	}
	else{
		vFnImprimir("\nIngrese el ID del lote!");
	}
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLoteIniciar(int iComandoPos){
	char strPar[16];
	if(iFnGetArg (iComandoPos, 2, strPar, sizeof(strPar)) == 1){
		vFnImprimir("\nTODO lote iniciar");//inserte su comando aqui usando [strPar] como argumento
	}
	else{
		vFnImprimir("\nIngrese el ID del lote!");
	}
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLoteLs(){
	vFnImprimir("\nTODO Lote LS");//inserte su comando aqui
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLoteMostrar(int iComandoPos){
	char strPar[16];
	if(iFnGetArg (iComandoPos, 2, strPar, sizeof(strPar)) == 1){
		vFnImprimir("\nTODO lote eliminar");//inserte su comando aqui usando [strPar] como argumento
	}
	else{
		vFnImprimir("\nIngrese el ID del lote!");
	}
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLoteNuevo(){
	vFnImprimir("\nTODO Lote Nuevo");//inserte su comando aqui
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLotePausar(){
	vFnImprimir("\nTODO Lote pausar");//inserte su comando aqui
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLoteReanudar(){
	vFnImprimir("\nTODO Lote reanudar");//inserte su comando aqui
}

