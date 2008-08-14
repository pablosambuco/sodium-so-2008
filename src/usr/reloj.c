
#include <usr/reloj.h>
#include <kernel/puertos.h>
#include <kernel/definiciones.h>
#include <video.h>
#include <kernel/tiempo.h>
extern unsigned long int ulTiempo;
extern int iRelojEstado;

/********************************************************************************
las funciones del reloj
********************************************************************************/
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
static unsigned
stauFnLeerCmos (unsigned uRegistro, char cCaracter)
{
  unsigned uDigitoAlto, uDigitoBajo;

  outb (uRegistro, 0x70);
  uDigitoAlto = uDigitoBajo = inb (0x71);
  if (!cCaracter)
    return uDigitoBajo;
/* convert from BCD to binary */
  uDigitoAlto >>= 4;
  uDigitoAlto &= 0x0F;
  uDigitoBajo &= 0x0F;
  return 10 * uDigitoAlto + uDigitoBajo;
}

/*****************************************************************************
* Funcion: lFnContarSegundos                                                 *
* Descripcion: Transforma una fecha en a�o-mes-dia-hora-min-seg a in long    *
* 		que representa lacantidad de segundos transcurridos desde    *
* 		el 1� de enero del 2000 a las 00:00:00                       *
* Parametros: Un array que tiene en sus 6 posiciones la fecha                *
* Valor devuelto: Conteo de milisegundos de la fecha                         *
* Ultima Modificacion:12/10/2007                                             *
******************************************************************************/
long lFnContarSegundos(int fecha[6])
{
	int anioInicio = 2000;
	int secuenciaAnioBisiesto[13] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};
	int secuenciaAnioNoBisiesto[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

	// Las horas, minutos y segundos salen directo
	long segundosAcumulados = fecha[5] + fecha[4] * 60 + fecha[3] * 3600;

   // Los dias transcurridos son 1 menos que el de la fecha actual
   segundosAcumulados += (fecha[2] - 1) * 86400;

   // Los meses transcurridos dependen de si el a�o es bisiesto
	if ((fecha[1] % 4) == 0)
	{
      segundosAcumulados += secuenciaAnioBisiesto[fecha[1]] * 86400;
	}
	else
	{
      segundosAcumulados += secuenciaAnioNoBisiesto[fecha[1]] * 86400;
	}

	// Ahora los a�os van a depender de la cantidad de a�os biciestos que hayan pasado
   for(; fecha[0] > anioInicio; anioInicio++)
	{
		if ((anioInicio % 4) == 0)
			segundosAcumulados += 366 * 86400;
		else
			segundosAcumulados += 365 * 86400;
	}

   return segundosAcumulados;
}

/*****************************************************************************
* Funcion: InicializarReloj                                                  *
* Descripcion: Inicializa el reloj del kernel con el de la CMOS              *
* Parametros: Ninguno                                                        *
* Valor devuelto: Ninguno                                                    *
* Ultima Modificacion:12/10/2007                                             *
******************************************************************************/
void
vFnInicializarReloj(){
  unsigned uMes, uDia, uHora, uMinuto, uSegundo;
  int iAnio;
  int piFecha[6];

  int iCaracter;
  iCaracter = -1;
  if (iCaracter == -1)
  {
    outb (0x0B, 0x70);
    if (inb (0x71) & 0x04)
      iCaracter = 0;
    else
      iCaracter = 1;
  }


  iAnio = stauFnLeerCmos (9, iCaracter);	/* 0-99 */
  uMes = stauFnLeerCmos (8, iCaracter);		/* 1-12 */
  uDia = stauFnLeerCmos (7, iCaracter);		/* 1-31 */

  /* get time */
  uHora = stauFnLeerCmos (4, iCaracter);	/* 0-23 */
  uMinuto = stauFnLeerCmos (2, iCaracter);	/* 0-59 */
  uSegundo = stauFnLeerCmos (0, iCaracter);	/* 0-59 */

  piFecha[0] = iAnio;
  piFecha[1] = uMes;
  piFecha[2] = uDia;
  piFecha[3] = uHora;
  piFecha[4] = uMinuto;
  piFecha[5] = uSegundo;
 
	  ulTiempo = lFnContarSegundos(piFecha);

  //vFnImprimir("Actualizo\n");

  ulTiempo*= 1000;

  iRelojEstado = TIME_OK;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/

void
vFnLoopReloj()
{

  static int staiCaracter;
  unsigned int uAnio, uMes, uDia, uHora, uMinuto, uSegundo;

  int iAnioAnterior, iMesAnterior, iDiaAnterior, iHoraAnterior, iMinutoAnterior, iSegundoAnterior;
      iAnioAnterior= iMesAnterior= iDiaAnterior= iHoraAnterior= iMinutoAnterior= iSegundoAnterior= -1;

  while (1)
    {
      staiCaracter = -1;
      if (staiCaracter == -1)
	{
	  outb (0x0B, 0x70);
	  if (inb (0x71) & 0x04)
	    staiCaracter = 0;
	  else
	    staiCaracter = 1;
	}

      uAnio = stauFnLeerCmos (9, staiCaracter);	/* 0-99 */
      uMes = stauFnLeerCmos (8, staiCaracter);	/* 1-12 */
      uDia = stauFnLeerCmos (7, staiCaracter);	/* 1-31 */

      vFnSysSetearX (HWND_RELOJ, 0);
      if (uDia != iDiaAnterior)
	{
	  iDiaAnterior = uDia;
	  vFnSysSetearX (HWND_RELOJ, 0);
	  if (uDia < 10)
	    vFnImprimirVentana (HWND_RELOJ, "0%d/", uDia);
	  else
	    vFnImprimirVentana (HWND_RELOJ, "%d/", uDia);
	}

      vFnSysSetearX (HWND_RELOJ, 3);
      if (uMes != iMesAnterior)
	{
	  iMesAnterior = uMes;
	  if (uMes < 10)
	    vFnImprimirVentana (HWND_RELOJ, "0%d/", uMes);
	  else
	    vFnImprimirVentana (HWND_RELOJ, "%d/", uMes);
	}

      vFnSysSetearX (HWND_RELOJ, 6);
      if (uAnio != iAnioAnterior)
	{
	  iAnioAnterior = uAnio;
	  if (uAnio < 10)
	    vFnImprimirVentana (HWND_RELOJ, "0%d ", uAnio);
	  else
	    vFnImprimirVentana (HWND_RELOJ, "%d ", uAnio);
	}


      /* get time */
      uHora = stauFnLeerCmos (4, staiCaracter);	/* 0-23 */
      uMinuto = stauFnLeerCmos (2, staiCaracter);	/* 0-59 */
      uSegundo = stauFnLeerCmos (0, staiCaracter);	/* 0-59 */


      vFnSysSetearX (HWND_RELOJ, 9);
      if (uHora != iHoraAnterior)
	{
	  iHoraAnterior = uHora;
	  if (uHora < 10)
	    vFnImprimirVentana (HWND_RELOJ, "0%d:", uHora);
	  else
	    vFnImprimirVentana (HWND_RELOJ, "%d:", uHora);
	}
      vFnSysSetearX (HWND_RELOJ, 12);
      if (uMinuto != iMinutoAnterior)
	{
	//  vFnImprimirVentana (uClockTicksActual - uClockTicksAnterior,60,0,VERDE_CLARO);
	//  uClockTicksAnterior = uClockTicksActual;
	  
	  iMinutoAnterior = uMinuto;
	  if (uMinuto < 10)
	    vFnImprimirVentana (HWND_RELOJ, "0%d:", uMinuto);
	  else
	    vFnImprimirVentana (HWND_RELOJ, "%d:", uMinuto);
	}
      vFnSysSetearX (HWND_RELOJ, 15);
      if (uSegundo < 10)
	vFnImprimirVentana (HWND_RELOJ, "0%d", uSegundo);
      else
	vFnImprimirVentana (HWND_RELOJ, "%d", uSegundo);


    }
}
