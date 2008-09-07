#include <kernel/puertos.h>
#include <shell/sys_video.h>
#include <shell/teclado.h>

char* stMemoriaVideo = (char *)0xB8000;
stuVentana pstuVentana[HWND_VENTANA_MAX] = {
  {0, 0, BLANCO, 0, 7, 80, 16},	//ventana del shell
  {0, 0, CYAN_CLARO, 0, 0, (POSRELOJ - 1), 1},	//CSwitch
  {0, 0, AMARILLO, 0, 24, 80, 1},	//ventana de ayuda
  {0, 0, MAGENTA_CLARO, 0, 2, 80, 4},	//proceso
  {0, 0, CYAN_CLARO, 0, 1, 80, 1},	//doble linea de CSwitch
  {0, 0, MAGENTA_CLARO, 0, 6, 80, 1},	//doble linea de proceso
  {0, 0, AMARILLO, 0, 23, 80, 1},	//doble linea de ayuda
  {0, 0, BLANCO_BRILLANTE, POSRELOJ, 0, 18, 1},	//ventana del reloj
  {0, 0, BLANCO, 0, 25, 80, 23},    // WV: Ventana del log, simula una nueva terminal.
  {0, 0, AMARILLO, 0,48, 80, 2}     // WV: Ventana de la ayuda del log.
};

byte  bBufferLog[(LOG_HISTORIA+1+1)*160]; // WV: Es el buffer que guarga el log.

static word  wIndexLog=0;              // WV: Indice del buffer del log, indica si esta lleno.
static word  wLecturaLog=0;            // WV. Indice de lectua dentro del buffer del log.

// WV: Define la base de donde la "terminal" log se mostrara en pantalla. 
#define  MEMORIA_VIDEO_LOG       0x07D0      // 80 x 25 = 2000(dec) = 0x07D0 (hex)


/**
 * iFnSysImprimirCaracter: imprime un caracter en las coordenadas especificas
 *
 * @param ch caracter a imprimir
 * @return estado de la operacion
 *
 */
int
iFnSysImprimirCaracter (int iHwnd, char cCaracter)
{
  switch (cCaracter)
    {
    case ASCII_BACKSPACE:
      vFnBorrarCaracter (iHwnd);
      break;
    case ASCII_ENTER:
      vFnNuevaLinea (iHwnd);
      break;
    case ASCII_TABULADOR:
      vFnTabulador (iHwnd);
      break;
    default:
      vFnPonerCaracter (iHwnd, cCaracter);
      vFnIncrementarCoordenadas (iHwnd);
      break;
    }

  if (iHwnd == 0)
    vFnActualizarCursor (0);

  return 1;
}


/**
 * vFnTabulador: Imprime un TAB por pantalla
 */

void
vFnTabulador (int iHwnd)
{
  static int staiIndice;
  static int staiCantidadEspacios;

  int iCursorX = pstuVentana[iHwnd].iCursorX;

  staiCantidadEspacios =
    (((iCursorX / TAM_TABULADOR) + 1) * TAM_TABULADOR) - iCursorX;
  for (staiIndice = 0; staiIndice < staiCantidadEspacios; staiIndice++)
    {
      vFnPonerCaracter (iHwnd, ' ');
      vFnIncrementarCoordenadas (iHwnd);
    }
}


/**
 * vFnNuevaLinea: Imprime una nueva linea
 */

void
vFnNuevaLinea (int iHwnd)
{
  pstuVentana[iHwnd].iCursorX = 0;
  if (pstuVentana[iHwnd].iCursorY >= pstuVentana[iHwnd].iAlto - 1)
    vFnScroll (iHwnd);
  else
    pstuVentana[iHwnd].iCursorY++;
}

/**
 * vFnBorrarCaracter: borra un caracter de pantalla iPosicionY actualiza el cursor
 */
void
vFnBorrarCaracter (int iHwnd)
{
   if (--pstuVentana[iHwnd].iCursorX == -1)
   {
      if (pstuVentana[iHwnd].iCursorY == 0)
         pstuVentana[iHwnd].iCursorX = 0;
      else
      {
         pstuVentana[iHwnd].iCursorX = pstuVentana[iHwnd].iAncho - 1;
         pstuVentana[iHwnd].iCursorY--;
      }
   }
   vFnPonerCaracter (iHwnd, ' ');
   
   if (iHwnd == HWND_COMANDO)
      vFnActualizarCursor (0);
}

/**
 * vFnScroll: Desplaza la pantalla hacia arriba una linea
 */
void
vFnScroll (int iHwnd)
{
  int iN, iJ;
  int iPosicionX = pstuVentana[iHwnd].iPosicionX;
  int iPosicionY = pstuVentana[iHwnd].iPosicionY;
  int iAncho = pstuVentana[iHwnd].iAncho;
  int iAlto = pstuVentana[iHwnd].iAlto;
   
   // WV: Si el scroll es sobre la ventana de log, salvo la primer linea, a pisar.
   if( iHwnd == HWND_LOG )
   {
      // WV: Siempre voy desplazando el buffer para que al hacer scroll quede en el orden correcto.
      for( iJ=wIndexLog; iJ>=0; iJ-- )
         for( iN=0; iN<160; iN++ )
            bBufferLog[ iN + (iJ*160) ] = bBufferLog[ iN + ((iJ-1)*160) ];
        
      // WV: Guardo la linea a pisar en el buffer del log.
      for (iN = 0; iN <160; iN+=2)
         bBufferLog[ (iN) ] = MEM_VIDEO(iPosicionX + iN, iPosicionY );

      // WV: Actualizo la cantidad de lineas guardadas en el buffer del log.
      if( wIndexLog <= LOG_HISTORIA ) wIndexLog++;
   }


  for (iJ = 1; iJ < iAlto; iJ++)
    {
      for (iN = 0; iN < iAncho; iN++)
	{
	  MEM_VIDEO ((iPosicionX + iN) * 2, iPosicionY + iJ - 1) =
	    MEM_VIDEO ((iPosicionX + iN) * 2, iPosicionY + iJ);
	  MEM_VIDEO ((iPosicionX + iN) * 2 + 1, iPosicionY + iJ - 1) =
	    MEM_VIDEO ((iPosicionX + iN) * 2 + 1, iPosicionY + iJ);
	}
    }

  for (iN = 0; iN < iAncho; iN++)
    {
      MEM_VIDEO ((iPosicionX + iN) * 2, iPosicionY + iAlto - 1) = ' ';
      MEM_VIDEO ((iPosicionX + iN) * 2 + 1, iPosicionY + iAlto - 1) =
	pstuVentana[iHwnd].cColor;
    }

  pstuVentana[iHwnd].iCursorX = 0;
  pstuVentana[iHwnd].iCursorY = iAlto - 1;
}

/**
 * stavFnActualizarCursor: actualiza la posicion del cursor en base a las
 *                    coordenadas actuales.
 * <pre>
 * lo que hay que escribirle a 0x3D5 es (para un par X,Y): Y * 80 + X
 * por ejemplo, para (10,8) -> 8 * 80 + 10 = 650 = 0x028A = 0000001010001010
 * pero se le escribe en dos partes, el word mas bajo (seteando
 * el port 0x3D4 con 0xF) en este caso 0x8A, iPosicionY el word mas alto, seteando
 * el port 0x3D4 con 0xE), en este caso con 0x02.
 * </pre>
 */
void 
vFnActualizarCursor (int iHwnd)
{
  int iCursorX;
  int iCursorY;

  iCursorX = pstuVentana[iHwnd].iCursorX + pstuVentana[iHwnd].iPosicionX;
  iCursorY = pstuVentana[iHwnd].iCursorY + pstuVentana[iHwnd].iPosicionY;

  outb (0xF, 0x3D4);
  outb ((iCursorY * 80 + iCursorX) & 0x00FF, 0x3D5);
  outb (0xE, 0x3D4);
  outb (0x2, 0x3D5);
  outb (((iCursorY * 80 + iCursorX) >> 8) & 0x00FF, 0x3D5);
}

/**
 * vFnIncrementarCoordenadas: Funcion privada al modulo que incrementa las
 *                          coordenadas por cada caracter que imprime.
 */
inline void
vFnIncrementarCoordenadas (int iHwnd)
{
  int iCursorX = pstuVentana[iHwnd].iCursorX;
  int iCursorY = pstuVentana[iHwnd].iCursorY;

  if (++iCursorX == pstuVentana[iHwnd].iAncho)
    {
      if (iCursorY == pstuVentana[iHwnd].iAlto - 1)
	vFnScroll (iHwnd);
      else
	iCursorY++;

      iCursorX = 0;
    }

  pstuVentana[iHwnd].iCursorX = iCursorX;
  pstuVentana[iHwnd].iCursorY = iCursorY;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
inline void
vFnPonerCaracter (int iHwnd, int iCaracter)
{
  if (pstuVentana[iHwnd].iAlto > 0)
    {
      MEM_VIDEO ((pstuVentana[iHwnd].iPosicionX +
		  pstuVentana[iHwnd].iCursorX) * 2,
		 pstuVentana[iHwnd].iPosicionY +
		 pstuVentana[iHwnd].iCursorY) = iCaracter;
      MEM_VIDEO ((pstuVentana[iHwnd].iPosicionX +
		  pstuVentana[iHwnd].iCursorX) * 2 + 1,
		 pstuVentana[iHwnd].iPosicionY +
		 pstuVentana[iHwnd].iCursorY) = pstuVentana[iHwnd].cColor;
    }
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
char
cFnObtenerColor (int iHwnd)
{
  return pstuVentana[iHwnd].cColor;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnSysSetearColor (int iHwnd, char cColor)
{
  pstuVentana[iHwnd].cColor = cColor;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnSysSetearX (int iHwnd, int iCursorX)
{
  pstuVentana[iHwnd].iCursorX = iCursorX;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnSysSetearY (int iHwnd, int iCursorY)
{
  pstuVentana[iHwnd].iCursorY = iCursorY;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnClsWin (int iHwnd)
{
  int iN;

  for (iN = 0; iN < pstuVentana[iHwnd].iAlto; iN++)
    vFnImprimirVentana (iHwnd, "\n");

}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnCambiarVisibilidadVentanaProceso ()
{
  int iN;
  static int staiEstado = 1;
  static stuVentana stastuVentanaShell;
  static stuVentana stastuVentanaProceso;
  static stuVentana stastuSeparadorVentanaProceso;

  if (staiEstado == 1)
    {
      staiEstado = 0;

      vFnClsWin (HWND_PROCESOS);
      vFnClsWin (HWND_SEP_PROCESOS);

      stastuVentanaProceso = pstuVentana[HWND_PROCESOS];
      stastuSeparadorVentanaProceso = pstuVentana[HWND_SEP_PROCESOS];
      stastuVentanaShell = pstuVentana[HWND_COMANDO];

      pstuVentana[HWND_COMANDO].iPosicionY = 2;	//ventana context switch y reloj 
      pstuVentana[HWND_COMANDO].iAlto =
	pstuVentana[HWND_COMANDO].iAlto + stastuVentanaProceso.iAlto +
	stastuSeparadorVentanaProceso.iAlto;

      pstuVentana[HWND_PROCESOS].iCursorX = 0;
      pstuVentana[HWND_PROCESOS].iCursorY = 0;
      pstuVentana[HWND_PROCESOS].iAlto = 0;
      pstuVentana[HWND_PROCESOS].iAncho = 0;

      pstuVentana[HWND_SEP_PROCESOS].iCursorX = 0;
      pstuVentana[HWND_SEP_PROCESOS].iCursorY = 0;
      pstuVentana[HWND_SEP_PROCESOS].iAlto = 0;
      pstuVentana[HWND_SEP_PROCESOS].iAncho = 0;
      for (iN = 0;
	   iN <
	   (stastuVentanaProceso.iAlto + stastuSeparadorVentanaProceso.iAlto);
	   iN++)
	vFnScroll (HWND_COMANDO);
      pstuVentana[HWND_COMANDO].iCursorY =
	pstuVentana[HWND_COMANDO].iCursorY - (stastuVentanaProceso.iAlto +
				   stastuSeparadorVentanaProceso.iAlto);
    }
  else
    {
      staiEstado = 1;
      pstuVentana[HWND_COMANDO].iPosicionY = stastuVentanaShell.iPosicionY;
      pstuVentana[HWND_COMANDO].iAlto = stastuVentanaShell.iAlto;
      pstuVentana[HWND_COMANDO].iCursorY =
      pstuVentana[HWND_COMANDO].iCursorY - (stastuVentanaProceso.iAlto +
				   stastuSeparadorVentanaProceso.iAlto);
      pstuVentana[HWND_PROCESOS] = stastuVentanaProceso;
      pstuVentana[HWND_SEP_PROCESOS] = stastuSeparadorVentanaProceso;

      vFnClsWin (HWND_PROCESOS);
     }
}
// WV: Cambia la pagina activa de la memoria de video, para simular terminal.
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnCambiarTerminal(void)
{
  int iFlags;
  unsigned short int bInterrupcionesHabilitadas = 0;
  
   __asm__ ("pushf\n pop %%eax": "=a" (iFlags):);

  // si estaban habilitadas, aqui se deshabilitan
  if (iFlags & 0x200)
    {
      __asm__ ("cli"::);
      bInterrupcionesHabilitadas = 1;
    }

   switch( bTerminalActiva )
   {
    // Si estoy en la terminal SODIUM, paso a la terminal LOG.
    case TERM_SODIUM : 
                  bTerminalActiva = TERM_LOG;
                  outb(0x0C, 0x3D4); outb((MEMORIA_VIDEO_LOG&0xFF00)>>8 , 0x3D5); // Parte alta de la memoria.
                  outb(0x0D, 0x3D4); outb((MEMORIA_VIDEO_LOG&0x00FF)    , 0x3D5); // Parte baja de la memoria.
      
                  break;
    // Estoy en la terminal LOG y paso a la terminal SODIUM.
    case TERM_LOG:
                  bTerminalActiva = TERM_SODIUM;
                  outb(0x0C, 0x3D4); outb(0x00, 0x3D5);
                  outb(0x0D, 0x3D4); outb(0x00, 0x3D5);
               break;
   }

   if (bInterrupcionesHabilitadas)
    __asm__ ("sti"::);
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/

// WV: Mueve la "terminal" de log hacia arriba.
void vFnLogScrollArriba()
{
   byte bN, bJ;
   byte bAuxBuffer[160];
   int iPosicionX = pstuVentana[HWND_LOG].iPosicionX;
   int iPosicionY = pstuVentana[HWND_LOG].iPosicionY;

   // WV: Si todavia no tiene nada el buffer. No se hace el scroll.
   if( wIndexLog == 0 ) return;

   // WV: Si el indice de lectura del buffer es menor que el de escritura.
   //     No leo mas de lo que escribi.
   if( (wLecturaLog) < wIndexLog )
   {
      // WV: Salvo la ultima linea de la "terminal" log.
      for( bN=0; bN < pstuVentana[HWND_LOG].iAncho<<1; bN+=2)
         bAuxBuffer[bN] = MEM_VIDEO ( iPosicionX + bN, iPosicionY + pstuVentana[HWND_LOG].iAlto -1 );
         
      // WV: Hago el scrooll de la pantalla.
      for( bJ=pstuVentana[HWND_LOG].iAlto-1; bJ > 0; bJ--)
         for( bN=0; bN < pstuVentana[HWND_LOG].iAncho<<1; bN+=2)
            MEM_VIDEO ( iPosicionX + bN, iPosicionY + bJ ) = MEM_VIDEO ( iPosicionX + bN, iPosicionY + bJ -1);
      
      // WV: Actualizo la primer linea de la terminal, con lo que tiene el buffer.
      for( bN=0; bN < pstuVentana[HWND_LOG].iAncho<<1; bN+=2)
        MEM_VIDEO ( iPosicionX + bN, iPosicionY) =  bBufferLog[ (bN) + ((wLecturaLog)*160) ];
      
      // WV: Guardo la ultima linea de la "teminal" los en el buffer.
      for( bN=0; bN < pstuVentana[HWND_LOG].iAncho<<1; bN++)
         bBufferLog[ bN + ((wLecturaLog)*160)] = bAuxBuffer[bN];

      // Incremento el indice de lectura del log.
      wLecturaLog++;
    }
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
// WV: Mueve la "terminal" de log hacia abajo.
void vFnLogScrollAbajo()
{
   byte bN, bJ;
   byte bAuxBuffer[160];
   int iPosicionX = pstuVentana[HWND_LOG].iPosicionX;
   int iPosicionY = pstuVentana[HWND_LOG].iPosicionY;

   // WV: Si todavia no tiene nada el buffer. No se hace el scroll.
   if( wIndexLog == 0 ) return;
   
   //WV: Si todavia no fui para arriba, No se hace scroll.
   if( wLecturaLog != 0 )
   {
         // Incremento el indice de lectura del log.
      wLecturaLog--;
    //  wLecturaLog -- ;
      // WV: Salvo la linea ultima linea de la "terminal" log.
      for( bN=0; bN < pstuVentana[HWND_LOG].iAncho<<1; bN+=2 )
         bAuxBuffer[(bN)] = MEM_VIDEO ( iPosicionX + bN, iPosicionY );
         
      // WV: Hago el scroll de la pantalla.
      for( bJ=0; bJ < pstuVentana[HWND_LOG].iAlto-1; bJ++ )
         for( bN=0; bN < pstuVentana[HWND_LOG].iAncho<<1; bN+=2 )
            MEM_VIDEO ( iPosicionX + bN, iPosicionY + bJ ) = MEM_VIDEO ( iPosicionX + bN, iPosicionY + bJ+1);
      
      // WV: Actualizo la ultima linea de la terminal, con lo que tiene el buffer.
      for( bN=0; bN < 160; bN+=2 )
        MEM_VIDEO ( iPosicionX + bN, iPosicionY+pstuVentana[HWND_LOG].iAlto-1) =
                                     bBufferLog[ bN + ((wLecturaLog)*160) ];
      
      // WV: Guardo la ultima linea de la "teminal" los en el buffer.
      for( bN=0; bN < pstuVentana[HWND_LOG].iAncho<<1; bN++ )
         bBufferLog[ bN + ((wLecturaLog)*160)] = bAuxBuffer[bN];
  
   }
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
// WV: Mueve la "terminal" al maximo scroll posible.
void vFnLogScrollMaximo()
{
   int iN;
  
   for( iN=wIndexLog;iN>=0;iN--)
      vFnLogScrollArriba();
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
// Wv: Mueve la "terminal" al minimo scroll posible.
void vFnLogScrollMinimo()
{
   int iN;
  
   for( iN=wLecturaLog;iN>=0;iN--)
      vFnLogScrollAbajo();
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
// WV: Limpia el log.
void vFnLogBorrar()
{
   byte bN, bJ;
   int iPosicionX = pstuVentana[HWND_LOG].iPosicionX;
   int iPosicionY = pstuVentana[HWND_LOG].iPosicionY;
   
   // WV: Reseteo el contador de lineas escritas en el buffer del log.
   wIndexLog=0;
   
   // WV: Reseto el contados de las lineas leidas en el scroll del log.
   wLecturaLog=0; 
   
   // WV: Reseteo la pantalla del log.
   for( bJ=0; bJ < pstuVentana[HWND_LOG].iAlto; bJ++ )
      for( bN=0; bN < pstuVentana[HWND_LOG].iAncho; bN+=2 )
          MEM_VIDEO ( iPosicionX + bN, iPosicionY + bJ ) = ' ';
          
   // WV: Reseteo las coordenadas del cursor.
   pstuVentana[HWND_LOG].iCursorX = 0;
   pstuVentana[HWND_LOG].iCursorY = 0;
}
