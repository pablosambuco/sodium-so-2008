/** 
	\file shell/cmd_lotes.h
*/
#ifndef _CMD_LOTES_H_
#define _CMD_LOTES_H_

/**
	\defgroup prototiposLotes Funciones de subcomando Lotes de shell
*/
/*@{*/
void vFnMenuLotes(int iComandoPos);
void vFnSubCmdLoteAyuda();
void vFnSubCmdLoteCargar(int);
void vFnSubCmdLoteEditar(int);
void vFnSubCmdLoteEliminar(int);
void vFnSubCmdLoteGuardar(int);
void vFnSubCmdLoteIniciar(int);
void vFnSubCmdLoteLs();
void vFnSubCmdLoteMostrar(int);
void vFnSubCmdLoteNuevo();
void vFnSubCmdLotePausar();		
void vFnSubCmdLoteReanudar();	
/*@}*/


#endif
