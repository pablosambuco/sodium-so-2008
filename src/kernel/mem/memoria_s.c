#include <kernel/mem/memoria_s.h>
#include <kernel/libk/string.h>
#include <shell/teclado.h>

extern int ulMemoriaBase;

extern int ulMemoriaTope;

extern int iFmf, iCcmf, iMatrizMf[30][2];

// Matriz de 30 filas por 2 columnas donde se almacenaran las reservas y liberaciones de memoria.
// Se guarda [-1, posfinal] para especificar un segm. de memoria libre.
// Se guarda [posini, posfinal] para especificar un segm. de memoria alocado.
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnParticionarMemoria ()
{
  int iN;

  iMatrizMf[0][0] = -1;
  iMatrizMf[0][1] = ulMemoriaBase + 10240;	//Primer incremento de 10K

  for (iN = 1; iN < 29; iN++)
    {
      iMatrizMf[iN][0] = -1;
      iMatrizMf[iN][1] = iMatrizMf[iN - 1][1] + 1 + (iN + 1) * 10240;	//(iN+1)*10240 cálculo del incremento
    }

  iMatrizMf[29][0] = -1;
  iMatrizMf[29][1] = ulMemoriaTope;	//Ultimo incremento hasta memoria tope
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
//SEGMENTO_MAX
//Muestra el tamaño del segmento libre más grande
int
iFnSegmentoMaximo ()
{
  int iN;

  for (iN = 29; iN >= 0; iN--)
    {
      if (iMatrizMf[iN][0] == -1)
    	return (iMatrizMf[iN][1] - (iMatrizMf[iN - 1][1] + 1));
    }

  //no hay segmentos libres
  return -1;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
	    //MOSTRAR_MEM_SEG
//Muestra los segmentos ocupados en la memoria segmentada
void
vFnMostrarMemoriaSegmentada ()
{
  int iN = 0, iJ = 0;

  vFnImprimir ("\n#### SEGMENTOS OCUPADOS ####");

  while (iN < 30)
    {
      if (iMatrizMf[iN][0] != -1)	//Si el segmento está ocupado lo muestra
	{
	  vFnImprimir ("\nSegmento Nro. %d:\t", iN);
	  vFnImprimir ("Inicio: 0x%xh  ", iMatrizMf[iN][0]);
	  vFnImprimir ("Fin: 0x%xh  ", iMatrizMf[iN][1]);
	  vFnImprimir ("Tam: %d bytes", iMatrizMf[iN][1] - iMatrizMf[iN][0]);
	  iJ++;
	}
      if ((iJ % 14 == 0) && (iJ > 0))
	cFnPausa ();
      iN++;
    }
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
// MALLOC
// Recibe como parametro la cant. de memoria a reservar en bytes.
// Devuelve la direccion base del segmento reservado.
// Si no pudo liberar u ocurrio un error en el proceso de liberacion devuelve -1.

int
iFnMalloc (int iCantidadMemoria)
{				//1
  int iCantMemSegm = 0;		// Variable para calculos internos.

//TODO - lala
vFnImprimir("\n\nME QUIERO MORIRRRR!!!\n\n");

// Valida que no quiera reservar mas memoria de la total disponible.
  if ((ulMemoriaBase + iCantidadMemoria) < ulMemoriaTope)
    {				//2
      for (iFmf = 0; iFmf < 30; iFmf++)
	{			//3
	  // Busca en los segmentos de memoria libres.
	  // Se guarda [-1, posfinal] para especificar un segm. de memoria libre.
	  if ((iMatrizMf[iFmf][0] == -1))
	    {			//4
	      iCantMemSegm = 0;	// Se resetea.
	      if (iFmf == 0)	// Primer segmento libre.
		{
		  // Cant = PosFinalBloqueActualLibre - MemoriaBase
		  iCantMemSegm = (iMatrizMf[iFmf][1] - ulMemoriaBase);
		}
	      else
		{
		  // Cant = (PosFinalBloqueActualLibre- PosFinalBloqueAnt + 1)
		  iCantMemSegm =
		    (iMatrizMf[iFmf][1] - (iMatrizMf[iFmf - 1][1] + 1));
		}
	      // Si la cantidad libre del segmento >= a la Cant. de Memoria a Alocar.
	      if (iCantMemSegm >= iCantidadMemoria)
		{
		  if (iFmf == 0)	// Primer segmento liberado.
		    {
		      // Base = Memoria Base.
		      iMatrizMf[iFmf][0] = ulMemoriaBase;
		    }
		  else
		    {
		      // Base = Tope anterior + 1.
		      iMatrizMf[iFmf][0] = iMatrizMf[iFmf - 1][1] + 1;
		    }
		  return iMatrizMf[iFmf][0];	// Retorna direccion base alocada.
		}
	    }			//4
	}			// 3 For.
    }				//2
  return -1;			// Si recorrio todo el For y no pudo alocar memoria, o si la memoria disponible no alcanza.
}				//1


/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
// FREE
// Recibe como parametro la direccion base a liberar, y devuelve la cant. de memoria liberada.
// Si no pudo liberar u ocurrio un error en el proceso de liberacion devuelve -1.
int
iFnFree (int iCantidadMemoria)
{
  int iMemoriaLiberada;

  if (iCantidadMemoria >= 0)
    {
      // Se recorre la matriz buscando la posicion base a liberar en la columna 1.
      for (iFmf = 0; iFmf < 30; iFmf++)

	if (iMatrizMf[iFmf][0] == iCantidadMemoria)
	  {
	    // Memoria Liberada = Posicion base - Posicion tope + 1.
	    // Ej.: base: 3   tope: 6    memoria liberada = 4 bytes.
	    iMemoriaLiberada = iMatrizMf[iFmf][1] - iMatrizMf[iFmf][0] + 1;

	    // Se guarda [-1, posfinal] para especificar un segm. de memoria liberado.
	    iMatrizMf[iFmf][0] = -1;

	    return iMemoriaLiberada;
	  }
    }
  return -1;			// Si no encontro el segmento a liberar.
}
