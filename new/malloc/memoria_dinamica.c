#include "memoria_dinamica.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

#define HEAP_TAMANIO_INICIAL 3000
#define HEAP_INCREMENTO 3000

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
 * @date 02/10/2008
 */
void * malloc (unsigned int uiTamanioDeseado) {
    t_nodo * pDirBloque;
    t_nodo * pNodoAnteriorAlBloque;
    t_nodo * pNodoMasAlto;
    t_nodo * pNodoAnteriorAlMasAlto;
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
        printf("\nmalloc: Si no quiere memoria, no moleste!");
        return NULL;
    }

    //Esta parte SOLO SE EJECUTA LA PRIMERA VEZ que se llama a malloc.
    //Se encarga de crear el Heap y de inicializar la lista de espacios libres.
    if( !stcExisteHeap ) {

        stuListaBloquesLibres.pNodoSig = NULL;

        //Se crea el Heap, agrandando el segmento
        /* OPTIMIZACION: Ya que sabemos de que tamanio es el primer malloc, lo
         * tenemos en cuenta para que no haga falta redimensionar el heap dos
         * veces
         */
        if( iFnAgrandarHeap( uiTamanioDeseado + HEAP_TAMANIO_INICIAL,
                              &pcAntiguoFinDeHeap, &pcFinDeHeap) != 0 ) {
            //Si el SO no nos da mas memoria no se puede hacer el malloc
            printf("\nmalloc: Imposible crear Heap");
            return NULL;
        }

        //Se apunta la lista al nuevo bloque libre
        stuListaBloquesLibres.pNodoSig = (void *) pcAntiguoFinDeHeap;
        stuListaBloquesLibres.nTamanio = 0;
       
        //Se crea el encabezado del bloque
        (stuListaBloquesLibres.pNodoSig)->nTamanio = pcFinDeHeap -
            pcAntiguoFinDeHeap - sizeof(t_nodo); //Cuidado: resta de punteros
        (stuListaBloquesLibres.pNodoSig)->pNodoSig = NULL;
       
        printf("\nmalloc: Se creo un Heap de %d bytes.",
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
        (t_nodo*)pvFnBuscarNodoAnteriorMemoriaLibre(
                uiTamanioDeseado + sizeof(t_nodoOcupado),
                &pNodoAnteriorAlMasAlto );

    //Si no se encontro espacio contiguo libre, tenemos un puntero al ultimo
    //nodo de la lista
    if (pNodoAnteriorAlBloque->pNodoSig == NULL) {
        printf("\nmalloc: No se encontraron %d bytes libres en el heap, "
                "intentando redimensionar el segmento.", uiTamanioDeseado);

        //TODO - Verificar que HEAP_INCREMENTO sea mayor a uiTamanioDeseado + la
        //estructura de control, si no, puede que redimensionemos el heap y siga
        //sin entrar
        //SOLUCION SIMPLE: Agregamos HEAP_INCREMENTO + uiTamanioDeseado

        //Se intenta agrandar el segmento
        if( iFnAgrandarHeap( uiTamanioDeseado + HEAP_INCREMENTO,
                              &pcAntiguoFinDeHeap, &pcFinDeHeap) != 0 ) {
            //Si el SO no nos da mas memoria no se puede hacer el malloc
            printf("\nmalloc: Imposible agrandar Heap");
            return NULL;
        }

        pNodoMasAlto = pNodoAnteriorAlMasAlto->pNodoSig;
        
        //Si el nodo libre mas alto llegaba hasta el final del segmento, se le
        //acopla el nuevo espacio libre, si no, se crea un nuevo nodo
        if( pcAntiguoFinDeHeap == ( (char*)pNodoMasAlto) +
                                    pNodoMasAlto->nTamanio +
                                    sizeof(t_nodo) ) {
            printf("\nmalloc: Acoplando espacio libre al ultimo nodo.");

            pNodoMasAlto->nTamanio += pcFinDeHeap - pcAntiguoFinDeHeap;
            pNodoAnteriorAlMasAlto->pNodoSig = pNodoMasAlto->pNodoSig;

            vFnInsertarBloqueLibreEnListaOrd( pNodoMasAlto );

            //Se busca el anterior al bloque libre (ahora sabemos que existe)
            pNodoAnteriorAlBloque = (t_nodo*)pvFnBuscarNodoAnteriorMemoriaLibre(
                                    uiTamanioDeseado + sizeof(t_nodoOcupado),
                                    NULL );
        } else {
            printf("\nmalloc: Creando nuevo bloque de espacio libre.");
            //Se lo agrega a la lista de libres (SIEMPRE sera el ultimo nodo)
            pNodoAnteriorAlBloque->pNodoSig = (t_nodo *) pcAntiguoFinDeHeap;
            pNodoAnteriorAlBloque->pNodoSig->pNodoSig = NULL;
            pNodoAnteriorAlBloque->pNodoSig->nTamanio =
                            pcFinDeHeap - pcAntiguoFinDeHeap - sizeof(t_nodo);
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

    printf("\nmalloc: Se reservaron %d bytes (+%d ctrl).",
            uiTamanioDeseado, sizeof(t_nodoOcupado) );

    return (void *) ((char*)pDirBloque + sizeof(t_nodoOcupado));
}


/**
\brief Reserva memoria dinamicamente inicializada en 0
\param nTamanio Indica la cantidad de bytes de memoria que deseo reservar
\returns Puntero a void que indica el comienzo del bloque de memoria reservada. En caso de no poder asignar la memoria devuelve NULL
\date 22/10/2008
*/
void *calloc(unsigned int uiTamanio) {
    void * pvRetorno;

    pvRetorno = malloc(uiTamanio);
    if(pvRetorno == NULL) {
        return NULL;
    }

    return (void*)ucpFnMemSetCero( (unsigned char*)pvRetorno, uiTamanio );
}


/**
\brief Modifica un bloque de memoria reservado dinamicamente
\param pBloqueAModificar Puntero al bloque de memoria a modificar
\param uiNuevoTamanio Cantidad de bytes de memoria que deseo reservar
\returns Puntero a void que indica el comienzo del bloque de memoria reservada. En caso de no poder asignar la memoria devuelve NULL
\date 02/10/2008
*/
void *realloc( void *pBloqueAModificar, unsigned int uiNuevoTamanio) {
    void * pNuevoBloque;
    unsigned int uiTamanioOriginal;

    // 1 Caso trivial, pBloqueAModificar == NULL; hacemos malloc
    if( pBloqueAModificar == NULL ) {
        return malloc( uiNuevoTamanio );
    }

    // 2 Caso trivial, uiNuevoTamanio == 0; hacemos free
    if( uiNuevoTamanio == 0) {
        free( pBloqueAModificar );
        return NULL;
    }

    uiTamanioOriginal =
        ( (t_nodoOcupado*)
             ( (char*)pBloqueAModificar - sizeof(t_nodoOcupado) )) -> nTamanio;

    // 3 Caso trivial, uiNuevoTamanio == uiTamanioOriginal; no hacemos nada
    if( uiNuevoTamanio == uiTamanioOriginal) {
        return pBloqueAModificar;
    }

    // OTROS CASOS
    /* Podriamos intentar redimensionar el bloque ocupado in-situ, pero las
     * diferentes posibilidades hacen que sea mas sencillo crear un nuevo bloque
     * y copiar los contenidos. Si bien no es la opcion mas performante (siempre
     * se deben copiar valores y puede que haya que agrandar el heap), tiene la
     * ventaja de ser facil de debuguear.
     * Hacer el redimensionamiento in-situ parece mas logico aun cuando 
     * uiNuevoTamanio es menor al uiTamanioOriginal (se achica el bloque), pero
     * igualmente se elige crear un nuevo bloque ya que asi se puede reducir la
     * fragmentacion interna.
     */

    pNuevoBloque = malloc( uiNuevoTamanio );
    if( pNuevoBloque == NULL ) {
        return NULL;
    }

    //Se copian N datos (N = el menor entre uiTamanioOriginal y uiNuevoTamanio)
    if( uiNuevoTamanio > uiTamanioOriginal ) {
        memcpy( pNuevoBloque, pBloqueAModificar, uiTamanioOriginal );
    } else {
        memcpy( pNuevoBloque, pBloqueAModificar, uiNuevoTamanio );
    }

    free( pBloqueAModificar );

    return pNuevoBloque;
}


/**
 * @brief Libera la memoria anteriormente reservada con malloc
 * @param Direccion de inicio del bloque
 */
void free( void * pInicioBloque ) {
    t_nodo* pNuevoLibre;
    t_nodo* pAux;
    t_nodo* pAModificar;

    //TODO - Podria guardarse en una variable global la direccion de inicio del
    //Heap (cuando malloc lo crea) para evitar que se haga free de una direccion
    //por debajo del Heap.
    if( pInicioBloque == NULL ) {
        printf("\nfree: Quiso liberar NULL!");
        return;
    }

    //Creo un nuevo nodo para la lista de bloques libres. Todavia no lo enlazo.
    pNuevoLibre = (t_nodo*) ( (char*) pInicioBloque - sizeof(t_nodoOcupado) );

    printf("\nfree: Liberando un bloque de %d bytes (+%d ctrl).",
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
    pAux = &stuListaBloquesLibres;
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
    pAux = &stuListaBloquesLibres;
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

    printf("\nfree: Queda un bloque libre de %d bytes (+%d ctrl).",
            pNuevoLibre->nTamanio, sizeof(t_nodo) );
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
 * @param uiTamanioDeseado Tamanio en bytes del bloque libre buscado
 * @param ppvBloqueMasAlto Direccion de retorno del bloque mas lejano al
 * inicio del segmento
 * @returns La direccion del nodo de la lista de bloques libres QUE APUNTA al
 * bloque libre encontrado (nodo anterior) o la direccion del untlimo nodo de la
 * lista si no se encontraron bloques (se debe castear a t_nodo*)
 * @date 22/10/2008
 */
void * pvFnBuscarNodoAnteriorMemoriaLibre(unsigned int uiTamanioDeseado,
        t_nodo ** ppBloqueMasAlto) {
    t_nodo* pBloqueLibre;
    t_nodo* pMasAlto;

    /* Se busca un bloque que pueda albergar el bloque
     * Si el nuevo bloque no entra 'justo' (justo = no sobra ni un byte), se
     * verifica que no solo entre el uiTamanioDeseado, sino tambien el nodo que
     * se tendra que insertar al 'partir' el bloque
     */
    pBloqueLibre = &stuListaBloquesLibres;
    pMasAlto = pBloqueLibre->pNodoSig;

    while( pBloqueLibre->pNodoSig != NULL &&
         (pBloqueLibre->pNodoSig->nTamanio+sizeof(t_nodo))!= uiTamanioDeseado &&
          pBloqueLibre->pNodoSig->nTamanio < uiTamanioDeseado ) {

                //Llevo la cuenta de cual es el bloque mas lejano al inicio
                if( pBloqueLibre->pNodoSig > pMasAlto->pNodoSig ) {
                    pMasAlto = pBloqueLibre;
                }

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

    //Si el usuario desea conocer cual es el bloque mas lejano al inicio, se lo
    //devuelvo
    if(ppBloqueMasAlto != NULL) {
        *ppBloqueMasAlto = pMasAlto;
    }

    return (void *) pBloqueLibre;
}


/**
 * @brief Agranda el Heap, agrandando el segmento de datos
 * @param Tamanio a adicionar al Heap
 * @param Puntero a puntero para devolver el Fin de Heap Anterior
 * @param Puntero a puntero para devolver el Fin de Heap Nuevo
 * @return 0 si resulto con exito, distinto de 0 si hubo error
 * @date 02/10/2008
 */
int iFnAgrandarHeap( int iTamanio, char** ppcFinHeapAnterior,
                                   char** ppcFinHeapNuevo ) {
    char * pcAux;
    
    pcAux = (char*) sbrk( iTamanio );

    if( (int)pcAux == (-1) ) {
        //El SO no nos da mas memoria
        printf("iFnAgrandarHeap: El S.O. NO permitio agrandar el segmento.");
        return -1;
    }
    *ppcFinHeapAnterior = pcAux;

    *ppcFinHeapNuevo = (char*) sbrk( 0 );   //Suponemos que un sbrk(0) no falla

    return 0;
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
    pNodoActual = &stuListaBloquesLibres;
    while ((pNodoActual = pNodoActual->pNodoSig) != NULL) {
        printf("\n#\tBloque %2d\tdir inic: %d\ttam = %6d bytes (+%d ctrl)",
             nIndice, (int)pNodoActual, pNodoActual->nTamanio, sizeof(t_nodo) );
        nIndice++;
    }
    printf("\n\n");
}



// Esta funcion en realidad se usa de libk/string.h

/**
 * @brief Inicializa uiTamanio bytes a 0 desde la posicion ucpDirInicio
 * @param ucpDirInicio Direccion de inicio
 * @param uiTamanio Cantidad de bytes
 * @date 22/10/2008
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
