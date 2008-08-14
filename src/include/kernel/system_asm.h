/**
	\file kernel/system_asm.h
	\brief Contiene constantes y funciones del sistema escritas en assembler
*/
#ifndef _SYSTEM_ASM_H_
#define _SYSTEM_ASM_H_

#include <kernel/definiciones.h>

void vFnIniciarIDT_Asm(const word wLimit, const void *pvBase);
void vFnRemapearPIC_Asm();
void vFnIniciarTimer_Asm();
void vFnHandlerGenerico_Asm();  /*!<Handler generico para las interrupciones*/
void vFnHandlerTimer_Asm();     /*!<Handler para la interrupcion del timer (int 0x20)*/
void vFnHandlerTeclado_Asm();  /*!<Handler para la interrupcion del teclado (int 0x21)*/
void vFnExcepcionCPU7_Asm();   /*!<Handler para excepcion x primer uso de FPU*/
void vFnExcepcionCPU16_Asm();  /*!<Handler para excepcion en uso de FPU*/
void vFnHandlerSyscall_Asm();  /*!<Handler para los syscall*/

#endif //_PUERTOS_H_
