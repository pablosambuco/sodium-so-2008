#include <kernel/mem/paginas.h>
#include <shell/shell.h>
#include <kernel/pcb.h>
#include <video.h>

static struct stuBitMap *pstuBitMapTablaFrames = 0;


int iFnBuscarVacio ();
int iFnVerificarSuficientes (int iCantidadRequerida);
struct stuTablaPagina *stuTablaPaginaFnAsignarPaginas (unsigned int
						       uiTamanio);
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnIniciarMapaBits (int iPosicionBase, int iTamanio)
{
/*************************************************************************
esta funcion se encarga de : a partir de una posicion de memoria
especifica, pasado por parametro(posBase), y segun la cantidad
de memoria, tambien pasada por parametro(tam), del computador genera el
mapa de bits vacio y ademas setea cuantas tabalas de paginas entranen una
pagina, esto es para luego asignarle los frames a los procesos.
Esta funcion guarda tambien el espacio reservado por el kernel, mas el
mapa de bits
.
*************************************************************************/
  int iN, iJ, iAux;
  iTamanioMemoria = (iTamanio / TAMANIOPAGINA) / 8;
  pstuBitMapTablaFrames = (struct stuBitMap *) iPosicionBase;	//calcula en donde meter el mapa de bits
  //si no los pasan por parametros esto desaparece
//se setea toda la tabla a cero
  for (iN = 0; iN < iTamanioMemoria; iN++)
    {
      pstuBitMapTablaFrames[iN].bit0 = 0;
      pstuBitMapTablaFrames[iN].bit1 = 0;
      pstuBitMapTablaFrames[iN].bit2 = 0;
      pstuBitMapTablaFrames[iN].bit3 = 0;
      pstuBitMapTablaFrames[iN].bit4 = 0;
      pstuBitMapTablaFrames[iN].bit5 = 0;
      pstuBitMapTablaFrames[iN].bit6 = 0;
      pstuBitMapTablaFrames[iN].bit7 = 0;
    }
/*se asignan las paginas para el sistema operativo el valor de este es dado
por la variable tamanoSO y se le agrega el espacio ocupado por el mapa de bits
*/
  iAux = sizeof (struct stuBitMap) * iN / TAMANIOPAGINA;
  if (sizeof (struct stuBitMap) * iN % TAMANIOPAGINA != 0)
    iAux++;
  for (iJ = 0;
       iJ <
       ((iPosicionBase / TAMANIOPAGINA) + (iPosicionBase % TAMANIOPAGINA) +
	iAux); iJ++)
    iFnBuscarVacio ();

  iLimCantPaginasProceso = TAMANIOPAGINA / sizeof (struct stuTablaPagina);
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
struct stuTablaPagina *
pstuTablaPaginaFnAsignarPaginas (unsigned int uiTamanio)
{
/*************************************************************************
esta funcion se encarga de : crear una tabla local con las paginas
asociadas a cada tarea a partir de los siguientes datos:
	*la cantidad de memoria requerida
por el proceso
la coloca en una pagina y si el tamanio del proceso supera el espacio de una pagina
se buscan mas y se guerada en que frame se encuentran las proximas paginas del
proceso, y coloca la referencia a esa direccion (de la primer pagina) en la pcb
*************************************************************************/
  int iN = 0, iCantidad, iCantTPaginasProceso, iJ, iAux;
  struct stuTablaPagina *pstuTablaPaginaTpProceso,
    *pstuTablaPaginaTpProcesoAux;
  iCantidad = uiTamanio / TAMANIOPAGINA;
  if ((uiTamanio % TAMANIOPAGINA) != 0)
    iCantidad++;
  iCantTPaginasProceso = iCantidad / (iLimCantPaginasProceso - 1);
  if ((iCantidad % (iLimCantPaginasProceso - 1)) != 0)
    iCantTPaginasProceso++;
  if (iFnVerificarSuficientes (iCantidad + iCantTPaginasProceso) == 1)	//verifica si hay suficientes frames libres para poder alojar todo el proceso en memoria, mas la cantidad de pagians para las tablas de paginas
    {
      pstuTablaPaginaTpProceso = (struct stuTablaPagina *) (iFnBuscarVacio () * TAMANIOPAGINA);	//se busca el lugar en donde realizar la asignacion de la tabla
      pstuTablaPaginaTpProcesoAux = pstuTablaPaginaTpProceso;
      iJ = 0;
      iAux = 0;
      while (iJ < iCantTPaginasProceso)
	{
	  iN = 0;
	  while ((iN < iLimCantPaginasProceso - 1) && (iAux < iCantidad))
	    {
	      //pstuTablaPaginaTpProcesoAux[iN].pag=iAux;
	      pstuTablaPaginaTpProcesoAux[iN].uiFrame = iFnBuscarVacio ();	//lo que se va haciendo es completar la tabla de paginas segun lo que le debuelve buscar vacio (ver especificacion de esta funcion)
	      pstuTablaPaginaTpProcesoAux[iN].bUsado = 0;	//todos los frames que se usen completamente tendran en este campo un 0
	      iAux++;
	      iN++;
	    }
	  if ((iAux == iCantidad) && (iN < iLimCantPaginasProceso - 1))
	    {
	      pstuTablaPaginaTpProcesoAux[iN - 1].bUsado = uiTamanio % TAMANIOPAGINA;	//el ultimo frame sera siempre el que se use parcialmente, en el campo de usados se especifican la cantidad de bytes que fueron usadas
	      iJ = 0;
	      iAux = 0;
	      return pstuTablaPaginaTpProceso;	//retorno el puntero a la direccion de la primer tabla para hallar todas las restantes
	    }
	  else if (iN == iLimCantPaginasProceso - 1)
	    {
	      pstuTablaPaginaTpProcesoAux[iN].uiFrame = iFnBuscarVacio ();	//guardo donde va ha estar la continuacion de la tabla de paginas para este proceso
	      pstuTablaPaginaTpProcesoAux = (struct stuTablaPagina *) (pstuTablaPaginaTpProcesoAux[iN].uiFrame * TAMANIOPAGINA);	//me ubico en la proxima tabla de paginas para el proceso
	      iJ++;
	    }
	}
    }
/*  else
								    {
*/ return NULL;			//si no se pudo asignar por falta de memoria devuelve null
  //   }

}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int
iFnVerificarSuficientes (int iCantidadRequerida)
{
 /************************************************************************
 esta funcion verifica que halla suficientes frames libres para alojar
 todo el proceso en memoria
 lo hace recorriendo el mapa y contando los ceros!!!!
 retorna un 1 si hay suficientes y un cero si no los hay
 ************************************************************************/
  int iN = 0;
  int iContadorHuecos = 0;

  for (; ((iN < iTamanioMemoria) && (iContadorHuecos < iCantidadRequerida));
       iN++)
    {
      if (pstuBitMapTablaFrames[iN].bit0 == 0)
	iContadorHuecos++;
      if (pstuBitMapTablaFrames[iN].bit1 == 0)
	iContadorHuecos++;
      if (pstuBitMapTablaFrames[iN].bit2 == 0)
	iContadorHuecos++;
      if (pstuBitMapTablaFrames[iN].bit3 == 0)
	iContadorHuecos++;
      if (pstuBitMapTablaFrames[iN].bit4 == 0)
	iContadorHuecos++;
      if (pstuBitMapTablaFrames[iN].bit5 == 0)
	iContadorHuecos++;
      if (pstuBitMapTablaFrames[iN].bit6 == 0)
	iContadorHuecos++;
      if (pstuBitMapTablaFrames[iN].bit7 == 0)
	iContadorHuecos++;
    }
  return (iContadorHuecos >= iCantidadRequerida) ? 1 : 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int
iFnBuscarVacio ()
{
/*********************************************************************
lo que hace esta funcion es: encuentra el primer espacio vacio, lo marca como
ocupado y devuelve el frame que ocupo.
 El algoritmo utilizado es first fit
*********************************************************************/
  int iN = 0;

  for (; iN < iTamanioMemoria; iN++)
    {
      if (pstuBitMapTablaFrames[iN].bit0 == 0)
	{
	  pstuBitMapTablaFrames[iN].bit0 = 1;
	  return (iN * 8 + 0);
	}
      if (pstuBitMapTablaFrames[iN].bit1 == 0)
	{
	  pstuBitMapTablaFrames[iN].bit1 = 1;
	  return (iN * 8 + 1);
	}
      if (pstuBitMapTablaFrames[iN].bit2 == 0)
	{
	  pstuBitMapTablaFrames[iN].bit2 = 1;
	  return (iN * 8 + 2);
	}
      if (pstuBitMapTablaFrames[iN].bit3 == 0)
	{
	  pstuBitMapTablaFrames[iN].bit3 = 1;
	  return (iN * 8 + 3);
	}
      if (pstuBitMapTablaFrames[iN].bit4 == 0)
	{
	  pstuBitMapTablaFrames[iN].bit4 = 1;
	  return (iN * 8 + 4);
	}
      if (pstuBitMapTablaFrames[iN].bit5 == 0)
	{
	  pstuBitMapTablaFrames[iN].bit5 = 1;
	  return (iN * 8 + 5);
	}
      if (pstuBitMapTablaFrames[iN].bit6 == 0)
	{
	  pstuBitMapTablaFrames[iN].bit6 = 1;
	  return (iN * 8 + 6);
	}
      if (pstuBitMapTablaFrames[iN].bit7 == 0)
	{
	  pstuBitMapTablaFrames[iN].bit7 = 1;
	  return (iN * 8 + 7);
	}
    }
  return -1;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnLimpiarFrame (int iFrame)
{
/*********************************************************************
lo que hace esta funcion es exactamente lo contrario que la anterior
le pasan una direccion y la marca como desocupada
*********************************************************************/
  int iPosicion, iBit;
  iPosicion = iFrame / 8;
  iBit = iFrame % 8;
  if (iBit == 0)
    pstuBitMapTablaFrames[iPosicion].bit0 = 0;
  if (iBit == 1)
    pstuBitMapTablaFrames[iPosicion].bit1 = 0;
  if (iBit == 2)
    pstuBitMapTablaFrames[iPosicion].bit2 = 0;
  if (iBit == 3)
    pstuBitMapTablaFrames[iPosicion].bit3 = 0;
  if (iBit == 4)
    pstuBitMapTablaFrames[iPosicion].bit4 = 0;
  if (iBit == 5)
    pstuBitMapTablaFrames[iPosicion].bit5 = 0;
  if (iBit == 6)
    pstuBitMapTablaFrames[iPosicion].bit6 = 0;
  if (iBit == 7)
    pstuBitMapTablaFrames[iPosicion].bit7 = 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int
iFnLimpiarMemoria (int iTamanio, struct stuTablaPagina *pstuTablaPaginaTabla)
{
 /*****************************************************************
 esta funcion se usa cuando el proceso se muere o lo matan, desaloja
 todas las paginas que tenia asignado ese proceso.
 lo hace llamando n veces a la subrutina limpiar frame
 donde n es la cantidad de entradas que tiene la tabla de pagina
 y esto lo realiza recorriendo la tabla de pagina y liberando los
 frames por ultimo libera el frame ocupado para la tabla de pagina
 ****************************************************************/
  int iN, iCantidad, iAux, iCantTPaginasProceso, iJ;
  struct stuTablaPagina *pstuTablaPaginaTpProceso,
    *pstuTablaPaginaTpProcesoAux;
  iCantidad = iTamanio / TAMANIOPAGINA;
  if ((iTamanio % TAMANIOPAGINA) != 0)
    iCantidad++;

  iCantTPaginasProceso = iCantidad / (iLimCantPaginasProceso - 1);
  if ((iCantidad % (iLimCantPaginasProceso - 1)) != 0)
    iCantTPaginasProceso++;
  pstuTablaPaginaTpProceso = pstuTablaPaginaTabla;
  pstuTablaPaginaTpProcesoAux = pstuTablaPaginaTpProceso;
  iJ = 0;
  iAux = 0;
  while (iJ < iCantTPaginasProceso)
    {
      iN = 0;
      while ((iN < iLimCantPaginasProceso - 1) && (iAux < iCantidad))
	{
	  vFnLimpiarFrame (pstuTablaPaginaTpProcesoAux[iN].uiFrame);
	  iAux++;
	  iN++;
	}
      if ((iAux == iCantidad) && (iN < iLimCantPaginasProceso - 1))
	{
	  vFnLimpiarFrame ((((int) pstuTablaPaginaTpProceso) /
			    TAMANIOPAGINA));
	  break;
	}
      else if (iN == iLimCantPaginasProceso - 1)
	{
	  pstuTablaPaginaTpProcesoAux =
	    (struct stuTablaPagina *) (pstuTablaPaginaTpProceso[iN].uiFrame *
				       TAMANIOPAGINA);
	  vFnLimpiarFrame ((((int) pstuTablaPaginaTpProceso) /
			    TAMANIOPAGINA));
	  pstuTablaPaginaTpProceso = pstuTablaPaginaTpProcesoAux;
	  iJ++;
	}
    }
  return 1;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnImprimirTablaPaginas (int iTamanio,
			 struct stuTablaPagina *pstuTablaPaginaTabla)
{
/******************************************************************
imprime la tabla de paginas de un proceso determinado
*******************************************************************/
  int iN, iCantidad, iAux, iCantTPaginasProceso, iJ;
  struct stuTablaPagina *pstuTablaPaginaTpProcesoAux;
  iCantidad = iTamanio / TAMANIOPAGINA;
  if ((iTamanio % TAMANIOPAGINA) != 0)
    iCantidad++;

  vFnImprimir ("\n\tPaginas\tFrames\tUsado");

  iCantTPaginasProceso = iCantidad / (iLimCantPaginasProceso - 1);
  if ((iCantidad % (iLimCantPaginasProceso - 1)) != 0)
    iCantTPaginasProceso++;
  pstuTablaPaginaTpProcesoAux = pstuTablaPaginaTabla;
  iJ = 0;
  iAux = 0;
  while (iJ < iCantTPaginasProceso)
    {
      iN = 0;
      while ((iN < iLimCantPaginasProceso - 1) && (iAux < iCantidad))
	{
	  vFnImprimir ("\n\t%d\t%d", iAux,
		       pstuTablaPaginaTpProcesoAux[iN].uiFrame);
	  if (pstuTablaPaginaTpProcesoAux[iN].bUsado == 0)
	    {
	      vFnImprimir ("\tCompleto");
	    }
	  else
	    {
	      vFnImprimir ("\t%d", pstuTablaPaginaTpProcesoAux[iN].bUsado);
	    }
	  if (iN % 10 == 0 && iN != 0)
	    cFnPausa ();
	  iAux++;
	  iN++;
	}
      if ((iAux == iCantidad) && (iN < iLimCantPaginasProceso - 1))
	{
	  iJ = 0;
	  iAux = 0;
	  break;
	}
      else if (iN == iLimCantPaginasProceso - 1)
	{
	  pstuTablaPaginaTpProcesoAux =
	    (struct stuTablaPagina *) (pstuTablaPaginaTpProcesoAux[iN].
				       uiFrame * TAMANIOPAGINA);
	  iJ++;
	}
    }
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnImprimirMapaBits ()
{
/******************************************************************
imprime el mapa de bits
*******************************************************************/
  int iN = 0;
  int iContador = 0;
  int iContadorlinea = 0;
  unsigned int uiDireccionFrame = 0;

  vFnImprimir ("\n\n");

  uiDireccionFrame = iN * 8 * 4096;
  vFnImprimir ("Direccion fisica primer frame %d.\n", uiDireccionFrame);

  for (iN = 0; iN < iTamanioMemoria; iN++)
    {

      iContador++;

      if (pstuBitMapTablaFrames[iN].bit0 == 0)
	vFnImprimir ("0");
      else
	vFnImprimir ("1");
      if (pstuBitMapTablaFrames[iN].bit1 == 0)
	vFnImprimir ("0");
      else
	vFnImprimir ("1");
      if (pstuBitMapTablaFrames[iN].bit2 == 0)
	vFnImprimir ("0");
      else
	vFnImprimir ("1");
      if (pstuBitMapTablaFrames[iN].bit3 == 0)
	vFnImprimir ("0");
      else
	vFnImprimir ("1");
      vFnImprimir (" ");
      if (pstuBitMapTablaFrames[iN].bit4 == 0)
	vFnImprimir ("0");
      else
	vFnImprimir ("1");
      if (pstuBitMapTablaFrames[iN].bit5 == 0)
	vFnImprimir ("0");
      else
	vFnImprimir ("1");
      if (pstuBitMapTablaFrames[iN].bit6 == 0)
	vFnImprimir ("0");
      else
	vFnImprimir ("1");
      if (pstuBitMapTablaFrames[iN].bit7 == 0)
	vFnImprimir ("0");
      else
	vFnImprimir ("1");
      vFnImprimir (" ");

      if (iContador > 6)
	{
	  vFnImprimir ("\n");
	  iContador = 0;
	  iContadorlinea++;
	}

      if (iContadorlinea > 10)
	{
	  iContadorlinea = 0;
	  uiDireccionFrame = iN * 8 * 4096;
	  vFnImprimir ("Direccion fisica ultimo frame: %d.",
		       uiDireccionFrame);
	  cFnPausa ();
	  vFnImprimir ("\n");
	}

    }
  uiDireccionFrame = iN * 8 * 4096;
  vFnImprimir ("\nDireccion fisica ultimo frame: %d.\n", uiDireccionFrame);
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int
iFnDarDireccion (struct stuTablaPagina *stuTablaPaginaTabla, int iTamanio,
		 int iPagina, int iOffset)
{
 /*******************************************************
 Devuelve la direccion lineal de memoria que el proceso requirio
 retornando distintos mensajes de error en caso que esto suceda:
 si quere acceder a una direccion no valida tira -2 si lapagina
 es incorrecta o -3
 si el offset es incorrecto.
 Devuelve el frame si la direccion esta bien ya que el offset lo tiene
 el shell o de donde me llamaron
 ************************************************************/
  int iCantidadPaginas, iJ, iFrame = 0, iPosicion;
  int iAux, iCantTPaginasProceso;
  struct stuTablaPagina *pstuTablaPaginaTpProcesoAux;
  iCantidadPaginas = iTamanio / TAMANIOPAGINA;
//se calculan cuantas paginas tiene el proceso
  if ((iTamanio % TAMANIOPAGINA) != 0)
    iCantidadPaginas++;
//si la pagina que se requirio es mas grande que la cantidad de pags del proceso
//esta intentando de acceder a una pagina que no es suya
  if (iPagina >= iCantidadPaginas)
    return (-2);		//-2 es el codigo de error por pagina invalida
//verifica que el offset este dentro de la pagina
  iCantTPaginasProceso = iCantidadPaginas / (iLimCantPaginasProceso - 1);
  if ((iCantidadPaginas % (iLimCantPaginasProceso - 1)) != 0)
    iCantTPaginasProceso++;
  pstuTablaPaginaTpProcesoAux = stuTablaPaginaTabla;
  iJ = 0;
  iAux = 1;

  while (iPagina >= (iAux * iLimCantPaginasProceso - 1))
    {
      pstuTablaPaginaTpProcesoAux =
	(struct stuTablaPagina *)
	pstuTablaPaginaTpProcesoAux[iLimCantPaginasProceso - 1].uiFrame;
      iAux++;
    }
  iPosicion =
    (iLimCantPaginasProceso - 1) - ((iAux * iLimCantPaginasProceso - 1) -
				    iPagina);
  if (pstuTablaPaginaTpProcesoAux[iPosicion].bUsado != 0)
    if (iOffset >= pstuTablaPaginaTpProcesoAux[iPosicion].bUsado)
      return (-3);		//-3 es el codigo de offset invalido
  iFrame = pstuTablaPaginaTpProcesoAux[iPosicion].uiFrame;	//sino devuelve el frame en donde hay que buscar


  if (iOffset >= TAMANIOPAGINA)
    return (-3);		//si es mayor a 4K obviamente es invalido
  else if (iFrame == 0)
    return (-3);
  else
    return (iFrame * TAMANIOPAGINA + iOffset);	//si no hay frame que haga referencia a esa pagina -3
  //sino se devuelve el offset
}
