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
    char * pcFinDeSegmento; 
    static char * pcAntiguoFinDeSegmento; 

    static char stcExisteHeap = 0;

    if(uiTamanioDeseado == 0) {
        iFnImprimir_usr("\nmalloc: Si no quiere memoria, no moleste!");
        return NULL;
    }

    //Esta parte SOLO SE EJECUTA LA PRIMERA VEZ que se llama a malloc.
    //Se encarga de crear el Heap y de inicializar la lista de espacios libres.
    if( !stcExisteHeap ) {
        //TODO - Crear el heap
        //TODO - Agrandar el segmento (REFACTORIZAR)
        pcAntiguoFinDeSegmento = (void*)(-1); //TODO - sbrk(...);
       
        stuListaBloquesLibres.pNodoSig = NULL;

        if( (int)pcAntiguoFinDeSegmento == (-1) ) {
            //Si el SO no nos da mas memoria no se puede hacer el malloc
            iFnImprimir_usr("\nmalloc: Imposible crear Heap");
            return NULL;
        }

        pcFinDeSegmento = NULL; //TODO - sbrk( 0 );
        
        //Se apunta la lista al nuevo bloque libre
        stuListaBloquesLibres.pNodoSig = (void *) pcAntiguoFinDeSegmento;
        stuListaBloquesLibres.nTamanio = 0;
       
        //Se crea el encabezado del bloque
        (stuListaBloquesLibres.pNodoSig)->nTamanio = pcFinDeSegmento -
            pcAntiguoFinDeSegmento; //Cuidado al cambiar: resta de punteros
        (stuListaBloquesLibres.pNodoSig)->pNodoSig = NULL;
       
        //Se deja inicializado
        pcAntiguoFinDeSegmento = pcFinDeSegmento;
 
        iFnImprimir_usr("\nmalloc: Se creo un Heap de %d bytes.",
                pcFinDeSegmento - pcAntiguoFinDeSegmento);

        stcExisteHeap = 1; //Ya creamos el Heap, esto no se ejecuta mas
    }

    //Se busca un bloque libre
    //La funcion pvFnBuscarNodoAnteriorMemoriaLibre retorna un puntero al
    //nodo de la lista de libres que precede al bloque encontrado
    pNodoAnteriorAlBloque =
        (t_nodo*)pvFnBuscarNodoAnteriorMemoriaLibre( uiTamanioDeseado + 
                                                    sizeof(t_nodoOcupado) );

    //Si no se encontro espacio contiguo libre
    if (pNodoAnteriorAlBloque == NULL) {
        iFnImprimir_usr("\nmalloc: No se encontraron %d bytes de memoria libre "
                "en el heap, intentando redimensionar el segmento.",
                uiTamanioDeseado);

        //Se intenta agrandar el segmento
        //TODO - Agrandar el segmento (REFACTORIZAR)
        pcFinDeSegmento = NULL; //TODO - sbrk( ... ); puede ser una variable aux
       
        if( (int)pcFinDeSegmento == -1 ) {
            //Si el SO no nos da mas memoria no se puede hacer el malloc
            return NULL;
        }

        pcFinDeSegmento = NULL; //TODO - sbrk( 0 );

        //Se calcula la direccion del nuevo bloque libre
        pNuevoBloqueLibre = (t_nodo *) pcAntiguoFinDeSegmento;
        //Se calcula su tamanio
        pNuevoBloqueLibre->nTamanio = pcFinDeSegmento - pcAntiguoFinDeSegmento;
        //Se lo agrega a la lista de libres
        vFnInsertarBloqueLibreEnListaOrd( pNuevoBloqueLibre );

        pcAntiguoFinDeSegmento = pcFinDeSegmento;
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

    iFnImprimir_usr("\nmalloc: Se reservaron %d bytes.", uiTamanioDeseado);

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
    pNuevoLibre = (t_nodo*) ( pInicioBloque - sizeof(t_nodoOcupado) );
    //nTamanio es el primer campo en ambas estructuras (t_nodo y t_nodoOcupado),
    //por lo que ya se tiene el tamanio del bloque cargado en la estructura.
    //nTamanio NO incluye la informacion de control (t_nodoOcupado)

    //Se calcula el tamanio de memoria que quedara libre
    uiTamanioBloque =
        ((t_nodoOcupado *)pNuevoLibre)->nTamanio + sizeof(t_nodoOcupado);

    /* Recorro la lista de bloques vacios buscando un bloque libre ANTERIOR que
     * sea adyacente al bloque a liberar. Guardo la direccion del nodos que lo
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
     * que sea adyacente al bloque a liberar. Guardo la direccion del nodo que
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

    iFnImprimir_usr("\nfree: Se liberaron %d bytes.", uiTamanioBloque);
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
 * bloque libre encontrado (nodo anterior) o NULL si no se encontraron bloques
 * (se debe castear a t_nodo*)
 * @date 02-08-2008
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

    if( pBloqueLibre->pNodoSig != NULL ) {
            //Se encontro un bloque que sirve para crear el nuevo bloque
            iFnImprimir_usr("\nSe encontro un bloque de memoria libre de %dKb",
                    pBloqueLibre->pNodoSig->nTamanio >> 10);

            return (void *) pBloqueLibre;
    }

    return NULL;
}

