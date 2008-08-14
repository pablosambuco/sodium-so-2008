#include <shell/shell.h>
#include <shell/cmd_log.h>
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
void vFnMenuLog(int iComandoPos)
{
char strSubCMD[16];
	
  	if ((iFnGetArg (iComandoPos, 1, strSubCMD, sizeof(strSubCMD)) == 1)){
		if(iFnCompararCadenas(strSubCMD,"ayuda")==1){
			vFnSubCmdLogAyuda();
		} else if(iFnCompararCadenas(strSubCMD,"eliminar")==1){
			vFnSubCmdLogEliminar();
		} else if(iFnCompararCadenas(strSubCMD,"finalizar")==1){
			vFnSubCmdLogFinalizar();
		} else if(iFnCompararCadenas(strSubCMD,"guardar")==1){
			vFnSubCmdLogGuardar(iComandoPos);
		} else if(iFnCompararCadenas(strSubCMD,"iniciar")==1){
			vFnSubCmdLogIniciar();
		} else if(iFnCompararCadenas(strSubCMD,"ls")==1){
			vFnSubCmdLogLs();
		} else if(iFnCompararCadenas(strSubCMD,"pausar")==1){
			vFnSubCmdLogPausar();
		} else if(iFnCompararCadenas(strSubCMD,"reanudar")==1){
			vFnSubCmdLogReanudar();
		} else if(iFnCompararCadenas(strSubCMD,"tamanio")==1){
			vFnSubCmdLogTamanio(iComandoPos);
		} else {
			vFnImprimir("\n Uso incorrecto, tipee [log ayuda] o [log] para ver las opciones disponibles");
		}
	} else {
	    vFnImprimir( "\n uso: log [subcomando] [parametros]"
			 "\n subcomandos: "
		      	 "\n  ayuda"
		         "\n  eliminar"
				 "\n  finalizar"
				 "\n  guardar [Nombre_Archivo]"
				 "\n  inciar"		
				 "\n  ls"		
				 "\n  pausar"		
				 "\n  reanudar"		
				 "\n  tamanio [Knro_eventos]"				
		         );
	}
}


/******************************************************************************
Funciónes: Subcomandos de log
Descripción: permite la creacion de un archivo de log, para poder verificar los 
			resultados de la prueba de algoritmos. Se pueden eliminar, finalizar, 
			guardar, inciar, pausar, y reanudar. Tambien se puede establecer el tamanio
			de un log en un numero de keventos multiplos de 1024

Recibe:   posicion de comienzo del comando en el buffer del shell
Devuelve: nada

Fecha última modificación: 09/09/2007
******************************************************************************/

void vFnSubCmdLogAyuda(){
	vFnImprimir( "\n uso: log [subcomando] [parametros]"
				 "\n subcomandos: "
		      	 "\n  ayuda"
		         "\n  eliminar"
				 "\n  finalizar"
				 "\n  guardar [Nombre_Archivo]"
				 "\n  inciar"		
				 "\n  ls"		
				 "\n  pausar"		
				 "\n  reanudar"		
				 "\n  tamanio [Knro_eventos]"				
		         );
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLogEliminar(){
		vFnImprimir("\nTODO log Eliminar"); //inserte su comando aqui
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLogFinalizar(){
		vFnImprimir("\nTODO log finalizar"); //inserte su comando aqui	
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLogGuardar(int iComandoPos){
	char strPar[16];
	if(iFnGetArg (iComandoPos, 2, strPar, sizeof(strPar)) == 1){
		vFnImprimir("\nTODO log guardar");//inserte su comando aqui usando [strPar] como argumento
	}
	else{
		vFnImprimir("\nIngrese el nombre del archivo donde guardar el log!");
	}
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLogIniciar(){
		vFnImprimir("\nTODO log iniciar"); //inserte su comando aqui	
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLogLs(){
		vFnImprimir("\nTODO log ls"); //inserte su comando aqui	
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLogPausar(){
			vFnImprimir("\nTODO log pausar"); //inserte su comando aqui
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLogReanudar(){
			vFnImprimir("\nTODO log reanudar"); //inserte su comando aqui
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnSubCmdLogTamanio(int iComandoPos){
	char strPar[16];
	if(iFnGetArg (iComandoPos, 2, strPar, sizeof(strPar)) == 1){
		vFnImprimir("\nTODO log tamanio");//inserte su comando aqui usando [strPar] como argumento
	}
	else{
		vFnImprimir("\nIngrese la cantidad de eventos a guardar (se calculara como multiplos de 1024)");
	}
}

