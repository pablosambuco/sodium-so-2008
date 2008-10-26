#include <usr/libsodium.h>
#include <usr/memoria_dinamica.h>
#include <usr/sodstd.h>
#include <usr/sodstring.h>

//TODO - Sacar usr/sodstdio.h, esta solo para mostrar informacion para debug
#include <usr/sodstdio.h>


/*
 * Estrategia de Gestion de Memoria Dinamica a nivel usuario:
 *
 * - Se mantiene una lista de bloques libres, ordenada por tamano de bloque
 *   ascendente. El nodo inicial es la variable stuListaBloquesLibres.
 *
 * - Al realizar malloc se busca el primer bloque que pueda alojar la memoria
 *   requerida + la informacion de control asociada; por lo tanto la estrategia
 *   es BEST-FIT (First-Fit con lista ordenada).
 *
 * - Se comienza con un Heap de 0 bytes y cada vez que se necesita mas memoria
 *   dinamica se llama a iFnAgrandarHeap
 *
 * - Al realizar free, se libera un bloque de memoria dinamica de la siguiente
 *   manera:
 *     (1) si el bloque a liberar NO tiene bloques libres adyacentes, se crea un
 *         nuevo bloque, y se inserta en la lista ordenada de bloques libres
 *     (2) si el bloque a liberar tiene bloques libres adyacentes, se acoplan
 *         dichos bloques al bloque nuevo formando un unico bloque, se quita de
 *         la lista y se lo reinserta en la lista ordenada de bloques libres
 *
 * - Al realizar realloc se hace un malloc y (si fue exitos) un free.
 *
 * - Al realizar calloc se hace un malloc y luego se unicializa en 0 el bloque.
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
    char * pcAntiguoFinDeHeap; 

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
        /* OPTIMIZACION: Ya que sabemos de que tamanio es el primer malloc, lo
         * tenemos en cuenta para que no haga falta redimensionar el heap dos
         * veces
         */
        if( iFnAgrandarHeap( HEAP_TAMANIO_INICIAL + uiTamanioDeseado,
                              &pcAntiguoFinDeHeap, &pcFinDeHeap) != 0 ) {
            //Si el SO no nos da mas memoria no se puede hacer el malloc
            iFnImprimir_usr("\nmalloc: Imposible crear Heap");
            errno = ENOMEM;
            return NULL;
        }

        //Se apunta la lista al nuevo bloque libre
        stuListaBloquesLibres.pNodoSig = (void *) pcAntiguoFinDeHeap;
        stuListaBloquesLibres.nTamanio = 0;
       
        //Se crea el encabezado del bloque
        (stuListaBloquesLibres.pNodoSig)->nTamanio = pcFinDeHeap -
            pcAntiguoFinDeHeap - sizeof(t_nodo); //Cuidado: resta de punteros
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
        (t_nodo*)pvFnBuscarNodoAnteriorMemoriaLibre(
                uiTamanioDeseado + sizeof(t_nodoOcupado),
                &pNodoAnteriorAlMasAlto );

    //Si no se encontro espacio contiguo libre, tenemos un puntero al ultimo
    //nodo de la lista
    if (pNodoAnteriorAlBloque->pNodoSig == NULL) {
        iFnImprimir_usr("\nmalloc: No se encontraron %d bytes libres en el "
                "heap, intentando redimensionar el segmento.",uiTamanioDeseado);

        //TODO - Verificar que HEAP_INCREMENTO sea mayor a uiTamanioDeseado + la
        //estructura de control, si no, puede que redimensionemos el heap y siga
        //sin entrar.
        //SOLUCION SIMPLE: Agregamos HEAP_INCREMENTO + uiTamanioDeseado

        //Se intenta agrandar el segmento
        if( iFnAgrandarHeap( HEAP_INCREMENTO + uiTamanioDeseado,
                              &pcAntiguoFinDeHeap, &pcFinDeHeap) != 0 ) {
            //Si el SO no nos da mas memoria no se puede hacer el malloc
            iFnImprimir_usr("\nmalloc: Imposible agrandar Heap");
            errno = ENOMEM;
            return NULL;
        }
       
        pNodoMasAlto = pNodoAnteriorAlMasAlto->pNodoSig;
        
        //Si el nodo libre mas alto llegaba hasta el final del segmento, se le
        //acopla el nuevo espacio libre, si no, se crea un nuevo nodo
        if( pcAntiguoFinDeHeap == ( (char*)pNodoMasAlto) +
                                    pNodoMasAlto->nTamanio +
                                    sizeof(t_nodo) ) {
            iFnImprimir_usr("\nmalloc: Acoplando espacio libre al ultimo nodo");

            pNodoMasAlto->nTamanio += pcFinDeHeap - pcAntiguoFinDeHeap;
            pNodoAnteriorAlMasAlto->pNodoSig = pNodoMasAlto->pNodoSig;

            vFnInsertarBloqueLibreEnListaOrd( pNodoMasAlto );

            //Se busca el anterior al bloque libre (ahora sabemos que existe)
            pNodoAnteriorAlBloque = (t_nodo*)pvFnBuscarNodoAnteriorMemoriaLibre(
                                    uiTamanioDeseado + sizeof(t_nodoOcupado),
                                    NULL );
        } else {
            iFnImprimir_usr("\nmalloc: Creando nuevo bloque de espacio libre");
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

    iFnImprimir_usr("\nmalloc: Se reservaron %d bytes (+%d ctrl).",
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

    return (void*)memcero( (unsigned char*)pvRetorno, uiTamanio );
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
        errno = ENOMEM;
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
        iFnImprimir_usr("\nfree: Quiso liberar NULL!");
        return;
    }

    //Creo un nuevo nodo para la lista de bloques libres. Todavia no lo enlazo.
    pNuevoLibre = (t_nodo*) ( (char*) pInicioBloque - sizeof(t_nodoOcupado) );

    iFnImprimir_usr("\nfree: Liberando un bloque de %d bytes (+%d ctrl).",
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

    iFnImprimir_usr("\nfree: Queda un bloque libre de %d bytes (+%d ctrl).",
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
 * @param ppvBloqueMasAlto Direccion de retorno del bloque QUE APUNTA AL bloque
 * mas lejano al inicio del segmento
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
            iFnImprimir_usr("\npvBuscar...: Se encontro un bloque de memoria "
                            "libre de %d  bytes (+%d ctrl)",
                            pBloqueLibre->pNodoSig->nTamanio, sizeof(t_nodo));
    } else {
            //NO se encontro un bloque
            iFnImprimir_usr("\npvBuscar...: NO se encontro un bloque de memoria"
                            " libre de tamano suficiente");
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
        iFnImprimir_usr("iFnAgrandarHeap: El S.O. NO permitio agrandar el "
                "segmento.");
        return -1;
    }
    *ppcFinHeapAnterior = pcAux;

    *ppcFinHeapNuevo = (char*) sbrk( 0 );

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

    iFnImprimir_usr("\n# Bloques de memoria libres en el Heap");
    pNodoActual = &stuListaBloquesLibres;
    while ((pNodoActual = pNodoActual->pNodoSig) != NULL) {
        iFnImprimir_usr("\n#\tBloque %2d\tdir inic: %d\ttam = %6d bytes (+%d "
                "ctrl)", nIndice, (int)pNodoActual, pNodoActual->nTamanio,
                sizeof(t_nodo) );
        nIndice++;
    }
    iFnImprimir_usr("\n\n");
}

