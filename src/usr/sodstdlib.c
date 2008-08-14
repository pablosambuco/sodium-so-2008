#include <usr/sodstdlib.h>

/* ATENCION: Este archivo esta basado en kernel/libk/libk.c
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
 * vFnItoa_usr: transforma un entero en ascii
 * @param number numero a transformar
 * @param stBuffer posicion de memoria donde se almacenara el numero
 *
 * <pre>
 * pseudocodigo:
 *    el numero se va dividiendo por diez y el resto mas 48 es lo
 *    que hay que imprimir.  Ej:
 *
 * int nro = 345
 *
 *    resto = nro%10; // resto = 5
 *    nro /= 10; // nro = 34
 *    etc....
 * 
 * debido a que el nro se imprimiria al revez, se almacena en un aux.
 * </pre>
 */
void
vFnItoa_usr (int iNumero, char *stBuffer)
{
  int iAux;
  iAux = iNumero;

  if (iNumero < 0)
    {
      iNumero *= -1;
      *stBuffer++ = '-';
    }

  do
    {
      *stBuffer++ = 48;
    }
  while ((iAux /= 10) != 0);

  *stBuffer-- = '\0';
  while (iNumero != 0)
    {
      iAux = iNumero % 10;
      iNumero = iNumero / 10;
      *stBuffer-- = iAux + 48;
    }
}

/**
 * Tres funciones agregadas por Luis Moyano
 * tal vez le sirvan a alguien
 */
/**
 *Fecha 20-11-2004
 * dwFnCtod: transforma una cadena en dword (sin signo
 * @param stBuffer posicion de memoria donde viene el numero
 * @return el numero transformado
 */
/*
dword
dwFnCtod (char *stBuffer)
{

  int iN, iX, iLongitud;
  dword dwPonderacion = 1;
  dword dwNumero = 0;

  iLongitud = iFnLongitudCadena (stBuffer);
  for (iN = iLongitud - 2; iN >= 0; iN--)
    {
      if (stBuffer[iN] < 48 || stBuffer[iN] > 57)
	{
	  return NULL;
	}
      dwPonderacion = 1;
      for (iX = 0; iX < (iLongitud - iN - 2); iX++)
	{
	  dwPonderacion *= 10;
	}
      dwNumero += (((dword) (stBuffer[iN]) - 48) * dwPonderacion);
    }
  return dwNumero;
}
*/
/**
 *Fecha 20-11-2004
 * wFnCtow: transforma una cadena en word (sin signo
 * @param stBuffer posicion de memoria donde viene el numero
 * @return el numero transformado
 */
/*word
wFnCtow (char *stBuffer)
{

  int iN, iX, iLongitud;
  word wPonderacion = 1;
  word wNumero = 0;

  iLongitud = iFnLongitudCadena (stBuffer);
  for (iN = iLongitud - 2; iN >= 0; iN--)
    {
      if (stBuffer[iN] < 48 || stBuffer[iN] > 57)
	{
	  return NULL;
	}
      wPonderacion = 1;
      for (iX = 0; iX < (iLongitud - iN - 2); iX++)
	{
	  wPonderacion *= 10;
	}
      wNumero += (((word) (stBuffer[iN]) - 48) * wPonderacion);
    }
  return wNumero;
}
*/
/**
 *Fecha 20-11-2004
 * byFnCtob: transforma una cadena en byte (sin signo)
 * @param stBuffer posicion de memoria donde viene el numero
 * @return el numero transformado
 */
/*byte
byFnCtob (char *stBuffer)
{

  int iN, iX, iLongitud;
  byte byPonderacion = 1;
  byte byNumero = 0;

  iLongitud = iFnLongitudCadena (stBuffer);
  for (iN = iLongitud - 2; iN >= 0; iN--)
    {
      if (stBuffer[iN] < 48 || stBuffer[iN] > 57)
	{
	  return NULL;
	}
      byPonderacion = 1;
      for (iX = 0; iX < (iLongitud - iN - 2); iX++)
	{
	  byPonderacion *= 10;
	}
      byNumero += (((byte) (stBuffer[iN]) - 48) * byPonderacion);
    }
  return byNumero;

}
*/
/**
 *Modificacion hecha por Luis Moyano
 *verifica si el parametro es un numero negativo 
 *y si es asi lo devuelve como tal.
 *Fecha 20-11-2004
 * iFnCtoi: transforma una cadena en entero
 * @param stBuffer posicion de memoria donde viene el numero
 * @return el numero transformado
 */
/*int
iFnCtoi (char *stBuffer)
{

  int iN, iX, iLongitud;
  int iPonderacion = 1;
  int iNumero = 0;

  iLongitud = iFnLongitudCadena (stBuffer);
  for (iN = iLongitud - 2; iN >= 0; iN--)
    {
      if (stBuffer[iN] < 48 || stBuffer[iN] > 57)
	{
	  if (stBuffer[0] == 45)
	    {
	      //si es un negativo
	      iNumero = iNumero * (-1);
	      return iNumero;
	    }
	  else
	    return NULL;
	}
      iPonderacion = 1;
      for (iX = 0; iX < (iLongitud - iN - 2); iX++)
	{
	  iPonderacion *= 10;
	}
      iNumero += (((int) (stBuffer[iN]) - 48) * iPonderacion);
    }
  return iNumero;
}
*/
/**
 * vFnItoh: transforma un entero en ascii ( formato hexa )
 * @param number numero a transformar
 * @param stBuffer posicion de memoria donde se almacenara el numero
 */
/*void
vFnItoh (int iNumero, char *stBuffer)
{
  static int staiAux;
  staiAux = iNumero;

  if (iNumero < 0)
    {
     // iNumero = - iNumero;
     // *stBuffer++ = '-';
    }

  do
    {
      *stBuffer++ = 48;
    }
  while ((staiAux /= 16) != 0);

  *stBuffer-- = '\0';
  while (iNumero != 0)
    {
      staiAux = iNumero % 16;
      iNumero = iNumero / 16;
      if (staiAux < 10)
	*stBuffer-- = staiAux + 48;
      else
	*stBuffer-- = staiAux + 55;
    }
}
*/
/**
 * vFnWtoh: transforma un word en ascii (formato hexa)
 * @param number numero a transformar
 * @param stBuffer posicion de memoria donde se almacenara el numero
 */
/*void
vFnWtoh (word wNumero, char *stBuffer)
{
  static word stawNumero;
  stawNumero = wNumero;

  do
    {
      *stBuffer++ = 48;
    }
  while ((stawNumero /= 16) != 0);

  *stBuffer-- = '\0';
  while (wNumero != 0)
    {
      stawNumero = wNumero % 16;
      wNumero = wNumero / 16;
      if (stawNumero < 10)
	*stBuffer-- = stawNumero + 48;
      else
	*stBuffer-- = stawNumero + 55;
    }
}
*/
/**
 * vFnBtoh: transforma un byte en ascii ( formato hexa )
 * @param number numero a transformar
 * @param stBuffer posicion de memoria donde se almacenara el numero
 */
/*void
vFnBtoh (byte byNumero, char *stBuffer)
{
  static byte stabyAux;
  stabyAux = byNumero;

  do
    {
      *stBuffer++ = 48;
    }
  while ((stabyAux /= 16) != 0);

  *stBuffer-- = '\0';
  while (byNumero != 0)
    {
      stabyAux = byNumero % 16;
      byNumero = byNumero / 16;
      if (stabyAux < 10)
	*stBuffer-- = stabyAux + 48;
      else
	*stBuffer-- = stabyAux + 55;
    }
}
*/
/**
 * vFnWtoa: transforma un word en ascii
 * @param number numero a transformar
 * @param stBuffer posicion de memoria donde se almacenara el numero
 */
/*void
vFnWtoa (word wNumero, char *stBuffer)
{
  static word stawAux;
  stawAux = wNumero;

  do
    {
      *stBuffer++ = 48;
    }
  while ((stawAux /= 10) != 0);

  *stBuffer-- = '\0';
  while (wNumero != 0)
    {
      stawAux = wNumero % 10;
      wNumero = wNumero / 10;
      *stBuffer-- = stawAux + 48;
    }
}
*/
/**
 * vFnBtoa: transforma un byte en ascii
 * @param number numero a transformar
 * @param stBuffer posicion de memoria donde se almacenara el numero
 */
/*void
vFnBtoa (byte byNumero, char *stBuffer)
{
  static byte stabyAux;
  stabyAux = byNumero;

  do
    {
      *stBuffer++ = 48;
    }
  while ((stabyAux /= 10) != 0);

  *stBuffer-- = '\0';
  while (byNumero != 0)
    {
      stabyAux = byNumero % 10;
      byNumero = byNumero / 10;
      *stBuffer-- = stabyAux + 48;
    }
}
*/


/******************************************************************************
Función: iFnHtoi
Descripción:  Recibe una cadena de carateres en un formato hexa predeterminado
              y devuelve el equivalente en decimal.

Recibe:   *Puntero a la cadena a interpretar, (terminada en '/0' !!!!!)
Devuelve: *Integer con el valor decimal luego de la conversión.

Fecha última modificación: 09/04/2006
*******************************************************************************/
/*int
iFnHtoi (char *c)
{
  int iN, iJ = 0;
  for (; (*c != '\0');)
    {
      iN = *c++ - 48;
      9 < iN ? iN -= 7 : iN;
      iJ <<= 4;
      iJ |= iN & 15;
    }
  //vFnImprimir ("\nEl Valor ingresado en Hexa es igual a %d en decimal", iJ);
  return iJ;
}
*/

/******************************************************************************
Función: iFnXtoi
Descripción:  Recibe una cadena de carateres en un formato hexa o entero
              y devuelve el equivalente en decimal. Para ello interpreta la
              naturaleza de la cadena y llama a iFnXtoi si es hexa o a iFnCtoi si
              es entero. En el caso que sea hexa, normaliza el string para
              que termine en /0 y que no contenga el "0x" inicial o el "h"
              final.

Recibe:   *Puntero a la cadena a interpretar, (terminada en '/0' !!!!!)
Devuelve: *Integer con el valor decimal luego de la conversión.

Fecha última modificación: 09/04/2006
*******************************************************************************/
/*
int
iFnXtoi (char *c)
{
  int tipo = 0;
  char *p = c;
  while (*c != '\0')
    {
      if (*c == 'x' || *c == 'h')
	{
	  if (*c == 'x')
	    {
	      p += 2;
	    }
	  if (*c == 'h')
	    {
	      *c = '\0';
	    }
	  tipo = 1;
	}
      c++;
    }

  if (tipo == 0)
    {
      return (iFnCtoi (p));
    }
  else
    {
      return (iFnHtoi (p));
    }
}
*/





