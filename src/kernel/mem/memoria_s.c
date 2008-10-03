#include <kernel/mem/memoria_s.h>
#include <kernel/mem/memoria_k.h>


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
 */
void vFnInicializarMemoriaSegmentada() {
}


/**
 * @brief Reserva un segmento de memoria (no crea descriptores, solo quita el
 * espacio de la lista de libres)
 * @param Tamanio en bytes del bloque libre a reservar
 * @returns La direccion del bloque libre o 0 si no se encontro ninguno
 * @date 08-02-2008
 */
//TODO - SACAR! (?)
void * pvFnReservarSegmento(unsigned int uiTamanioDeseado) {
    return pvFnKMalloc(uiTamanioDeseado, MEM_ALTA | MEM_USUARIO);
}
 

/**
 * @brief Agrega un segmento de memoria a la lista de bloques libres
 * @param Direccion de inicio del segmento
 * @param Tamanio del segmento (en bytes)
 */
//TODO - SACAR! (?)
void vFnLiberarSegmento( void * pInicioSegmento,
                         unsigned int uiTamanioSegmento ) {
    vFnKFree(pInicioSegmento);
}


/**
 * @brief Muestra por pantalla la lista de bloques de memoria libre
 */
void vFnListarBloquesLibres() {
    vFnListarKMem();
}

