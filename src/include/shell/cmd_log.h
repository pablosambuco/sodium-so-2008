/** 
	\file shell/cmd_lotes.h
*/
#ifndef _CMD_LOG_H_
#define _CMD_LOG_H_

/**
	\defgroup prototiposLog Funciones de subcomando Log de shell
*/
/*@{*/
void vFnMenuLog(int iComandoPos);
void vFnSubCmdLogAyuda();
void vFnSubCmdLogEliminar();
void vFnSubCmdLogFinalizar();
void vFnSubCmdLogGuardar(int);
void vFnSubCmdLogIniciar();
void vFnSubCmdLogLs();
void vFnSubCmdLogPausar();
void vFnSubCmdLogReanudar();
void vFnSubCmdLogTamanio(int);
/*@}*/

#endif
