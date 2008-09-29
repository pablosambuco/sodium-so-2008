#include <usr/libsodium.h>
#include <usr/memoria_dinamica.h>

//TODO - lala - sacar
#include <usr/sodstdio.h>


/* TODO
 * Describir la estrategia de Gestion de Memoria Dinamica a nivel usuario
 */


/*
 * \brief Lista de bloques libres en el Heap
 */
t_nodo stuListaBloquesLibres;


/**
 * @brief Reserva un bloque de memoria en el heap
 * @param Tamanio en bytes del bloque a reservar
 * @returns La direccion del bloque reservado o NULL si no se pudo reservar
 * @date 19-09-2008
 */
void * malloc (unsigned int uiTamanioDeseado) {
    t_nodo * pDirBloque;
    t_nodo * pNodoAnteriorAlBloque;
    t_nodo * pNuevoBloqueLibre;

    /* Direccion de CORTE (BREAK, direccion siguiente al fin del segmento).
     * Con la forma de organizacion de memoria elegida, el Heap se ubica al
     * final del segmento de datos; el fin del Heap concide con el fin del
     * segmento de datos.
     */
    char * pcFinDeHeap; 
    static char * pcAntiguoFinDeHeap; 

    static char stcExisteHeap = 0;
    
    if(uiTamanioDeseado == 0) {
        iFnImprimir_usr("\nmalloc: Si no quiere memoria, no moleste!");
        return NULL;
    }

    //Esta parte SOLO SE EJECUTA LA PRIMERA VEZ que se llama a malloc.
    //Se encarga de crear el Heap y de inicializar la lista de espacios libres.
    if( !stcExisteHeap ) {

        stuListaBloquesLibres.pNodoSig = NULL;

        //Se crea el Heap, agrandando el segmento
        if( iFnAgrandarHeap( HEAP_TAMANIO_INICAL,
                              &pcAntiguoFinDeHeap, &pcFinDeHeap) != 0 ) {
            //Si el SO no nos da mas memoria no se puede hacer el malloc
            iFnImprimir_usr("\nmalloc: Imposible crear Heap");
            return NULL;
        }

        //Se apunta la lista al nuevo bloque libre
        stuListaBloquesLibres.pNodoSig = (void *) pcAntiguoFinDeHeap;
        stuListaBloquesLibres.nTamanio = 0;
       
        //Se crea el encabezado del bloque
        (stuListaBloquesLibres.pNodoSig)->nTamanio = pcFinDeHeap -
            pcAntiguoFinDeHeap; //Cuidado al cambiar: resta de punteros
        (stuListaBloquesLibres.pNodoSig)->pNodoSig = NULL;
       
        iFnImprimir_usr("\nmalloc: Se creo un Heap de %d bytes.",
                pcFinDeHeap - pcAntiguoFinDeHeap);

        //Se deja inicializado
        pcAntiguoFinDeHeap = pcFinDeHeap;
 
        stcExisteHeap = 1; //Ya creamos el Heap, esto no se ejecuta mas
    }

    //Se busca un bloque libre
    /* La funcion pvFnBuscarNodoAnteriorMemoriaLibre retorna un puntero al
     * nodo de la lista de libres que precede al bloque encontrado o un puntero
     * al ultimo nodo de la lista si no encontro el espacio necesario
     */
    pNodoAnteriorAlBloque =
        (t_nodo*)pvFnBuscarNodoAnteriorMemoriaLibre( uiTamanioDeseado + 
                                                    sizeof(t_nodoOcupado) );

    //Si no se encontro espacio contiguo libre, tenemos un puntero al ultimo
    //nodo de la lista
    if (pNodoAnteriorAlBloque->pNodoSig == NULL) {
        iFnImprimir_usr("\nmalloc: No se encontraron %d bytes libres en el "
                "heap, intentando redimensionar el segmento.",uiTamanioDeseado);

        //Se intenta agrandar el segmento
        if( iFnAgrandarHeap( HEAP_INCREMENTO,
                              &pcAntiguoFinDeHeap, &pcFinDeHeap) != 0 ) {
            //Si el SO no nos da mas memoria no se puede hacer el malloc
            iFnImprimir_usr("\nmalloc: Imposible agrandar Heap");
            return NULL;
        }
        
        //Si el ultimo nodo libre llegaba hasta el final del segmento, se le
        //acopla el nuevo espacio libre, si no, se crea un nuevo nodo
        if( pcAntiguoFinDeHeap == ( (char*)pNodoAnteriorAlBloque) + 
                                        pNodoAnteriorAlBloque->nTamanio ) {
            iFnImprimir_usr("\nmalloc: Acoplando espacio libre al ultimo nodo.");
            pNodoAnteriorAlBloque->nTamanio +=
                                    pcFinDeHeap - pcAntiguoFinDeHeap;

            //Se busca el anterior al bloque libre (ahora sabemos que existe)
            pNodoAnteriorAlBloque = (t_nodo*)pvFnBuscarNodoAnteriorMemoriaLibre(
                                    uiTamanioDeseado + sizeof(t_nodoOcupado) );
        } else {
            iFnImprimir_usr("\nmalloc: Creando nuevo bloque de espacio libre.");
            //Se lo agrega a la lista de libres (SIEMPRE sera el ultimo nodo)
            pNodoAnteriorAlBloque->pNodoSig = (t_nodo *) pcAntiguoFinDeHeap;
            pNodoAnteriorAlBloque->pNodoSig->pNodoSig = NULL;
            pNodoAnteriorAlBloque->pNodoSig->nTamanio =
                                    pcFinDeHeap - pcAntiguoFinDeHeap;
        }

        pcAntiguoFinDeHeap = pcFinDeHeap;
    }

    //Existe espacio libre suficiente y es contiguo

    //Se crea el bloque de memoria, actualizando la lista de bloques libres
    pDirBloque = pNodoAnteriorAlBloque->pNodoSig;
    pNodoAnteriorAlBloque->pNodoSig = pDirBloque->pNodoSig;

    //Si el nuevo bloque reservado no entra 'justo' en el bloque libre, se debe
    //crear un nuevo bloque libre. pvFnBuscarNodoAnteriorMemoriaLibre ya se
    //encargo de darnos un bloque en el que el nuevo bloque cabe justo o permite
    //albergar un nuevo nodo.
    if( pDirBloque->nTamanio > uiTamanioDeseado + sizeof(t_nodoOcupado) ) {
        //Se calcula la direccion del nuevo bloque libre
        pNuevoBloqueLibre = (t_nodo *)
            ((char*)pDirBloque + uiTamanioDeseado + sizeof(t_nodoOcupado));
        //Se calcula su tamanio
        pNuevoBloqueLibre->nTamanio =
            pDirBloque->nTamanio - uiTamanioDeseado - sizeof(t_nodoOcupado);
        //Se lo agrega a la lista de libres
        vFnInsertarBloqueLibreEnListaOrd( pNuevoBloqueLibre );
    }
    
    //Se graba el tamanio del bloque que se acaba de crear
    ((t_nodoOcupado *)pDirBloque)->nTamanio = uiTamanioDeseado;

    iFnImprimir_usr("\nmalloc: Se reservaron %d bytes (+%d de control).",
            uiTamanioDeseado, sizeof(t_nodoOcupado) );

    return (void *) ((char*)pDirBloque + sizeof(t_nodoOcupado));
}


/**
 * @brief Libera la memoria anteriormente reservada con malloc
 * @param Direccion de inicio del bloque
 */
void free( void * pInicioBloque ) {
    t_nodo* pNuevoLibre;
    t_nodo* pAux;
    t_nodo* pAModificar;

    unsigned int uiTamanioBloque;

    //TODO - Podria guardarse en una variable global la direccion de inicio del
    //Heap (cuando malloc lo crea) para evitar que se haga free de una direccion
    //por debajo del Heap.
    if( pInicioBloque == NULL ) {
        iFnImprimir_usr("\nfree: Quiso liberar NULL!");
        return;
    }

    //Creo un nuevo nodo para la lista de bloques libres. Todavia no lo enalzo.
    pNuevoLibre = (t_nodo*) ( (char*) pInicioBloque - sizeof(t_nodoOcupado) );
    /* nTamanio es el primer campo en ambas estructuras (t_nodo y t_nodoOcupado)
     * por lo que ya se tiene el tamanio del bloque cargado en la estructura,
     * pero nTamanio de t_nodoOPcupado NO incluye la informacion de control y
     * nTamanio de t_nodo SI la incluye, por eso debemos sumarsela.
     */
    pNuevoLibre->nTamanio += sizeof(t_nodoOcupado);

    //TODO - Sacar:
    //Se guarda el tamanio de memoria que quedara libre (solo para mostrar)
    uiTamanioBloque =
        ((t_nodoOcupado *)pNuevoLibre)->nTamanio - sizeof(t_nodoOcupado);

    /* Recorro la lista de bloques vacios buscando un bloque libre ANTERIOR que
     * sea ADYACENTE al bloque a liberar. Guardo la direccion del nodo que lo
     * apunta (y no la del nodo en si mismo) por si debe moverse.
     */
    pAux = &stuListaBloquesLibres;
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

    /* Recorro la lista de bloques vacios buscando un bloque libre POSTERIOR
     * que sea ADYACENTE al bloque a liberar. Guardo la direccion del nodo que
     * los apunta (y no la del nodo en si mismo) por si debe moverse.
     */
    pAux = &stuListaBloquesLibres;
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

    iFnImprimir_usr("\nfree: Se liberaron %d bytes (+%d de control).",
            uiTamanioBloque, sizeof(t_nodoOcupado) );
}


/**
 * @brief Inserta un nodo en la lista de bloques libres, ordenado por tamanio
 * @param Puntero al nodo a insertar
 */
void vFnInsertarBloqueLibreEnListaOrd( t_nodo * pNuevoNodo ) {
    t_nodo* pAux;

    //Busco la posicion de la lista en la cual insertar el nuevo nodo
    pAux = &stuListaBloquesLibres;
    while( pAux->pNodoSig != NULL &&
           pAux->pNodoSig->nTamanio < pNuevoNodo->nTamanio) {
                pAux = pAux->pNodoSig;
    }

    //Inserto en la lista
    pNuevoNodo->pNodoSig = pAux->pNodoSig;
    pAux->pNodoSig = pNuevoNodo;
}


/**
 * @brief Busca un bloque de memoria libre de uiTamanio bytes
 * @param Tamanio en bytes del bloque libre buscado
 * @returns La direccion del nodo de la lista de bloques libres QUE APUNTA al
 * bloque libre encontrado (nodo anterior) o la direccion del untlimo nodo de la
 * lista si no se encontraron bloques (se debe castear a t_nodo*)
 * @date 28-09-2008
 */
void * pvFnBuscarNodoAnteriorMemoriaLibre(unsigned int uiTamanioDeseado ) {
    t_nodo* pBloqueLibre;

    /* Se busca un bloque que pueda albergar el bloque
     * Si el nuevo bloque no entra 'justo' (justo = no sobra ni un byte), se
     * verifica que no solo entre el uiTamanioDeseado, sino tambien el nodo que
     * se tendra que insertar al 'partir' el bloque
     */
    pBloqueLibre = &stuListaBloquesLibres;
    while( pBloqueLibre->pNodoSig != NULL &&
          pBloqueLibre->pNodoSig->nTamanio != uiTamanioDeseado &&
          pBloqueLibre->pNodoSig->nTamanio < uiTamanioDeseado+sizeof(t_nodo) ) {
                pBloqueLibre = pBloqueLibre->pNodoSig;
    }
/* 
    if( pBloqueLibre->pNodoSig != NULL ) {
            //Se encontro un bloque que sirve para crear el nuevo bloque
            iFnImprimir_usr("\npvBuscar...: Se encontro un bloque de memoria "
                    "libre de %d bytes", pBloqueLibre->pNodoSig->nTamanio);
    } else {
            //NO se encontro un bloque
            iFnImprimir_usr("\npvBuscar...: NO se encontro un bloque de memoria"
                    " libre de tamano suficiente");
    }
*/
    return (void *) pBloqueLibre;
}


/**
 * @brief Agranda el Heap, agrandando el segmento de datos
 * @param Tamanio a adicionar al Heap
 * @param Puntero a puntero para devolver el Fin de Heap Anterior
 * @param Puntero a puntero para devolver el Fin de Heap Nuevo
 * @return 0 si resulto con exito, distinto de 0 si hubo error
 * @date 28-09-2008
 */
int iFnAgrandarHeap( int iTamanio, char** ppcFinHeapAnterior,
                                   char** ppcFinHeapNuevo ) {
    char * pcAux;
    
    //TODO - Reenplazar cuando exista sbrk()
//  pcAux = (char*) sbrk( iTamanio );
    pcAux = (char*) (16 * 1024);        //16K

    if( (int)pcAux == (-1) ) {
        //El SO no nos da mas memoria
        iFnImprimir_usr("iFnAgrandarHeap: El S.O. NO permitio agrandar el "
                "segmento.");
        return -1;
    }
    *ppcFinHeapAnterior = pcAux;

    //TODO - Reenplazar cuando exista sbrk()
//  *ppcFinHeapNuevo = (char*) sbrk( 0 );   //Suponemos que un sbrk(0) no falla
    *ppcFinHeapNuevo = (char*) ( 32 * 1024 );   //32K 

    return 0;
}


/**
 * @brief Muestra los bloques libres en el Heap
 * @date 28-09-2008
 */
void vFnMostrarMemLibreHeap()
{
    t_nodo *pNodoActual;
    int nIndice = 1;

    iFnImprimir_usr("\n# Bloques de memoria libres en el Heap");
    pNodoActual = &stuListaBloquesLibres;
    while ((pNodoActual = pNodoActual->pNodoSig) != NULL) {
        iFnImprimir_usr("\n#\tBloque %2d\tdir inic: %d\ttam = %6d bytes",
                nIndice, (int)pNodoActual, pNodoActual->nTamanio);
        nIndice++;
    }
    iFnImprimir_usr("\n\n");
}

