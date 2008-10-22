#include <kernel/libk/string.h>
#include <kernel/libk/libk.h>
#include <kernel/mem/memoria_k.h>

#include <video.h>  
//revisar si las funciones de video.h referentes a cadenas deberian ir en string.h

/**
 * @brief Copia 'size' bytes de 'orig' a 'dest'
 * @date 
 */
inline unsigned char* ucpFnCopiarMemoria(
        unsigned char *ucpDestino,
        unsigned char *ucpOrigen,
        unsigned int uiTamanio )
{
    /* Esta version en assembler es de 3 a 12 veces mas rapida (depende de la
     * cantidad de bytes a copiar) que la version anterior hecha en C.
     */
    int d0, d1, d2;

    __asm__ __volatile__(
        "rep ; movsl\n\t"
        "testb $2,%b4\n\t"
        "je 1f\n\t"
        "movsw\n"
        "1:\ttestb $1,%b4\n\t"
        "je 2f\n\t"
        "movsb\n"
        "2:"
        : "=&c" (d0), "=&D" (d1), "=&S" (d2)
        :"0" (uiTamanio/4), "q" (uiTamanio),
            "1" ((long) ucpDestino),"2" ((long) ucpOrigen)
        : "memory");

    return ucpDestino;
}


/**
 * @brief Inicializa uiTamanio bytes a 0 desde la posicion ucpDirInicio
 * @param ucpDirInicio Direccion de inicio
 * @param uiTamanio Cantidad de bytes
 * @date 
 */
inline unsigned char* ucpFnMemSetCero(
        unsigned char *ucpDirInicio,
        unsigned int uiTamanio )
{
    int d0, d1;

    __asm__ __volatile__(
        "xor %%eax, %%eax\n\t"
        "rep ; stosl\n\t"
        "testb $2,%b3\n\t"
        "je 1f\n\t"
        "stosw\n"
        "1:\ttestb $1,%b3\n\t"
        "je 2f\n\t"
        "stosb\n"
        "2:"
        : "=&c" (d0), "=&D" (d1)
        :"0" (uiTamanio/4), "q" (uiTamanio),
            "1" ((long) ucpDirInicio)
        : "%eax", "memory");

    return ucpDirInicio;
}


/**
 * iFnCompararCadenas: compara dos cadenas
 * @param cnstCadena1 primer cadena a comparar
 * @param cnstCadena2 segunda cadena a comparar
 * @return -1 si son diferentes, 0 si son iguales
 */
int
iFnCompararCadenas2 (const char *cnstCadena1, const char *cnstCadena2)
{
  int iN;

  if (iFnLongitudCadena (cnstCadena1) != iFnLongitudCadena (cnstCadena2))
    {
      return -1;
    }

  iN = 0;
  while (cnstCadena1[iN] != '\0')
    {
      if (cnstCadena1[iN] != cnstCadena2[iN++])
	{
	  return -1;
	}
    }
  return 0;
}

/******************************************************************************
Función: iFnCompararCadenas
Descripción:  compara dos cadenas. 

Recibe:   *char * cadena1, char * cadena2
Devuelve: *Devuelve un 1 si la cadena1 es igual a la cadena2
			un -1 si son distintas
NOTA: no es igual al strcmp estandar, solamente es una implementacion sencilla
 		y rapida que se adecua a las necesidades

Fecha última modificación: 09/09/2007
******************************************************************************/

int iFnCompararCadenas(const char* cadena1,const char* cadena2){
	int i=0;

	if(cadena1[0]=='\0')
		return -1;
	
	while(cadena1[i]!='\0' || cadena2[i]!='\0'){
		if(cadena1[i]!=cadena2[i])
			return -1;
		i++;
	}
	return 1;
}

/******************************************************************************
Funciónes: Funciones de apoyo a vFnImprimirString
Descripción:  Copian caracteres, cadenas, words, y devuelven la cantidad de
			  Bytes copiados (excepto copiaCaracter, por ser solo uno)

Reciben:   *char *cadena1, char *cadena2
Devuelve: en general, la cantidad de bytes copiados
Fecha última modificación: 23/09/2007
******************************************************************************/

/**
 * iFnCopiaCaracter: copia un caracter de caracter 
 *					origen a caracter destino
 *
 * @param ch caracter a imprimir
 * @return estado de la operacion
 *
 */
int
iFnCopiaCaracter (char *cCaracterDestino, const char *cCaracterOrigen)
{
  *cCaracterDestino=*cCaracterOrigen;
  return 1;
}

/**
 * iFnCopiaCadena: Copia una cadena a otra.
 * @param cCadenaDestino cadena a imprimir,cnstCadenaOrigen cadena a copiar
 * @return cantidad de caracteres impresos
 */
int
iFnCopiaCadena (char *cCadDestino, const char *cCadOrigen)
{
  int iCaracteresCopiados=0;	
  while (*cCadOrigen != '\0')
    {
      iFnCopiaCaracter (cCadDestino, cCadOrigen);
      cCadDestino++;
	  cCadOrigen++;
	  iCaracteresCopiados++;
    }
  return iCaracteresCopiados-1;
}

/**
 * iFnCopiaEntero: Copia un entero o un doble word (32 bits) a cCadenaDestino
 * @param cadena destino, nro entero a imprimir.
 */
int
iFnCopiaEntero (char *cCadenaDestino, const int cniNumero)
{
  // 2^32 = 4294967296 --> a lo sumo el entero tendra 10 digitos + signo + \0
  char stCadena[12];
  vFnItoa (cniNumero, (char *) stCadena); 
  return iFnCopiaCadena (cCadenaDestino, stCadena);
}

/**
 * iFnCopiaFloat: Copia un float o un double a cCadenaDestino
 * @param cadena destino, nro float a imprimir.
 */
int
iFnCopiaFloat (char *cCadenaDestino, const float cnfNumero, const int cniPrecision)
{
   char stCadena[512];
   vFnFtoa ((char *) stCadena, cnfNumero, cniPrecision);
   return iFnCopiaCadena (cCadenaDestino, stCadena);
}

/**
 * iFnCopiaWord: Copia un word (16 bits) a cCadenaDestino
 * @param cadena destino, nro entero a imprimir.
 */
int
iFnCopiaWord (char *cCadenaDestino, const word cnwNumero)
{
  // 2^16 = 65535 --> a lo sumo el entero tendra 5 digitos + signo + \0
  char stCadena[7];
  vFnWtoa (cnwNumero, (char *) stCadena); 
  return iFnCopiaCadena (cCadenaDestino, stCadena);

}

/**
 * iFnCopiaByte: Copia un byte (8 bits) a cCadDestino.
 * @param cadena destino,nro entero a imprimir.
 */
int
iFnCopiaByte (char *cCadenaDestino, const byte cnbyNumero)
{
  // 2^16 = 65535 --> a lo sumo el entero tendra 5 digitos + signo + \0
  char stCadena[7];

  vFnBtoa (cnbyNumero, (char *) stCadena);
  return iFnCopiaCadena (cCadenaDestino, stCadena);
}

/**
 * iFnCopiaHexa: Copia un entero o doble word (32 bits) en base 16 a cadena
 *					Destino.
 * @param cadena destino, nro entero a imprimir.
 */
int
iFnCopiaHexa (char *cCadenaDestino, const int cniNumero)
{
  // 2^32 = 0xFFFFFFFF --> a lo sumo el entero tendra 8 digitos + signo + \0
  char stCadena[10];

  vFnItoh (cniNumero, (char *) stCadena);
  return iFnCopiaCadena (cCadenaDestino, stCadena);

}

/**
 * iFnCopiaWordHexa: Copia un word (16 bits) en base 16 otra cadena.
 * @param cadena destino, nro entero a imprimir.
 */
int
iFnCopiaWordHexa (char *cCadenaDestino, const word cnwNumero)
{
  // 2^16 = 0xFFFF --> a lo sumo el entero tendra 4 digitos + signo + \0
  char stCadena[6];

  vFnWtoh (cnwNumero, (char *) stCadena); 
  return iFnCopiaCadena (cCadenaDestino, stCadena);
}

/**
 * iFnCopiaByteHexa: Imprime un byte (8 bits) en base 16 por pantalla.
 * @param nro entero a imprimir.
 */
int
iFnCopiaByteHexa (char *cCadenaDestino, const byte cnbyNumero)
{
  // 2^16 = 0xFF --> a lo sumo el entero tendra 2 digitos + signo + \0
  char stCadena[4];

  vFnBtoh (cnbyNumero, (char *) stCadena);
  return iFnCopiaCadena (cCadenaDestino, stCadena);
}

/******************************************************************************
Función: iFnImprimirString
Descripción:  Copia una cadena a la otra con formato

Recibe:   *char * cadena1, char * cadena2
Devuelve: *devuelve la cadena copiada de cadena2 a cadena1, con el formato indicado
NOTA: equivaldria al sprintf
Fecha última modificación: 23/09/2007
******************************************************************************/
void
vFnImprimirString (char *cCadenaDestino,const char *cCadenaOrigen, ...)
{
  int iCifras;
  int iCifrasDecimales; //Cantidad de decimales para imprimir flotantes

  va_list (lista_argumentos);
  va_start (lista_argumentos, cCadenaOrigen);

  while (*cCadenaOrigen){
	if (*cCadenaOrigen == '%'){
	  	cCadenaOrigen++;

        //Se parsea la cantidad de digitos pedida
        iCifras = 0;
        while ( *cCadenaOrigen >= '0' && *cCadenaOrigen <= '9') {
            //Actualmente se ignora
            iCifras = iCifras * 10 + *(cCadenaOrigen) - '0';
            cCadenaOrigen++;
        }

        //Se parsea la precision decimal
        if( *(cCadenaOrigen) == '.' ) {
            cCadenaOrigen++;
            iCifrasDecimales = 0;
            while ( *cCadenaOrigen >= '0' && *cCadenaOrigen <= '9') {
                iCifrasDecimales = iCifrasDecimales*10 +
                    *(cCadenaOrigen) - '0';
                cCadenaOrigen++;
            }
        } else {
            iCifrasDecimales = 6;   //Cantidad de decimales por defecto
        }

		switch (*cCadenaOrigen){
	    	case 'c':
	      		cCadenaDestino+=iFnCopiaCaracter (cCadenaDestino, va_arg (lista_argumentos, char *));
	      		break;
	    	case 'd':
	      		cCadenaDestino+=iFnCopiaEntero (cCadenaDestino, va_arg (lista_argumentos, int));
	      		break;
    		case 'f':
	    		cCadenaDestino+=iFnCopiaFloat (cCadenaDestino, va_arg (lista_argumentos, double),iCifrasDecimales);
		    	break;
	    	case 'w':
	      		cCadenaDestino+=iFnCopiaEntero (cCadenaDestino, va_arg (lista_argumentos, word));
	      		break;
	    	case 'b':
	      		cCadenaDestino+=iFnCopiaByte (cCadenaDestino, va_arg (lista_argumentos, byte));
	      		break;
	    	case 'x':
	      		cCadenaOrigen++;
	      		if (*cCadenaOrigen == 'w'){
		  			cCadenaDestino+=iFnCopiaWordHexa (cCadenaDestino, va_arg (lista_argumentos, word));
				}
	      		else if (*cCadenaOrigen == 'b'){
		  			cCadenaDestino+=iFnCopiaByteHexa (cCadenaDestino, va_arg (lista_argumentos, byte));
				}
	      		else{
		  			cCadenaOrigen--;
		  			cCadenaDestino+=iFnCopiaHexa (cCadenaDestino, va_arg (lista_argumentos, int));
				}
	      		break;
	    	case 's':
	      		cCadenaDestino+=iFnCopiaCadena (cCadenaDestino, va_arg (lista_argumentos, char *));
	      		break;
	    }
	}
    else{
		iFnCopiaCaracter (cCadenaDestino, cCadenaOrigen);
	}
    cCadenaOrigen++;
	cCadenaDestino++;
  }
  *cCadenaDestino='\0';
  va_end (lista_argumentos);
}

/******************************************************************************
Función: iFnLongitudCadena
Descripción:  Determina la longitud de una cadena

Recibe:   *char * cadena
Devuelve: *devuelve la cantidad de caracteres de la cadena

Fecha última modificación: 23/09/2007
******************************************************************************/
int
iFnLongitudCadena (const char *cnstCadena)
{
  int iLongitud;
  iLongitud = 0;
  while (cnstCadena[iLongitud++] != '\0');

  return iLongitud;
}


/**
 * pstFnConcatenarCadena: Une las cadenas pasadas por paràmetro.
 * @param stCadena1 cadena que va a estar del lado izquierdo,
 *   stCadena2 cadena que se encontrarà del lado derecho
 * @return cantidad de caracteres impresos
 */
char *
pstFnConcatenarCadena (char * stCadena1, char * stCadena2)
{
	int iCantidadCadena1,iCantidadCadena2,iN,iJ;
	char * stAux;	

	iCantidadCadena1=iFnLongitudCadena(stCadena1);
	iCantidadCadena2=iFnLongitudCadena(stCadena2);
	stAux = pvFnKMalloc(iCantidadCadena1+iCantidadCadena2, MEM_DEFAULT);
	if(stAux==NULL) return (stAux);
	iN=0;
	while (stCadena1[iN] != '\0')
	{
		stAux[iN]= stCadena1[iN];
		iN++;
	}
	iJ=0;
	while (stCadena2[iJ] != '\0')
	{
		stAux[iN]=stCadena2[iJ];
		iJ++;
		iN++;
	}
	stAux[iN]='\0';
	return (stAux);	

}


/**
 * iFnBuscarEnCadena: Busca una cadena dentro de otra.
 * @param stCadena cadena original,stCadenaABuscar cadena que se quiere buscar en la cadena original
 *        iPosInicial posiciòn dentro de la cadena original a partir de donde se quiere empezar a buscar.
 * @param stCadenaABuscar es la cadena que queremos encontrar
 * @param iPosInicial es la posiciòn inicial a partir de donde queremos empezar a buscar
 * @return posiciòn de el primer valor de la cadena a buscar, -2 sino lo encuentra, -1 si hubo error
 */
int
iFnBuscarEnCadena (char * stCadena, char * stCadenaABuscar,int iPosInicial)
{
	int iCantidadCadena,iCantidadCadenaABuscar,iN,iX;

	iCantidadCadena=iFnLongitudCadena(stCadena);
	iCantidadCadenaABuscar=iFnLongitudCadena(stCadenaABuscar);
	iN=iPosInicial;
	if(iN<0) return (-1);
	while (stCadena[iN] != '\0')
	{
		if (stCadena[iN]==stCadenaABuscar[0])
		{
			iX=0;
			while (stCadena[iN+iX]==stCadenaABuscar[iX] && iX<iCantidadCadenaABuscar-1)
			{
				iX++;
			}
		 if (iX==iCantidadCadenaABuscar-1)
			return (iN + 1);
		}
		iN++;
	}
	return -2;
}



/**
 * pstFnCadenaIzquierda: Corta la cadena pasada como paràmetro, devolviendo la cantidad de caracteres 
 * que se indican como segundo paràmetro.
 * @param stCadena cadena a la cual se le quiere recortar el lado Izquierdo,
 * @param iCantidad Cantidad de caracteres a tomar.
 * @return Cadena recortada o null si existiò algùn error
 */
char *
pstFnCadenaIzquierda (char * stCadena, int iCantidad)
{
	int iCantidadCadena1,iN;
	char * stAux;	

	iCantidadCadena1=iFnLongitudCadena(stCadena);
	if (iCantidadCadena1<iCantidad) return (NULL);
	if (iCantidad<=0) return (NULL);
	stAux= pvFnKMalloc(iCantidad+1, MEM_DEFAULT);
	if(stAux==NULL) return (stAux);
	for (iN=0;iN<iCantidad;iN++)
	{
		stAux[iN]= stCadena[iN];
	}
	stAux[iN]='\0';
	return (stAux);	
}

/**
 * pstFnCadenaDerecha: Corta la cadena pasada como paràmetro, devolviendo la cantidad de caracteres 
 * que se indican como segundo paràmetro.
 * @param stCadena cadena a la cual se le quiere recortar el lado derecho,
 * @param iCantidad Cantidad de caracteres a tomar.
 * @return Cadena recortada o null si existiò algùn error
 */
char *
pstFnCadenaDerecha (char * stCadena, int iCantidad)
{
	int iCantidadCadena1,iN,iPos,iX;
	char * stAux;	

	iCantidadCadena1=iFnLongitudCadena(stCadena);
	if (iCantidadCadena1<iCantidad) return (NULL);
	if (iCantidad<=0) return (NULL);
	stAux= pvFnKMalloc(iCantidad+1, MEM_DEFAULT);
	if(stAux==NULL) return (stAux);
	if(iCantidadCadena1==iCantidad)
		{
		  for (iN=0;iN<iCantidad;iN++)
		 {
		  stAux[iN]= stCadena[iN];
	         }
		stAux[iN]='\0';
		return (stAux);
		}
	iPos=iCantidadCadena1-iCantidad-1;
	iX=0;
	for (iN=iPos;iN<iCantidadCadena1;iN++)
	{
		stAux[iX]= stCadena[iN];
		iX++;
	}
	stAux[iN]='\0';
	return (stAux);	
}

/**
 * iFnEsNumero : Verifica si lo que contiene la cadena es un nùmero 
 * @param stCadena cadena a verificar,
 * @return -1 sino es nùmero o 0 si es nùmero
 */
int
iFnEsNumero (char * stCadena)
{
	int iCantidadCadena,iN;

	iCantidadCadena=iFnLongitudCadena(stCadena);
	iN=0;
	while (iN<iCantidadCadena && stCadena[iN]!= '\0')
	{
	 if(stCadena[iN] >= '0' && stCadena[iN] <= '9')
		iN++;
	 else return (-1);
	}
	return (0);	
}

/**
 * vFnStrLwr : convierte una cadena a minusculas
 * @param stCadena cadena a convertir a minusculas
 */
void vFnStrLwr(char *stCadena) 
{   
    //Recorremos la cadena
    while(*stCadena != 0)
    {   //Si es mayuscula, la convertimos
        if(*stCadena >= 'A' && *stCadena <= 'Z')
           *stCadena+='a'-'A';
        stCadena++;
    }
}

/**
 * vFnStrUpr : convierte una cadena a mayusculas
 * @param stCadena cadena a convertir a mayusculas
 */
void vFnStrUpr(char *stCadena) 
{   
    //Recorremos la cadena
    while(*stCadena != 0)
    {   //Si es minuscula, la convertimos
        if(*stCadena >= 'a' && *stCadena <= 'z')
           *stCadena+='A'-'a';
        stCadena++;
    }
}
