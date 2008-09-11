#include <kernel/mem/memoria_s.h>
#include <kernel/libk/string.h>
#include <shell/teclado.h>

extern unsigned int uiTamanioMemoriaBios;



//TODO - Revisar
extern int ulMemoriaBase;
extern int ulMemoriaTope;
extern int iFmf, iCcmf, iMatrizMf[30][2];



/* TODO
 * Revisar las variables:
 *  - ulMemoriaBase: deberia valer INICIO_MEMORIA_ALTA + TAMANIO_HEAP_KERNEL
 *  - ulMemoriaTope: no deberia estar hardcodeado, deberia ser lo informado por
 *                   la BIOS
 */

/* TODO
 * Describir la estrategia utilizada para administracion de memoria
 */


/**
 * @brief Crea el primer bloque de memoria libre. El tamanio del bloque sera
 * toda la memoria del sistema por arriba del Heap del Kernel
 * @date 08-02-2008
 */
void vFnInicializarMemoriaSegmentada() {
/*    //TODO Reemplazar la suma de direcciones por MEMORIA_BASE cuando se actualice su valor
    uiTotalMemoriaLibre = uiTamanioMemoriaBios - (INICIO_MEMORIA_ALTA + TAMANIO_HEAP_KERNEL);

    //TODO Reemplazar la suma de direcciones por MEMORIA_BASE cuando se actualice su valor
    stuListaSegmentosLibres.pNodoSig = (void *) INICIO_MEMORIA_ALTA + TAMANIO_HEAP_KERNEL;
    stuListaSegmentosLibres.nTamanio = 0;

    //NOTA nTamanio incluye la informacion de control (t_nodo)
    (stuListaSegmentosLibres.pNodoSig)->nTamanio = uiTotalMemoriaLibre;
    (stuListaSegmentosLibres.pNodoSig)->pNodoSig = NULL;

    vFnLog("\nINICIALIZANDO MEMORIA: %dMb disponibles para procesos",
            uiTotalMemoriaLibre >> 20);
*/
}


/**
 * @brief Busca un bloque de memoria libre de uiTamanio bytes
 * @param Tamanio en bytes del bloque libre buscado
 * @returns La direccion del nodo de la lista de bloques libres que apunta al
 * bloque libre encontrado (nodo anterior) o 0 si no se encontraron bloques
 * (se debe castear a t_nodo*)
 * @date 08-02-2008
 */
#if 0
void * pvFnBuscarNodoAnteriorMemoriaLibre(unsigned int uiTamanioDeseado ) {
    t_nodo* pBloqueLibre;

    /* Se busca un bloque que pueda albergar el segmento
     * Si el nuevo segmento no entra 'justo' (justo = no sobra ni un byte), se
     * verifica que no solo entre el uiTamanioDeseado, sino tambien el nodo que
     * se tendra que insertar al 'partir' el bloque
     */
    pBloqueLibre = &stuListaSegmentosLibres;
    while( pBloqueLibre->pNodoSig != NULL &&
          pBloqueLibre->pNodoSig->nTamanio != uiTamanioDeseado &&
          pBloqueLibre->pNodoSig->nTamanio < uiTamanioDeseado+sizeof(t_nodo) ) {
                pBloqueLibre = pBloqueLibre->pNodoSig;
    }

    if( pBloqueLibre->pNodoSig != NULL ) {
            //Se encontro un bloque que sirve para crear el nuevo segmento
            vFnLog("\nSe encontro un bloque de memoria libre de %dKb",
                    pBloqueLibre->pNodoSig->nTamanio >> 10);

            return (void *) pBloqueLibre;
    }

    return NULL;
}
#endif


/**
 * @brief Reserva un segmento de memoria (no crea descriptores, solo quita el
 * espacio de la lista de libres)
 * @param Tamanio en bytes del bloque libre a reservar
 * @returns La direccion del bloque libre o 0 si no se encontro ninguno
 * @date 08-02-2008
 */
void * pvFnReservarSegmento(unsigned int uiTamanioDeseado) {
    return pvFnKMalloc(uiTamanioDeseado, MEM_ALTA | MEM_USUARIO);
/*    t_nodo * pNodoAnteriorAlSegmento;
    t_nodo * pDireccionSegmento;
    t_nodo * pNuevoBloqueLibre;

    //Existe espacio libre suficiente?
    if( uiTotalMemoriaLibre > uiTamanioDeseado ) {

        //Se busca un bloque libre
        //La funcion pvFnBuscarNodoAnteriorMemoriaLibre retorna un puntero al
        //nodo de la lista de libres que precede al bloque encontrado
        pNodoAnteriorAlSegmento =
            (t_nodo*)pvFnBuscarNodoAnteriorMemoriaLibre( uiTamanioDeseado );

        //Si no existe un bloque libre que sirva para albergar el segmento,
        //se debe defragmentar la memoria
        if( pNodoAnteriorAlSegmento == NULL ) {
            vFnLog("\npvFnReservarSegmento: No hay %d bytes libres contiguos. "
                    "Debe defragmentarse", uiTamanioDeseado); 
            //TODO - Defragmentar memoria y sacar el return
            return NULL;
        }

        //Existe espacio libre suficiente y es contiguo

        //Se crea el 'segmento', actualizando la lista de bloques libres
        pDireccionSegmento = pNodoAnteriorAlSegmento->pNodoSig;
        pNodoAnteriorAlSegmento->pNodoSig = pDireccionSegmento->pNodoSig;

        //Si el nuevo segmento no entra 'justo' en el bloque libre, se debe
        //crear un nuevo bloque libre. pvFnBuscarNodoAnteriorMemoriaLibre ya se
        //encargo de darnos un bloque en el que el nuevo segmento cabe justo o
        //permite albergar un nuevo nodo.
        if(pDireccionSegmento->nTamanio > uiTamanioDeseado) {
            //Se calcula la direccion del nuevo bloque libre
            pNuevoBloqueLibre = (t_nodo *)
                                ((char*)pDireccionSegmento + uiTamanioDeseado);
            //Se calcula su tamanio
            pNuevoBloqueLibre->nTamanio = pDireccionSegmento->nTamanio -
                                            uiTamanioDeseado;
            //Se lo agrega a la lista de libres
            vFnInsertarBloqueLibreEnListaOrd( pNuevoBloqueLibre );
        }
        
        //Se descuenta la memoria asignada del total libre
        uiTotalMemoriaLibre -= uiTamanioDeseado;
        
        vFnLog("\npvFnReservarSegmento: Se reservan %d bytes. Quedan %d bytes"
                " libres", uiTamanioDeseado, uiTotalMemoriaLibre);

        return (void *) pDireccionSegmento;
    }

    vFnLog("\npvFnReservarSegmento: Imposible reservar memoria; no hay %d bytes"
            " libres", uiTamanioDeseado); 

    return NULL;
*/
}


/**
 * @brief Agrega un segmento de memoria a la lista de bloques libres
 * @param Direccion de inicio del segmento
 * @param Tamanio del segmento (en bytes)
 */
void vFnLiberarSegmento( void * pInicioSegmento,
                         unsigned int uiTamanioSegmento ) {
    pvFnKFree(pInicioSegmento);
#if 0
    t_nodo* pNuevoLibre;
    t_nodo* pAux;
    t_nodo* pAModificar;

    //Creo un nuevo nodo para la lista de bloques libres. Todavia no lo enalzo.
    pNuevoLibre = (t_nodo*) pInicioSegmento;
    pNuevoLibre->nTamanio = uiTamanioSegmento;

    /* Recorro la lista de bloques vacios buscando un bloque libre ANTERIOR que
     * sea adyacente al segmento a liberar. Guardo la direccion de los nodos que
     * los apuntan (y no la de los nodos en si mismos) por si deben moverse.
     */
    pAux = &stuListaSegmentosLibres;
    while( pAux->pNodoSig != NULL &&
            (char *)pNuevoLibre != ((char *)pAux->pNodoSig +
                                                pAux->pNodoSig->nTamanio) ) {
        pAux = pAux->pNodoSig;
    }

    //Si el bloque libre ANTERIOR es adyacente al nuevo, los unifico y quito el
    //nuevo bloque de la lista, dejandola ordenada
    if( pAux->pNodoSig != NULL) {
            pAModificar = pAux->pNodoSig;
            pAux->pNodoSig = pAModificar->pNodoSig;

            pAModificar->nTamanio += pNuevoLibre->nTamanio;
            pNuevoLibre = pAModificar;
    }

    /* Recorro la lista de bloques vacios buscando un bloque libres POSTERIOR
     * que sea adyacente al segmento a liberar. Guardo la direccion de los nodos
     * que los apuntan (y no la de los nodos en si mismos) por si deben moverse.
     */
    pAux = &stuListaSegmentosLibres;
    while( pAux->pNodoSig != NULL &&
            (char *)pAux->pNodoSig != ((char *)pNuevoLibre +
                                                pNuevoLibre->nTamanio) ) {
        pAux = pAux->pNodoSig;
    }

    //Si el bloque libre POSTERIOR es adyacente al nuevo, los unifico y quito el
    //nuevo bloque de la lista, dejandola ordenada
    if( pAux->pNodoSig != NULL) {
            pAModificar = pAux->pNodoSig;
            pAux->pNodoSig = pAModificar->pNodoSig;

            pNuevoLibre->nTamanio += pAModificar->nTamanio;
            //pNuevoLibre = pNuevoLibre;
    }

    //Inserto el nuevo bloque libre en la lista
    vFnInsertarBloqueLibreEnListaOrd( pNuevoLibre );

    //Actualizo la variable global de espacio libre
    uiTotalMemoriaLibre += uiTamanioSegmento;

    vFnLog("\npvFnLiberarSegmento: Se liberan %d bytes. Quedan %d bytes libres",
            uiTamanioSegmento, uiTotalMemoriaLibre);
#endif
}


/**
 * @brief Inserta un nodo en la lista de bloques libres, ordenado por tamanio
 * @param Puntero al nodo a insertar
 */
/*void vFnInsertarBloqueLibreEnListaOrd( t_nodo * pNuevoNodo ) {
    t_nodo* pAux;

    //Busco la posicion de la lista en la cual insertar el nuevo nodo
    pAux = &stuListaSegmentosLibres;
    while( pAux->pNodoSig != NULL &&
           pAux->pNodoSig->nTamanio < pNuevoNodo->nTamanio) {
                pAux = pAux->pNodoSig;
    }

    //Inserto en la lista
    pNuevoNodo->pNodoSig = pAux->pNodoSig;
    pAux->pNodoSig = pNuevoNodo;
}*/


/**
 * @brief Muestra por pantalla la lista de bloques de memoria libre
 */
void vFnListarBloquesLibres() {
    vFnListarKMem();
/*    t_nodo *pNodoActual;
    unsigned int uiIndice;

    vFnImprimir("\nListado de bloques de memoria libres en memoria "
                "convencional");

    uiIndice = 1;
    pNodoActual = &stuListaSegmentosLibres;
    while ( (pNodoActual = pNodoActual->pNodoSig) != NULL) {
        vFnImprimir("\n\tEl bloque %d se encuentra en: %d, con un tamanio de "
                    "%d b", uiIndice, (unsigned int)pNodoActual,
                    pNodoActual->nTamanio);
        uiIndice++;
    }

    vFnImprimir("\n");*/
}









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
