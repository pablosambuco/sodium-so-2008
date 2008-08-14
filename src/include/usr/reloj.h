/**
	\file usr/reloj.h
	\brief funciones del reloj
*/
#ifndef _RELOJ_H_
#define _RELOJ_H_

/**
  Esta funci�nn inicializa el reloj del sistema, es decir, la variable ulTiempo,
  con la hora que tiene la CMOS
*/
void vFnLoopReloj();

void vFnInicializarReloj();

#endif //_RELOJ_H_
