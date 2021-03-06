#include <video.h>
#include <kernel/definiciones.h>
#include <kernel/mem/memoria_k.h>
#include <kernel/system.h>


/*
 * Estrategia de Gestion de Memoria Dinamica a nivel kernel:
 *
 * - Se mantienen dos listas de bloques libres, una para memoria convencional y
 *   otra para memoria alta. Ambas estan ordenadas por direccion inicial de
 *   bloque ascendente. Los nodos iniciales son las variables
 *   InicioMemoriaKernel e InicioMemoriaAlta
 *
 * - Al realizar malloc se busca el primer bloque que pueda alojar la memoria
 *   requerida + la informacion de control asociada; por lo tanto la estrategia
 *   es FIRST-FIT.
 *   kmalloc recibe como parametro un entero que posee varias banderas e indica
 *   (hoy en dia):
 *      - En que lista reservar la memoria solicitada.
 *      - El uso que se le dara a la memoria solicitada. Basicamente, kernel o
 *        usuario (los bloques para procesos usuario son reubicables y un
 *        algoritmo de defragmentacion podria moverlos si reapunta la PCB y los
 *        descriptores de la GDT de dichos procesos).
 *
 * - Al realizar kfree, se libera un bloque de memoria dinamica de la siguiente
 *   manera:
 *      (1) si el bloque a liberar NO tiene bloques libres adyacentes, se crea
 *          un nuevo bloque, y se inserta en la lista ordenada de bloques libres
 *      (2) si el bloque a liberar tiene bloques libres adyacentes, se acoplan
 *          dichos bloques al bloque nuevo formando un unico bloque
 *
 * - Al realizar krealloc se hace:
 *      (1) si la lista a la que se desea hacer realloc es diferente a la
 *          original (mover de memoria convencional a alta o viceversa), se
 *          realiza un kmalloc y (si fue exitos) un kfree.
 *      (2) si la lista a la que se desea hacer realloc es la misma que la
 *          original: 
 *              (2A) si el nuevo tamanio es mayor al original, se realiza un
 *                   kmalloc (con las nuevas opciones) y (si fue exitoso) un
 *                   kfree.
 *              (2B) si el nuevo tamanio es menor al original, el bloque se deja
 *                   intacto. TODO: No se contemplan cambios en el campo de
 *                   opciones del bloque.
 *
 * - Al realizar kcalloc se hace un kmalloc y luego se unicializa en 0 el bloque
 *
 */



extern unsigned int uiTamanioMemoriaBaja;
extern unsigned int uiTamanioMemoriaBios;
extern unsigned int uiMemoriaDisponibleProcesos;

/**
\brief Inicializa la tabla de bloques libres del kernel
\date 09/12/2004
*/
void vFnIniciarKMem()
{
/* 
 * El footprint del sodium en memoria es similar al siguiente:
 * USO DE MEMORIA DESDE 0x0:
 *   CODIGO_KERNEL (main.bin... aprox 60kb)
 *   BSS_KERNEL (variable... aprox 128kb)
 *   GDT (64kb)
 *   IDT (1kb)
 *   --- libre --- (variable ~200kb aprox.)
 *   STACK_KERNEL (Variable. Usado por el kernel hasta que se lanza el planificador, luego queda fuera de uso)
 *   0xA0000 (640kb) Es el fin de la memoria convencional, a partir de aquí comienzan los framebuffers de la placa de video y rutinas de la bios... NO USAR!!! ADVERTENCIA: NO suponer que son 640kb justos. Usar uiTamanioMemoriaBaja
 *   0x100000 (1MB) A Partir de aquí podemos disponer nuevamente hasta el memory hole (16M), pero no está presente en todas las PCs... En algunas se puede activar o desactivar a voluntad desde el BIOS
 *   --- libre ---
 *   0x1000000 (16MB) (algunos kb)
 *   --- libre hasta fin de memoria (salvo que tengan menos de 16mb de memoria! ---
 */

    /* Inicializamos DOS listas enlazadas:
     * - Una maneja la memoria libre que se encuentra por debajo de los 640kb,
     * - La otra maneja la memoria por sobre 1MB
     */

    InicioMemoriaKernel.nTamanio = 0;
    //Calculamos la dirección inicial del heap (tenemos la posición inicial de
    //la IDT y conocemos su tamaño:
    InicioMemoriaKernel.pNodoSig =
        (void *) ((unsigned int) pstuIDT + sizeof(stuIDT));

    //Calculamos el tamanio de memoria convencional libre reservando 1kb
    //de memoria para stack de kernel.
    ((t_nodo *) InicioMemoriaKernel.pNodoSig)->nTamanio =
        uiTamanioMemoriaBaja - 1024 -
        (unsigned int) InicioMemoriaKernel.pNodoSig;
    ((t_nodo *) InicioMemoriaKernel.pNodoSig)->pNodoSig = NULL;

    InicioMemoriaAlta.nTamanio = 0;
    InicioMemoriaAlta.pNodoSig = (void *) INICIO_MEMORIA_ALTA;
    ((t_nodo *) InicioMemoriaAlta.pNodoSig)->nTamanio = 
	    uiTamanioMemoriaBios - (unsigned int) InicioMemoriaAlta.pNodoSig;
	((t_nodo *) InicioMemoriaAlta.pNodoSig)->pNodoSig = NULL;

    uiMemoriaDisponibleProcesos =
        uiTamanioMemoriaBios - (unsigned int) InicioMemoriaAlta.pNodoSig;

}


/**
\brief Reserva memoria dinamicamente
\param nTamanio Indica la cantidad de bytes de memoria que deseo reservar
\param uiOpciones Flags de control
\returns Puntero a void que indica el comienzo del bloque de memoria reservada. En caso de no poder asignar la memoria devuelve NULL
\date 02/10/2008
*/
void *pvFnKMalloc(dword nTamanio, unsigned int uiOpciones)
{
	t_nodo *pNodoActual, *pUltimoNodo;
	t_nodoOcupado *pNodoOcupado;
	dword nResto;

	/* Se recorre la lista dinamica de bloques libres buscando el primero que
     * tenga el tamano adecuado. Por ende el algoritmo utilizado es First Fit.
	 */

    if( GET_MEM_ALTA_BAJA(uiOpciones) == MEM_ALTA ) {
        // Memoria Alta
        pUltimoNodo = &InicioMemoriaAlta;
    } else {
        // Memoria Baja
        pUltimoNodo = &InicioMemoriaKernel;
    }

	while ((pNodoActual = pUltimoNodo->pNodoSig) != NULL) {
        /* En esta implementacin, los nodos de la lista dinamica de bloques
         * libres se escriben a modo de encabezado al comienzo de cada uno de
         * ellos. De esta manera se entiende que la direccion logica de cada
         * nodo sumado al tamano de dicha estructura apunta directamente al
         * comienzo de la memoria reservada.
		 */

		//Averigua si hay espacio suficiente para lo que pide el usuario + el
        //tamanio de la estructura nodoOcupado, que almacena el tamanio del
        //bloque actual.
		if ( pNodoActual->nTamanio >= nTamanio + sizeof(t_nodoOcupado)) {
            nResto = pNodoActual->nTamanio - (nTamanio + sizeof(t_nodoOcupado));
			pNodoActual->nTamanio = nResto;

			//Situo el puntero al bloque asignado
            //El bloque se reserva al FINAL del bloque libre existente
			pNodoOcupado = (t_nodoOcupado *) ((byte *) pNodoActual + nResto);

			//Indico el tamanio del bloque asignado (incluye encabezado)
			pNodoOcupado->nTamanio = nTamanio + sizeof(t_nodoOcupado);

            // El tipo de memoria KERNEL/USUARIO se usa para saber si el bloque 
            // de memoria se puede reubicar
			//pNodoOcupado->uiOpciones = GET_MEM_KERNEL_USUARIO(uiOpciones);
			pNodoOcupado->uiOpciones = uiOpciones;
			
			vFnLog("\npvFnKMalloc: Asignando %dKb en el Heap del Kernel",
                    pNodoOcupado->nTamanio >> 10);

            if( GET_MEM_ALTA_BAJA(uiOpciones) == MEM_ALTA ) {
                uiMemoriaDisponibleProcesos -= nTamanio + sizeof(t_nodoOcupado);
            }

			return ((void *) ((void *) pNodoOcupado + sizeof(t_nodoOcupado)));
		}
		//Si no hay espacio en el bloque actual avanzo al siguiente
		pUltimoNodo = pNodoActual;
	}	
    vFnLog("\npvFnKMalloc: No se pudo asignar memoria en el Heap del Kernel");
	return (NULL);
}


/**
\brief Reserva memoria dinamicamente inicializada en 0
\param nTamanio Indica la cantidad de bytes de memoria que deseo reservar
\param uiOpciones Flags de control
\returns Puntero a void que indica el comienzo del bloque de memoria reservada. En caso de no poder asignar la memoria devuelve NULL
\date 22/10/2008
*/
void *pvFnKCalloc(unsigned int uiTamanio, unsigned int uiOpciones) {
    void * pvRetorno;

    pvRetorno = pvFnKMalloc(uiTamanio, uiOpciones);
    if(pvRetorno == NULL) {
        return NULL;
    }

    return (void*)ucpFnMemSetCero( (unsigned char*)pvRetorno, uiTamanio );
}


/**
\brief Modifica un bloque de memoria reservado dinamicamente
\param pBloqueAModificar Puntero al bloque de memoria a modificar
\param uiNuevoTamanio Cantidad de bytes de memoria que deseo reservar
\param uiOpciones Flags de control
\returns Puntero a void que indica el comienzo del bloque de memoria reservada. En caso de no poder asignar la memoria devuelve NULL
\date 02/10/2008
*/
void *pvFnKRealloc( void *pBloqueAModificar,
                    unsigned int uiNuevoTamanio,
                    unsigned int uiOpciones) {

    void * pNuevoBloque;
    unsigned int uiTamanioOriginal;

    // 1 Caso trivial, pBloqueAModificar == NULL; hacemos malloc
    if( pBloqueAModificar == NULL ) {
        vFnLog("\npvFnKRealloc: Actuando como malloc");
        return pvFnKMalloc(uiNuevoTamanio, uiOpciones);
    }

    // 2 Caso trivial, uiNuevoTamanio == 0; hacemos free
    if( uiNuevoTamanio == 0) {
        vFnKFree(pBloqueAModificar);
        vFnLog("\npvFnKRealloc: Actuando como free");
        return NULL;
    }

    uiTamanioOriginal =
        ( (t_nodoOcupado*)
          ( (char*)pBloqueAModificar - sizeof(t_nodoOcupado) )) -> nTamanio;

    // 3 Hay que achicar (o dejar igual) el bloque
    // (y NO moverlo de memoria convencional a alta, o viceversa)
    if( GET_MEM_ALTA_BAJA(((t_nodoOcupado*)pBloqueAModificar)->uiOpciones) ==
        GET_MEM_ALTA_BAJA(uiOpciones) ) {

        // Decidimos no hacerlo, para evitar fragmentacion (si linux lo hace...)
        if( uiNuevoTamanio <= uiTamanioOriginal) {
            //TODO: Que pasa si el resto de las opciones del bloque (ademas de
            //memoria alta/baja) NO son iguales ?

            vFnLog("\npvFnKRealloc: Achicando un bloque de %d bytes a %d bytes",
                    uiTamanioOriginal, uiNuevoTamanio);
            return pBloqueAModificar;
        }
    }

    // 4 Hay que agrandar bloque
    // (o moverlo de memoria convencional a alta, o viceversa)
    pNuevoBloque = pvFnKMalloc( uiNuevoTamanio, uiOpciones );
    if( pNuevoBloque == NULL ) {
        //errno = ENOMEM;
        vFnLog("\npvFnKRealloc: ERROR! No se puede crear bloque de destino");
        return NULL;
    }

    //Se copian N datos (N = el menor entre uiTamanioOriginal y uiNuevoTamanio)
    if( uiNuevoTamanio > uiTamanioOriginal ) {
        ucpFnCopiarMemoria(
                (unsigned char *) pNuevoBloque,
                (unsigned char *) pBloqueAModificar,
                uiTamanioOriginal );
    } else {
        ucpFnCopiarMemoria(
                (unsigned char *) pNuevoBloque,
                (unsigned char *) pBloqueAModificar,
                uiNuevoTamanio );
    }

    vFnKFree( pBloqueAModificar );

    vFnLog("\npvFnKRealloc: OK! Agrandado un bloque de %d bytes a %d bytes",
            uiTamanioOriginal-sizeof(t_nodoOcupado), uiNuevoTamanio);
    return pNuevoBloque;
}


/**
\brief Libera el espacio de memoria reservado previamente con pvFnKMalloc
\param pNodoOcupado, puntero a la direccin de memoria previamente reservada con pvFnKMalloc
\date 29/09/2008
*/
void vFnKFree(void *pNodoALiberar) {
    t_nodo *pNodoActual, *pUltimoNodo;
    
    //Situo el puntero al verdadero comienzo del bloque a liberar
    pNodoALiberar = ((void *) pNodoALiberar) - sizeof(t_nodoOcupado);

    //En función de la dirección de memoria del bloque a liberar, decido que
    //lista usar. Puede ser la de memoria baja o la de memoria alta.
    //Tambien podria usar algo como GET_MEM_ALTA_BAJA(pNodoALiberar.uiOpciones)
    if ((dword) pNodoALiberar < INICIO_MEMORIA_ALTA) {
        pUltimoNodo = &InicioMemoriaKernel;
    } else {
        pUltimoNodo = &InicioMemoriaAlta;
        uiMemoriaDisponibleProcesos +=
            ((t_nodo*)pNodoALiberar)->nTamanio + sizeof(t_nodoOcupado);
    }

    //Busco el bloque libre mas cercano por ARRIBA del bloque a liberar
    //Avanzo el puntero mientras no llegue al ultimo bloque libre
   	while ((pNodoActual = pUltimoNodo->pNodoSig) != NULL) {
        //Cuando el puntero pasa delante del bloque a liberar dejo de avanzar
        if (pNodoActual > (t_nodo *) pNodoALiberar) {
            break;
        }
        //Si el puntero esta a detras del bloque a liberar lo avanzo al
        //siguiente bloque libre
        pUltimoNodo = pNodoActual;
    }

    //Chequeo el limite superior del bloque a liberar para saber si se puede
    //unir a un bloque libre y asi formar uno solo con el posterior libre
    if ((((byte *) pNodoALiberar)+((t_nodoOcupado *) pNodoALiberar)->nTamanio ==
        (byte *) pNodoActual) && pNodoActual != NULL) {
            ((t_nodo *) pNodoALiberar)->nTamanio =
                ((t_nodoOcupado *) pNodoALiberar)->nTamanio +
            		    pNodoActual->nTamanio;
            ((t_nodo *) pNodoALiberar)->pNodoSig = pNodoActual->pNodoSig;
    } else {
        //Si no se puede unir a ningun bloque libre lo inserto en la lista
        ((t_nodo *) pNodoALiberar)->pNodoSig = pNodoActual;
    }

    //Chequeo el limite inferior del bloque a liberar para saber si se puede
    //unir a un bloque libre y asi formar uno solo con el anterior libre
    if (((byte *) pUltimoNodo) + pUltimoNodo->nTamanio == pNodoALiberar) {
        pUltimoNodo->nTamanio = pUltimoNodo->nTamanio +
                                ((t_nodo *) pNodoALiberar)->nTamanio;
        pUltimoNodo->pNodoSig = ((t_nodo *) pNodoALiberar)->pNodoSig;
    } else {
        //Si no lo puedo unir a un bloque libre lo termino de insertar a la
        //lista de bloques libres
        pUltimoNodo->pNodoSig = pNodoALiberar;
    }
}


/**
\brief Muestra en pantalla la lista dinamica de bloques libres
\date 09/12/2004
*/
void vFnListarKMem()
{
	t_nodo *pNodoActual;
	byte nIndice = 1;

	vFnImprimir(
            "\nListado de bloques de memoria libres en memoria convencional");
	pNodoActual = &InicioMemoriaKernel;
	while ((pNodoActual = pNodoActual->pNodoSig) != NULL) {
		vFnImprimir
		    ("\n\tEl bloque %d se encuentra en: %x, con un tamanio de %d KB",
		     nIndice, pNodoActual, pNodoActual->nTamanio / 1024);

		nIndice++;
	}

	vFnImprimir
	    ("\nListado de bloques de memoria libres en memoria alta");
	pNodoActual = &InicioMemoriaAlta;
	nIndice = 1;
	while ((pNodoActual = pNodoActual->pNodoSig) != NULL) {
		vFnImprimir
		    ("\n\tEl bloque %d se encuentra en: %x, con un tamanio de %d KB",
		     nIndice, pNodoActual, pNodoActual->nTamanio / 1024);

		nIndice++;
	}
}
