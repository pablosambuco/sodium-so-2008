#include <video.h>
#include <kernel/definiciones.h>
#include <kernel/mem/memoria_k.h>
#include <kernel/system.h>


extern unsigned int uiTamanioMemoriaBaja;
extern unsigned int uiTamanioMemoriaBios;


/***************************************************************************
 Funcion: vFnIniciarKMem
 Descripcion: Esta funcion inicializa la tabla de bloques libres del kernel
 Parametros: Ninguno
 Valor devuelto: Ninguno
 Ultima modificacion: 09/12/2004
***************************************************************************/
void vFnIniciarKMem()
{
//TODO - Actualizar el footprint para que refleje los cambios hechos, a saber:
// - El Limite para el HEAP del Kernel
// - Los segmentos libres para procesos nuevos
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
    //CHAU, YA NO, Reservamos TAMANIO_HEAP_KERNEL para Heap del Kernel
    //((t_nodo *) InicioMemoriaAlta.pNodoSig)->nTamanio = TAMANIO_HEAP_KERNEL;
    ((t_nodo *) InicioMemoriaAlta.pNodoSig)->nTamanio = 
	    uiTamanioMemoriaBios - (unsigned int) InicioMemoriaAlta.pNodoSig;
	((t_nodo *) InicioMemoriaAlta.pNodoSig)->pNodoSig = NULL;

}


/*****************************************************************************
 Funcion: pvFnKMalloc
 Descripcion:    Reserva memoria dinamicamente.
 Parametros:     dword nTamanio, indica la cantidad de bytes de memoria que
 				 deseo reservar
 Valor devuelto: Puntero a void que indica el comienzo del bloque de memoria
 		 reservada. En caso de no poder asignar la memoria devuelve NULL
 Ultima Modificacion: 09/12/2004
*****************************************************************************/
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

    //TODO - Poner este comentario donde corresponda
    /* El tipo de memoria KERNEL/USUARIO se usa para saber si el bloque de
     * memoria se puede reubicar
     */

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

			//Indico el tamanio del bloque asignado
			pNodoOcupado->nTamanio = nTamanio + sizeof(t_nodoOcupado);
			pNodoOcupado->uiOpciones = GET_MEM_KERNEL_USUARIO(uiOpciones);
			
			vFnLog("\npvFnKMalloc: Asignando %dKb en el Heap del Kernel",
                    pNodoOcupado->nTamanio >> 10);
			return ((void *) ((void *) pNodoOcupado + sizeof(t_nodoOcupado)));
		}
		//Si no hay espacio en el bloque actual avanzo al siguiente
		pUltimoNodo = pNodoActual;
	}	
    vFnLog("\npvFnKMalloc: No se pudo asignar memoria en el Heap del Kernel");
	return (NULL);
}


/*******************************************************************************
 Funcion: pvFnKFree
 Descripcion: Esta funcion libera el espacio de memoria reservado previamente
              por la funcion Malloc.
 Parametros:  void *pNodoOcupado, puntero a la direccin de memoria previamente
 	      	  reservada por la funcion Malloc.
 Valor devuelto: Puntero a null
 Ultima modificacion: 09/12/2004
*******************************************************************************/
void* pvFnKFree(void *pNodoALiberar) {
    t_nodo *pNodoActual, *pUltimoNodo;
    
    //Situo el puntero al verdadero comienzo del bloque a liberar
    pNodoALiberar = ((void *) pNodoALiberar) - sizeof(t_nodoOcupado);

    //En función de la dirección de memoria del bloque a liberar, decido que
    //lista usar. Puede ser la de memoria baja o la de memoria alta.

    //TODO - Reemplazar constante por define
    if ((dword) pNodoALiberar < 0x100000) {
        pUltimoNodo = &InicioMemoriaKernel;
    } else {
        pUltimoNodo = &InicioMemoriaAlta;
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
    return (void*) NULL;
}


/***************************************************************************
 Funcion: MostrarLista
 Descripcion: Recorre la lista dinamica de bloques libres listandolos en
 	      pantalla.
 Parametros: Ninguno
 Valor devuelto: Ninguno
 Ultima modificacion: 09/12/2004
***************************************************************************/
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
