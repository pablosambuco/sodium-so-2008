#include <usr/sodstdio.h>

/* ATENCION: Este archivo esta basado en kernel/video.c
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


/**
 * Imprime una cadena por pantalla.
 * 
 * Esta funcion no aplica formato alguno.
 *
 * @param strCadena
 * 	Puntero a la cadena terminada en /0.
 *
 * @returns Cantidad de caracteres impresos
 */
int iFnImprimirCadena_usr(const char *cnstrCadena)
{
	write(0, cnstrCadena, 0);
	return 0;//TODO - Devolver la cantidad de caracteres impresos
} 

/**
 * Imprime un entero o un doble word (32 bits) por pantalla
 * @param Handle de la ventana en la que se desea imprimir.
 * @param nro entero a imprimir.
 */
int iFnImprimirEntero_usr(const int cniNumero)
{
	// 2^32 = 4294967296 --> a lo sumo el entero tendra 10 digitos + signo + \0
	char stCadena[12];

	vFnItoa_usr(cniNumero, (char *) stCadena);
	return iFnImprimirCadena_usr(stCadena);
}

/**
 * Imprime un float o double por pantalla
 * @param Handle de la ventana en la que se desea imprimir.
 * @param nro float a imprimir.
 */
int iFnImprimirFloat_usr(const double cnfNumero, const int cniPrecision)
{
	char stCadena[512]; //El numero entero mas largo en un double tiene:
                        //  309 cifras
                        //  + 1 signo 
                        //  + punto decimal
                        //  + las cifras decimales que se pidan
                        //  + 1 cifra decimal extra (para redondeo)
                        //  + 1 caracter nulo

	vFnFtoa_usr((char *) stCadena, cnfNumero, cniPrecision);
	return iFnImprimirCadena_usr(stCadena);
}

/**
 * vFnImprimirWord: Imprime un word (16 bits) por pantalla
 * @param Handle de la ventana en la que se desea imprimir.
 * @param nro entero a imprimir.
 */
/*void vFnImprimirWord(int hVentana, const word cnwNumero)
{
	// 2^16 = 65535 --> a lo sumo el entero tendra 5 digitos + signo + \0
	char stCadena[7];

	vFnWtoa(cnwNumero, (char *) stCadena);
	iFnImprimirCadena(hVentana, stCadena);

}*/

/**
 * vFnImprimirByte: Imprime un byte (8 bits) por pantalla.
 * @param Handle de la ventana en la que se desea imprimir.
 * @param nro entero a imprimir.
 */
/*void vFnImprimirByte(int hVentana, const byte cnbyNumero)
{
	// 2^16 = 65535 --> a lo sumo el entero tendra 5 digitos + signo + \0
	char stCadena[7];

	vFnBtoa(cnbyNumero, (char *) stCadena);
	iFnImprimirCadena(hVentana, stCadena);
}*/

/**
 * vFnImprimirHexa: Imprime un entero o doble word (32 bits) en base 16 por 
 *                pantalla.
 * @param Handle de la ventana en la que se desea imprimir.
 * @param nro entero a imprimir.
 */
/*void vFnImprimirHexa(int hVentana, const int cniNumero)
{
	// 2^32 = 0xFFFFFFFF --> a lo sumo el entero tendra 8 digitos + signo + \0
	char stCadena[10];

	vFnItoh(cniNumero, (char *) stCadena);
	iFnImprimirCadena(hVentana, stCadena);

}*/

/**
 * vFnImprimirWordHexa: Imprime un word (16 bits) en base 16 por pantalla.
 * @param Handle de la ventana en la que se desea imprimir.
 * @param nro entero a imprimir.
 */
/*void vFnImprimirWordHexa(int hVentana, const word cnwNumero)
{
	// 2^16 = 0xFFFF --> a lo sumo el entero tendra 4 digitos + signo + \0
	char stCadena[6];

	vFnWtoh(cnwNumero, (char *) stCadena);
	iFnImprimirCadena(hVentana, stCadena);
}*/

/**
 * vFnImprimirByteHexa: Imprime un byte (8 bits) en base 16 por pantalla.
 * @param Handle de la ventana en la que se desea imprimir.
 * @param nro entero a imprimir.
 */
/*void vFnImprimirByteHexa(int hVentana, const byte cnbyNumero)
{
	// 2^16 = 0xFF --> a lo sumo el entero tendra 2 digitos + signo + \0
	char stCadena[4];

	vFnBtoh(cnbyNumero, (char *) stCadena);
	iFnImprimirCadena(hVentana, stCadena);
}*/

/**
 * Imprime una cadena por pantalla, recibiendo argumentos variables,
 *           en forma analoga a la funcion de biblioteca de C printf().
 * @param Handle de la ventana en la que se desea imprimir.
 * @param Cadena a imprimir y parsear para conocer los argumentos variables
 * @param ... de cero a N argumentos con parametros para la funcion imprimir().
 *
 * basada en la funcion printf del GNU/gcc
 * @link http://www.gnu.org
 */
/*void vFnImprimirVentana(int hVentana, const char *cnstCadena, ...)
{
	int iFlags;
	int bInterrupcionesHabilitadas = 0;	// indica si las interrupciones estaban
	// habilitadas al entrar a la funcion

	int iCifras;
	int iCifrasDecimales; //Cantidad de decimales para imprimir flotantes

	va_list(lista_argumentos);
	va_start(lista_argumentos, cnstCadena);

      __asm__("pushf\n pop %%eax": "=a"(iFlags):);

	// si estaban habilitadas, aqui se deshabilitan
	if (iFlags & 0x200) {
		__asm__("cli"::);
		bInterrupcionesHabilitadas = 1;
	}


	while (*cnstCadena) {
		if (*cnstCadena == '%') {
		 	cnstCadena++;

            //Se parsea la cantidad de digitos pedida
			iCifras = 0;
			while ( *cnstCadena >= '0' && *cnstCadena <= '9') {
				//Actualmente se ignora
				iCifras = iCifras * 10 + *(cnstCadena) - '0';
				cnstCadena++;
			}

			//Se parsea la precision decimal
			if( *(cnstCadena) == '.' ) {
				cnstCadena++;
                iCifrasDecimales = 0;
				while ( *cnstCadena >= '0' && *cnstCadena <= '9') {
					iCifrasDecimales = iCifrasDecimales*10 +
						*(cnstCadena) - '0';
                    cnstCadena++;
				}
			} else {
                iCifrasDecimales = 6;   //Cantidad de decimales por defecto
            }

			switch (*cnstCadena) {
			case 'c':
				iFnSysImprimirCaracter(hVentana,
						       va_arg
						       (lista_argumentos,
							char));
				break;
			case 'd':
				vFnImprimirEntero(hVentana,
						  va_arg(lista_argumentos,
							 int));
				break;
			case 'f':
				vFnImprimirFloat(hVentana,
						 va_arg(lista_argumentos,
							double),iCifrasDecimales);
				break;
			case 'w':
				vFnImprimirEntero(hVentana,
						  va_arg(lista_argumentos,
							 word));
				break;
			case 'b':
				vFnImprimirByte(hVentana,
						va_arg(lista_argumentos,
						       byte));
				break;
			case 'x':
				cnstCadena++;
				if (*cnstCadena == 'w') {
					vFnImprimirWordHexa(hVentana,
							    va_arg
							    (lista_argumentos,
							     word));
				} else if (*cnstCadena == 'b') {
					vFnImprimirByteHexa(hVentana,
							    va_arg
							    (lista_argumentos,
							     byte));
				} else {
					cnstCadena--;
					vFnImprimirHexa(hVentana,
							va_arg
							(lista_argumentos,
							 int));
				}
				break;
			case 's':
				iFnImprimirCadena(hVentana,
						  va_arg(lista_argumentos,
							 char *));
				break;
			}
		} else {
			iFnSysImprimirCaracter(hVentana, *cnstCadena);
		}
		cnstCadena++;
	}
	va_end(lista_argumentos);

	if (bInterrupcionesHabilitadas)
		__asm__("sti"::);
}*/

/**
 * Imprime una cadena por pantalla, recibiendo argumentos variables,
 *           en forma analoga a la funcion de biblioteca de C printf().
 * @param Cadena a imprimir y parsear para conocer los argumentos variables
 * @param ... de cero a N argumentos con parametros para la funcion imprimir().
 *
 * basada en la funcion printf del GNU/gcc
 * @link http://www.gnu.org
 */

int iFnImprimir_usr(char *cnstCadena, ...)
{
/*	int iFlags;
	int bInterrupcionesHabilitadas = 0;	// indica si las interrupciones estaban
	// habilitadas al entrar a la funcion
*/
	char * pcPivotCadena;
	char cCaracterAux;

	int iCaractImpresos = 0; //Cantidad total de caracteres impresos

	int iCifras;
	int iCifrasDecimales; //Cantidad de decimales para imprimir flotantes

	va_list(lista_argumentos);
	va_start(lista_argumentos, cnstCadena);
/*
      __asm__("pushf\n pop %%eax": "=a"(iFlags):);

	// si estaban habilitadas, aqui se deshabilitan
	if (iFlags & 0x200) {
		__asm__("cli"::);
		bInterrupcionesHabilitadas = 1;
	}
*/
	while (*cnstCadena) {
		pcPivotCadena = cnstCadena;

		while (*pcPivotCadena != '%' && *pcPivotCadena) {
		 	pcPivotCadena++;
		}

		//En vez de imprimir de a un caracter, imprimimos los bloques de la
		//caden que no necesitan parseo
		//Para eso "cortamos" temporalmente la cadena insertando '\0'
		//Lo hacemos solo si hay algo que imprimir

        //TODO - Evaluar si vale la pena modificar la vFnImrpimir original
        //para que incluya esta modificacion
		if(pcPivotCadena != cnstCadena) {
			cCaracterAux = *pcPivotCadena;
			*pcPivotCadena = '\0';
			iCaractImpresos += iFnImprimirCadena_usr(cnstCadena);
			*pcPivotCadena = cCaracterAux;
			cnstCadena = pcPivotCadena;
		}

		//Aqui tratamos los bloques que requieren parseo
		if (*cnstCadena == '%') {
		 	cnstCadena++;

            //Se parsea la cantidad de digitos pedida
			iCifras = 0;
			while ( *cnstCadena >= '0' && *cnstCadena <= '9') {
				//Actualmente se ignora
				iCifras = iCifras * 10 + *(cnstCadena) - '0';
				cnstCadena++;
			}

			//Se parsea la precision decimal
			if( *(cnstCadena) == '.' ) {
				cnstCadena++;
                iCifrasDecimales = 0;
				while ( *cnstCadena >= '0' && *cnstCadena <= '9') {
					iCifrasDecimales = iCifrasDecimales*10 +
						*(cnstCadena) - '0';
                    cnstCadena++;
				}
			} else {
                iCifrasDecimales = 6;   //Cantidad de decimales por defecto
            }

			switch (*cnstCadena) {
/*			case 'c':
				iFnSysImprimirCaracter(0,
						       va_arg
						       (lista_argumentos,
							char));
				break;
*/			case 'd':
				iCaractImpresos += iFnImprimirEntero_usr(
									  va_arg(lista_argumentos,int));
				break;
			case 'f':
				iCaractImpresos += iFnImprimirFloat_usr(
										va_arg(lista_argumentos,double),
										iCifrasDecimales);
				break;
/*			case 'w':
				vFnImprimirEntero(0,
						  va_arg(lista_argumentos,
							 word));
				break;
			case 'b':
				vFnImprimirByte(0,
						va_arg(lista_argumentos,
						       byte));
				break;
			case 'x':
				cnstCadena++;
				if (*cnstCadena == 'w') {
					vFnImprimirWordHexa(0,
							    va_arg
							    (lista_argumentos,
							     word));
				} else if (*cnstCadena == 'b') {
					vFnImprimirByteHexa(0,
							    va_arg
							    (lista_argumentos,
							     byte));
				} else {
					cnstCadena--;
					vFnImprimirHexa(0,
							va_arg
							(lista_argumentos,
							 int));
				}
				break;
*/			case 's':
				iCaractImpresos += iFnImprimirCadena_usr(
										va_arg(lista_argumentos, char *));
				break;
			}
			cnstCadena++;
		}
/*		 else {
			iFnSysImprimirCaracter(0, *cnstCadena);
		}
		cnstCadena++;
*/
	}
	va_end(lista_argumentos);
/*
	if (bInterrupcionesHabilitadas)
		__asm__("sti"::);
*/
	return iCaractImpresos;
}


/**
 * vFnLog: Imprime una cadena en la terminal del log, recibiendo argumentos variables,
 *           en forma analoga a la funcion de biblioteca de C printf().
 * @param stastCadena cadena a imprimir y parsear para conocer los argumentos variables
 * @param ... de cero a N argumentos con parametros para la funcion imprimir().
 *
 * basada en la funcion printf del GNU/gcc
 * @link http://www.gnu.org
 */
/*void vFnLog (const char *cnstCadena, ...)
{
  int iFlags;
  int bInterrupcionesHabilitadas = 0;	// indica si las interrupciones estaban
  
  // habilitadas al entrar a la funcion

  int iCifras;
  int iCifrasDecimales; //Cantidad de decimales para imprimir flotantes

  va_list (lista_argumentos);
  va_start (lista_argumentos, cnstCadena);

__asm__ ("pushf\n pop %%eax": "=a" (iFlags):);

  // si estaban habilitadas, aqui se deshabilitan
  if (iFlags & 0x200)
    {
      __asm__ ("cli"::);
      bInterrupcionesHabilitadas = 1;
    }

    // WV: Reseteo el Scroll para evitar perder informacion del log.
    vFnLogScrollMinimo(); 

  while (*cnstCadena)
    {
		if (*cnstCadena == '%') {
		 	cnstCadena++;

            //Se parsea la cantidad de digitos pedida
			iCifras = 0;
			while ( *cnstCadena >= '0' && *cnstCadena <= '9') {
				//Actualmente se ignora
				iCifras = iCifras * 10 + *(cnstCadena) - '0';
				cnstCadena++;
			}

			//Se parsea la precision decimal
			if( *(cnstCadena) == '.' ) {
				cnstCadena++;
                iCifrasDecimales = 0;
				while ( *cnstCadena >= '0' && *cnstCadena <= '9') {
					iCifrasDecimales = iCifrasDecimales*10 +
						*(cnstCadena) - '0';
                    cnstCadena++;
				}
			} else {
                iCifrasDecimales = 6;   //Cantidad de decimales por defecto
            }

	  switch (*cnstCadena)
	    {
	    case 'c':
	      iFnSysImprimirCaracter (HWND_LOG ,
				      va_arg (lista_argumentos, char));
	      break;
	    case 'd':
	      vFnImprimirEntero (HWND_LOG , va_arg (lista_argumentos, int));
	      break;
	    case 'f':
	      vFnImprimirFloat (HWND_LOG, va_arg (lista_argumentos, double), iCifrasDecimales);
	      break;
	    case 'w':
	      vFnImprimirEntero (HWND_LOG , va_arg (lista_argumentos, word));
	      break;
	    case 'b':
	      vFnImprimirByte (HWND_LOG , va_arg (lista_argumentos, byte));
	      break;
	    case 'x':
	      cnstCadena++;
	      if (*cnstCadena == 'w')
		{
		  vFnImprimirWordHexa (HWND_LOG ,
				       va_arg (lista_argumentos, word));
		}
	      else if (*cnstCadena == 'b')
		{
		  vFnImprimirByteHexa (HWND_LOG ,
				       va_arg (lista_argumentos, byte));
		}
	      else
		{
		  cnstCadena--;
		  vFnImprimirHexa (HWND_LOG , va_arg (lista_argumentos, int));
		}
	      break;
	    case 's':
	      iFnImprimirCadena (HWND_LOG , va_arg (lista_argumentos, char *));
	      break;
	    }
	}
      else
	{
	  iFnSysImprimirCaracter (HWND_LOG , *cnstCadena);
	}
      cnstCadena++;
    }
  va_end (lista_argumentos);

  if (bInterrupcionesHabilitadas)
    __asm__ ("sti"::);
}
*/

/******************************************************************************
 * Imprime en el margen superior de la pantalla (ventana 1) los datos de la
 * tarea a la que se le cede la ejecución.
 * @params Color de caracter
 * @params Pid actual
 * @params Puntero a string que indica el nombre de la tarea
 * @params Indice en la GDT para el task struct del proceso actual
 * Ultima Modificación: 21/09/2004
 *****************************************************************************/
/*void
vFnImprimirContextSwitch(int iColor, int iPid, char *stNombre,
			 int iGDTIndice)
{

	vFnImprimirVentana(1, "\nCSW: PID %d\tTAREA=%s\tINDICE GDT: %d",
			   iPid, stNombre, iGDTIndice);
}
*/
/**********************************************************************************
Funcion:vFnImprimirPrompt

Ultima Modificación: 21/09/2004

***********************************************************************************/
/*
void vFnImprimirPrompt()
{
	vFnImprimir("%s", STRPROMPTLINE);
}
*/


/**********************************************************************************
Funcion:vFnImprimirOk

Ultima Modificación: 21/09/2004

***********************************************************************************/
/*
void vFnImprimirOk(int iLinea)
{
	int iFlags;
	unsigned short int bInterrupcionesHabilitadas = 0;
      __asm__("pushf\n pop %%eax": "=a"(iFlags):);

	// si estaban habilitadas, aqui se deshabilitan
	if (iFlags & 0x200) {
		__asm__("cli"::);
		bInterrupcionesHabilitadas = 1;
	}
	//sys_setear_x(linea);
	vFnSysSetearColor(HWND_COMANDO, VERDE_CLARO);
	vFnImprimir("		[ HECHO ] ");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);

	if (bInterrupcionesHabilitadas)
		__asm__("sti"::);

}
*/
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
/*void vFnImprimirNOk(int iLinea)
{
	int iFlags;
	unsigned short int bInterrupcionesHabilitadas = 0;
      __asm__("pushf\n pop %%eax": "=a"(iFlags):);

	// si estaban habilitadas, aqui se deshabilitan
	if (iFlags & 0x200) {
		__asm__("cli"::);
		bInterrupcionesHabilitadas = 1;
	}
	//sys_setear_x(linea);
	vFnSysSetearColor(HWND_COMANDO, ROJO);
	vFnImprimir("		[ NOK!!!] ");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);

	if (bInterrupcionesHabilitadas)
		__asm__("sti"::);

}*/
