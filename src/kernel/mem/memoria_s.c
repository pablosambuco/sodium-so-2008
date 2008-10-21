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
 * @param Tamanio en bytes del bloque a reservar
 * @returns La direccion del bloque o NULL si no se pudo reservar
 * @date 02-08-2008
 */
void * pvFnReservarSegmento(unsigned int uiTamanioDeseado) {
    return pvFnKMalloc(uiTamanioDeseado, MEM_ALTA | MEM_USUARIO);
}


/**
 * @brief Redimensiona un segmento de memoria (no modifica descriptores, solo
 * quita el espacio de la lista de libres)
 * @param Nuevo tamanio en bytes del bloque
 * @returns La direccion del bloque o NULL si no se pudo reservar
 * @date 18-10-2008
 */
void * pvFnRedimensionarSegmento(void * pvBloque, unsigned int uiTamanioDeseado) {
    return pvFnKRealloc(pvBloque, uiTamanioDeseado, MEM_ALTA | MEM_USUARIO);
}


/**
 * @brief Agrega un segmento de memoria a la lista de bloques libres
 * @param Direccion de inicio del segmento
 * @param Tamanio del segmento (en bytes)
 */
void vFnLiberarSegmento( void * pInicioSegmento ) {
    vFnKFree(pInicioSegmento);
}


/**
 * @brief Muestra por pantalla la lista de bloques de memoria libre
 */
void vFnListarBloquesLibres() {
    vFnListarKMem();
}

