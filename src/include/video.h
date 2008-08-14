/* ATENCION: El archivo include/usr/sodstdio.h esta basado en este archivo
 * Cualquier modificacion que se haga en este archivo debe ser tenida en cuenta
 * para ser incluida en el archivo mencionado y viceversa.
 */

/**
 * \file video.h
 * Biblioteca que contiene todas las funciones del driver de video
 */
#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <kernel/definiciones.h>
#include <kernel/libk/string.h>

#define LIMITE 			255

/**
	\defgroup colorConst Constantes de color de caracter
*/
/*@{*/
#define POSRELOJ 		62
#define NEGRO 			0
#define AZUL 			1
#define VERDE 			2
#define CIAN 			3
#define ROJO 			4
#define MAGENTA 		5
#define MARRON 			6
#define BLANCO 			7
#define GRIS 			8
#define AZUL_CLARO 		9
#define VERDE_CLARO 		10
#define CYAN_CLARO 		11
#define ROJO_CLARO 		12
#define MAGENTA_CLARO 		13
#define AMARILLO 		14
#define BLANCO_BRILLANTE 	15
/*@}*/

/**
	\defgroup hwndConst Constantes de ventanas
*/
/*@{*/
#define HWND_COMANDO		0 
#define HWND_CSW		1
#define HWND_AYUDA		2
#define HWND_PROCESOS		3
#define HWND_SEP_CSW		4
#define HWND_SEP_PROCESOS	5
#define HWND_SEP_AYUDA		6
#define HWND_RELOJ		7
#define HWND_LOG        	8 /*!<nueva ventana, simula una terminal de log.*/
#define HWND_LOG_AYUDA  9     /*!<Ventana de la ayuda del log.*/
#define HWND_VENTANA_MAX 10	 /*!< Limite de ventanas en el sistema.*/
/*@}*/

typedef char* va_list;

// macros
/**
 *  \brief Constantes que representan los colores
 *
 * Utilizada por imprimir(...) para manejar argumentos variables\n
 * Extraida del compilador GNU/gcc @see http://www.gnu.org
 * \param type Numero
 */
#define va_rounded_size(type) \
(((sizeof (type) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

/**
 * Utilizada por imprimir(...) para manejar argumentos variables
 * Extraida del compilador GNU/gcc @see http://www.gnu.org
 *
 * \param ap Lista de parametros (puntero)
 * \param v valor para calcular inicio del proximo elemento de la lista
 */
#define va_start(ap, v) \
((void) (ap = (va_list) &v + va_rounded_size (v)))

/**
 * Utilizada por imprimir(...) para manejar argumentos variables
 * Extraida del compilador GNU/gcc @see http://www.gnu.org
 * \param ap lista de parametros (puntero)
 * \param type Valor leido para pasar al siguiente valor de la lista
 */
#define va_arg(ap, type) \
(ap += va_rounded_size (type), *((type *)(ap - va_rounded_size (type))))

/**
 * Utilizada por imprimir(...) para manejar argumentos variables
 * Extraida del compilador GNU/gcc @see http://www.gnu.org
 * \param ap lista de parametros
 */
#define va_end(ap) ((void) (ap = 0))

/**
 * \brief Imprime la cadena que recibe como argumento
 */
int iFnImprimirCadenas (int,const char*);

/**
 * \brief Imprime un numero entero por pantalla
 */
void vFnImprimirEntero (int,const int);

/**
 * \brief Imprime un numero flotante por pantalla
 */
void vFnImprimirFloat (int,const double, const int);

/**
 * \brief Imprime un numero entero por pantalla en formato hexa
 */
void vFnImprimirHexa (int,const int);

/**
 * \brief Imprime un numero de 16 bits por pantalla en formato hexa
 */
void vFnImprimirWordHexa (int,const word);

/**
 * \brief Imprime un numero de 8 bits por pantalla en formato hexa
 */
void vFnImprimirByteHexa (int,const byte);

/**
 * \brief Imprime una cadena por pantalla, aceptando argumentos variables
 * ( analoga a printf )
 */
void vFnImprimirWin (int,const char*, ... );
void vFnImprimir (const char*, ... );
void vFnImprimirI (const char*, ... );

extern char cFnSysObtenerColor(int);
extern void vFnSysSetearColor(int,char);
extern void vFnSysSetearX(int,int);
extern void vFnSysSetearY(int,int);
extern void vFnClsWin(int);

/*
* Funciones agregadas para lograr compatibilidad con el SODIUM.
*/

void vFnImprimirContextSwitch(int color,int pid, char* nombre, int indiceGDT);
void vFnImprimirPrompt();
void vFnImprimirOk(int linea);
void vFnImprimirNOk(int linea);

void vFnImprimirVentana (int hVentana, const char *cnstCadena, ...);

/* WV: Agregadas por el log */

#define  TERM_SODIUM   0
#define  TERM_LOG      1

int bTerminalActiva; /*!< Indica que terminal esta activa. Se usa en el shell. */

void vFnCambiarTerminal(void);                 /*!< Cambia la pagina activa, para simular terminal */
void vFnLog (const char *cnstCadena, ...);     /*!< Funcion que imprime en la "terminal" del log. */

void vFnLogScrollArriba();                     /*!< Mueve la "terminal" de log hacia arriba. */
void vFnLogScrollAbajo();                      /*!< Mueve la "terminal" de log hacia abajo. */

void vFnLogScrollMaximo();                     /*!< Mueve la "terminal" al maximo scroll posible. */
void vFnLogScrollMinimo();                     /*!< Mueve la "terminal" al minimo scroll posible. */
void vFnLogBorrar();                           /*!< Limpia el log. */


#endif //_VIDEO_H_
