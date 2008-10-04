#include "memoria_dinamica.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


//TODO - Sacar esto
char HEAP [12][1000];


/* TODO
 * Describir la estrategia de Gestion de Memoria Dinamica a nivel usuario
 */


/*
 * \brief Lista de bloques libres en el Heap
 */
t_nodo BloquesLibresMemoriaAlta;



void vFnInicializarHeap() {

    BloquesLibresMemoriaAlta.nTamanio = 0;
    //BloquesLibresMemoriaAlta.pNodoSig = (void *) INICIO_MEMORIA_ALTA;
    BloquesLibresMemoriaAlta.pNodoSig = (void *) HEAP;

    //((t_nodo *) BloquesLibresMemoriaAlta.pNodoSig)->nTamanio = 
    //    uiTamanioMemoriaBios - (unsigned int) BloquesLibresMemoriaAlta.pNodoSig - sizeof(t_nodo);
    ((t_nodo *) BloquesLibresMemoriaAlta.pNodoSig)->nTamanio =
        sizeof(HEAP) - sizeof(t_nodo);
    ((t_nodo *) BloquesLibresMemoriaAlta.pNodoSig)->pNodoSig = NULL; 
}



/**
 * @brief Reserva un bloque de memoria en el heap
 * @param Tamanio en bytes del bloque a reservar
 * @returns La direccion del bloque reservado o NULL si no se pudo reservar
 * @date 02/10/2008
 */
void * pvFnKMalloc (unsigned int uiTamanioDeseado) {
    t_nodo * pDirBloque;
    t_nodo * pNodoAnteriorAlBloque;
    t_nodo * pNuevoBloqueLibre;


    if(uiTamanioDeseado == 0) {
        printf("\npvFnKMalloc: Si no quiere memoria, no moleste!");
        return NULL;
    }

    //Se busca un bloque libre
    /* La funcion pvFnBuscarNodoAnteriorMemoriaLibre retorna un puntero al
     * nodo de la lista de libres que precede al bloque encontrado o un puntero
     * al ultimo nodo de la lista si no encontro el espacio necesario
     */
    pNodoAnteriorAlBloque = (t_nodo*)pvFnBuscarNodoAnteriorMemoriaLibre(
                            uiTamanioDeseado + sizeof(t_nodoOcupado) );

    //Si no se encontro espacio contiguo libre
    if (pNodoAnteriorAlBloque->pNodoSig == NULL) {
        //Lo unico que queda es "defragmentar la memoria"
        printf("\npvFnKMalloc: No se encontraron %d bytes libres en el heap, "
                "intentando redimensionar el segmento.", uiTamanioDeseado);

        //TODO - Defragmentar memoria

        //Se busca el anterior al bloque libre
        pNodoAnteriorAlBloque = (t_nodo*)pvFnBuscarNodoAnteriorMemoriaLibre(
                                uiTamanioDeseado + sizeof(t_nodoOcupado) );

        //Si ahora no existe, no hay forma de reservar memoria
        if (pNodoAnteriorAlBloque->pNodoSig == NULL) {
            return NULL;
        }
    }

    //Existe espacio libre suficiente y es contiguo

    //Se crea el bloque de memoria, actualizando la lista de bloques libres
    pDirBloque = pNodoAnteriorAlBloque->pNodoSig;
    pNodoAnteriorAlBloque->pNodoSig = pDirBloque->pNodoSig;

    //Si el nuevo bloque reservado no entra 'justo' en el bloque libre, se debe
    //crear un nuevo bloque libre. pvFnBuscarNodoAnteriorMemoriaLibre ya se
    //encargo de darnos un bloque en el que el nuevo bloque cabe justo o permite
    //albergar un nuevo nodo.
    if( pDirBloque->nTamanio + sizeof(t_nodo) >
                                    uiTamanioDeseado + sizeof(t_nodoOcupado) ) {
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

    printf("\npvFnKMalloc: Se reservaron %d bytes (+%d ctrl).",
            uiTamanioDeseado, sizeof(t_nodoOcupado) );

    return (void *) ((char*)pDirBloque + sizeof(t_nodoOcupado));
}








/**
 * @brief Libera la memoria anteriormente reservada con pvFnKMalloc
 * @param Direccion de inicio del bloque
 */
void vFnKFree( void * pInicioBloque ) {
    t_nodo* pNuevoLibre;
    t_nodo* pAux;
    t_nodo* pAModificar;

    //TODO - Podria guardarse en una variable global la direccion de inicio del
    //Heap (cuando pvFnKMalloc lo crea) para evitar que se haga vFnKFree de una direccion
    //por debajo del Heap.
    if( pInicioBloque == NULL ) {
        printf("\nvFnKFree: Quiso liberar NULL!");
        return;
    }

    //Creo un nuevo nodo para la lista de bloques libres. Todavia no lo enlazo.
    pNuevoLibre = (t_nodo*) ( (char*) pInicioBloque - sizeof(t_nodoOcupado) );

    printf("\nvFnKFree: Liberando un bloque de %d bytes (+%d ctrl).",
            pNuevoLibre->nTamanio, sizeof(t_nodoOcupado) );

    /* nTamanio es el primer campo en ambas estructuras (t_nodo y t_nodoOcupado)
     * por lo que ya se tiene el tamanio del bloque cargado en la estructura;
     * Solo falta ajustar el tamanio por la diferencia de tamanio de las
     * estructuras de control
     */
    pNuevoLibre->nTamanio += sizeof(t_nodoOcupado) - sizeof(t_nodo);

    /* Recorro la lista de bloques vacios buscando un bloque libre ANTERIOR que
     * sea ADYACENTE al bloque a liberar. Guardo la direccion del nodo que lo
     * apunta (y no la del nodo en si mismo) por si debe moverse.
     */
    pAux = &BloquesLibresMemoriaAlta;
    while( pAux->pNodoSig != NULL &&
           (char *)pNuevoLibre != ((char *)pAux->pNodoSig +
                                    pAux->pNodoSig->nTamanio+sizeof(t_nodo)) ) {
        pAux = pAux->pNodoSig;
    }

    //Si el bloque libre ANTERIOR es adyacente al nuevo, los unifico y quito el
    //nuevo bloque de la lista, dejandola ordenada
    if( pAux->pNodoSig != NULL) {
            pAModificar = pAux->pNodoSig;
            pAux->pNodoSig = pAModificar->pNodoSig;

            pAModificar->nTamanio += pNuevoLibre->nTamanio + sizeof(t_nodo);
            pNuevoLibre = pAModificar;
    }

    /* Recorro la lista de bloques vacios buscando un bloque libre POSTERIOR
     * que sea ADYACENTE al bloque a liberar. Guardo la direccion del nodo que
     * los apunta (y no la del nodo en si mismo) por si debe moverse.
     */
    pAux = &BloquesLibresMemoriaAlta;
    while( pAux->pNodoSig != NULL &&
            (char *)pAux->pNodoSig != ((char *)pNuevoLibre +
                                       pNuevoLibre->nTamanio+sizeof(t_nodo)) ) {
        pAux = pAux->pNodoSig;
    }

    //Si el bloque libre POSTERIOR es adyacente al nuevo, los unifico y quito el
    //nuevo bloque de la lista, dejandola ordenada
    if( pAux->pNodoSig != NULL) {
            pAModificar = pAux->pNodoSig;
            pAux->pNodoSig = pAModificar->pNodoSig;

            pNuevoLibre->nTamanio += pAModificar->nTamanio + sizeof(t_nodo);
            //pNuevoLibre = pNuevoLibre;
    }

    //Inserto el nuevo bloque libre en la lista
    vFnInsertarBloqueLibreEnListaOrd( pNuevoLibre );

    printf("\nvFnKFree: Queda un bloque libre de %d bytes (+%d ctrl).",
            pNuevoLibre->nTamanio, sizeof(t_nodo) );
}


/**
 * @brief Inserta un nodo en la lista de bloques libres, ordenado por tamanio
 * @param Puntero al nodo a insertar
 */
void vFnInsertarBloqueLibreEnListaOrd( t_nodo * pNuevoNodo ) {
    t_nodo* pAux;

    //Busco la posicion de la lista en la cual insertar el nuevo nodo
    pAux = &BloquesLibresMemoriaAlta;
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
 * @date 02/10/2008
 */
void * pvFnBuscarNodoAnteriorMemoriaLibre(unsigned int uiTamanioDeseado ) {
    t_nodo* pBloqueLibre;

    /* Se busca un bloque que pueda albergar el bloque
     * Si el nuevo bloque no entra 'justo' (justo = no sobra ni un byte), se
     * verifica que no solo entre el uiTamanioDeseado, sino tambien el nodo que
     * se tendra que insertar al 'partir' el bloque
     */
    pBloqueLibre = &BloquesLibresMemoriaAlta;
    while( pBloqueLibre->pNodoSig != NULL &&
         (pBloqueLibre->pNodoSig->nTamanio+sizeof(t_nodo))!= uiTamanioDeseado &&
          pBloqueLibre->pNodoSig->nTamanio < uiTamanioDeseado ) {
                pBloqueLibre = pBloqueLibre->pNodoSig;
    }

    if( pBloqueLibre->pNodoSig != NULL ) {
            //Se encontro un bloque que sirve para crear el nuevo bloque
            printf("\npvBuscar...: Se encontro un bloque de memoria libre de %d"
                   " bytes (+%d ctrl)", pBloqueLibre->pNodoSig->nTamanio,
                   sizeof(t_nodo));
    } else {
            //NO se encontro un bloque
            printf("\npvBuscar...: NO se encontro un bloque de memoria libre "
                    "de tamano suficiente");
    }

    return (void *) pBloqueLibre;
}


/**
 * @brief Muestra los bloques libres en el Heap
 * @date 02/10/2008
 */
void vFnMostrarMemLibreHeap()
{
    t_nodo *pNodoActual;
    int nIndice = 1;

    printf("\n# Bloques de memoria libres en el Heap");
    pNodoActual = &BloquesLibresMemoriaAlta;
    while ((pNodoActual = pNodoActual->pNodoSig) != NULL) {
        printf("\n#\tBloque %2d\tdir inic: %d\ttam = %6d bytes (+%d ctrl)",
             nIndice, (int)pNodoActual, pNodoActual->nTamanio, sizeof(t_nodo) );
        nIndice++;
    }
    printf("\n\n");
}

