/* ATENCION: El archivo include/usr/sodstdlib.h esta basado en este archivo
 * Cualquier modificacion que se haga en este archivo debe ser tenida en cuenta
 * para ser incluida en el archivo mencionado y viceversa.
 */

/** \file libk.h
 *  \brief funciones de utilidad para el Sodi-Kernel
 */

#ifndef _LIBK_H_
#define _LIBK_H_

#include <kernel/definiciones.h>

dword dwFnCtod (char *stBuffer);
word wFnCtow (char *stBuffer);
byte byFnCtob (char *stBuffer);

void vFnItoa (int iNumero, char *stBuffer);
void vFnItoh (int iNumero, char *stBuffer);

void vFnWtoa (word wNumero, char *stBuffer);
void vFnWtoh (word wNumero, char *stBuffer);

void vFnBtoa (byte byNumero, char *stBuffer);
void vFnBtoh (byte byNumero, char *stBuffer);

int iFnHtoi (char *c);
int iFnCtoi (char *stBuffer);
int iFnXtoi (char *c);

extern void vFnFtoa(char *stBuffer, double fNumero, int iPrecision);
extern double fFnAtof(const char *stBuffer);

#endif
