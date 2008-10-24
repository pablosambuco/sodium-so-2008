/* ATENCION: Este archivo esta basado en include/kernel/libk/libk.h
 * Cualquier modificacion que se haga en este archivo debe ser tenida en cuenta
 * para ser incluida en el archivo original mencionado y viceversa.
 *
 * Se opto por aniadir el sufijo _usr a las funciones que aqui figuran para
 * diferenciarlas de las disponibles a nivel kernel. Esta decision no se
 * relaciona con ninguna limitacion tecnica, solo se hace para evitar
 * confusiones a quienes las utilicen.
 * 
 * Puede que muchas de las funciones y partes del codigo de hallen comentadas.
 * Esto no quiere decir que no funcionen o sean inapropiadas, es solo que al
 * momento de generar este archivo no se tuvo el tiempo necesario para probarlas
 */

/** \file sodstdlib.h
 *  \brief funciones de utilidad
 */

#ifndef __SODSTDLIB_H_
#define __SODSTDLIB_H_

/*dword dwFnCtod (char *stBuffer);
word wFnCtow (char *stBuffer);
byte byFnCtob (char *stBuffer);
*/
void vFnItoa_usr (int iNumero, char *stBuffer);
/*void vFnItoh (int iNumero, char *stBuffer);

void vFnWtoa (word wNumero, char *stBuffer);
void vFnWtoh (word wNumero, char *stBuffer);

void vFnBtoa (byte byNumero, char *stBuffer);
void vFnBtoh (byte byNumero, char *stBuffer);

int iFnHtoi (char *c);
int iFnCtoi (char *stBuffer);
int iFnXtoi (char *c);
*/
extern void vFnFtoa_usr(char *stBuffer, double fNumero, int iPrecision);
extern double fFnAtof_usr(const char *stBuffer);

#endif
