/**
	\file shell/cmd_planif.h
*/
#ifndef _CMD_PLANIF_H_
#define _CMD_PLANIF_H_

/**
	\defgroup protoPlanif Funciones de subcomando shell planif
*/
/*@{*/
void vFnMenuPlanif(int iComandoPos);
void vFnSubCmdPlanifAplicar();
void vFnSubCmdPlanifCambiar(int);
void vFnSubCmdPlanifDefecto();
void vFnSubCmdPlanifMostrar();
void vFnSubCmdPlanifSet(int);
/*@}*/
#endif
