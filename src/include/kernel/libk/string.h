/** \file string.h
 *   \brief funciones de utilidad para el Sodi-Kernel referentes a cadenas y bytes
 */
#ifndef _STRING_H_
#define _STRING_H_

#include <kernel/definiciones.h>

inline unsigned char* ucpFnCopiarMemoria(unsigned char *ucpDestino, 
				  unsigned char *ucpOrigen, 
				  unsigned int uiTamanio);

int iFnLongitudCadena (const char *cnstCadena);
int iFnCompararCadenas (const char *, const char *);
int iFnCompararCadenas2 (const char *, const char *);
int iFnCopiaCaracter (char *, const char *);
int iFnCopiaCadena (char *, const char *);

int iFnCopiaEntero (char *, const int);
int iFnCopiaFloat (char *, const float, const int);
int iFnCopiaWord (char *, const word);
int iFnCopiaByte (char *, const byte);
int iFnCopiaHexa (char *, const int);
int iFnCopiaWordHexa (char *, const word);
int iFnCopiaByteHexa (char *, const byte);
void vFnImprimirString (char *, const char *, ...);
int iFnLongitudCadena (const char *);

char * pstFnConcatenarCadena (char * , char * );
int iFnBuscarEnCadena (char * , char * ,int );
char * pstFnCadenaIzquierda (char * , int );
char * pstFnCadenaDerecha (char * , int );
int iFnEsNumero (char * stCadena);

void vFnStrLwr (char * stCadena);
void vFnStrUpr (char * stCadena);
#endif

