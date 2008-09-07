/**
	\file shell/video.h
	\brief Biblioteca de funciones de video del sistema
*/
#ifndef _SYS_VIDEO_H_
#define _SYS_VIDEO_H_

#include <video.h>

#define TAM_TABULADOR 8	/*!<Cantidad de caracteres de la tecla TAB */
/**
	\brief Macro que llama a stMemoriaVideo para escribir en la posicion deseada
	\param iPosicionX posicion en X del caracter
	\param iPosicionY posicion en Y del caracter
*/
#define MEM_VIDEO(iPosicionX,iPosicionY) stMemoriaVideo[(iPosicionX)+(iPosicionY)*160]

/**
	\brief Estructura con informacion de las ventanas
*/
typedef struct _stuVentana_
{
  int iCursorX;
  int iCursorY;
  char cColor;
  int iPosicionX;
  int iPosicionY;
  int iAncho;
  int iAlto;
}stuVentana;

void vFnTabulador (int);
void vFnNuevaLinea (int);
void vFnBorrarCaracter (int);
void vFnScroll (int);
void vFnActualizarCursor (int);
inline void vFnIncrementarCoordenadas (int);
inline void vFnPonerCaracter (int, int);
char cFnObtenerColor (int);
void vFnSysSetearColor (int, char);
void vFnSysSetearX (int, int);
void vFnSysSetearY (int, int);
void vFnCambiarVisibilidadVentanaProceso ();
void vFnClsWin (int);

#endif
