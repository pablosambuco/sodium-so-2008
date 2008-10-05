#include <usr/sodstd.h>
#include <usr/libsodium.h>

static void *pvBrkActual = NULL;

/**
 * @brief Cambia el limite del segmento actual
 * @param pvLimite Nuevo limite de segmento
 * @returns 0 con exito, -1 con error 
 * @note Basada en la version de glibc, con modificaciones
 */
int brk(void *pvLimite) {
    void *pvBrkNuevo;
  
    pvBrkNuevo = __brk( pvLimite );
    /* Diferencia con glibc: NO ASUMIMOS QUE __brk ESTABLEZCA LA DIRECCION DE
     * CORTE (ulLimite) SOLICITADA
     * Sodium no garantiza que la direccion ulLimite sea identica a la que
     * solicita el usuario con la llamada a __brk. Sodium garantiza que ulLimite
     * sera AL MENOS la direccion solicitada (pudiendo ulLimite ser mayor).
     * ver brk SunOS: http://cc.in2p3.fr/doc/phpman.php/man/sbrk/2
     */
    /*if ( pvBrkNuevo != pvLimite ) {
        return -1;
    }*/
    if ( pvBrkNuevo == NULL ) {
        return -1;
    }
    
    pvBrkActual = pvBrkNuevo;
    return 0;
}


/**
 * @brief Cambia el limite del segmento actual
 * @param iIncremento Incremento al limite del segmento
 * @returns Direccion de fin anterior del segmento. -1 con error
 * @note Basada en la version de glibc, con modificaciones
 */
void * sbrk(int iIncremento) {
    void *pvBrkAnterior, *pvBrkNuevo;
  
    if ( pvBrkActual == NULL ) {
        pvBrkActual = __brk(NULL);
    }

    if ( iIncremento == 0 ) {
        return pvBrkActual;
    }

    pvBrkNuevo = __brk( pvBrkActual + iIncremento );
  
    /*iFnImprimir_usr(" [%d]", (unsigned int)pvBrkActual);
    iFnImprimir_usr(" +%d", (unsigned int)iIncremento);
    iFnImprimir_usr(" (%d)", (unsigned int)pvBrkNuevo);*/

    /* Diferencia con glibc: NO ASUMIMOS QUE __brk ESTABLEZCA LA DIRECCION DE
     * CORTE (ulLimite) SOLICITADA
     * Sodium no garantiza que la direccion ulLimite sea identica a la que
     * solicita el usuario con la llamada a __brk. Sodium garantiza que ulLimite
     * sera AL MENOS la direccion solicitada (pudiendo ulLimite ser mayor).
     * ver brk SunOS: http://cc.in2p3.fr/doc/phpman.php/man/sbrk/2
     */
    /*if ( pvBrkNuevo != (pvBrkActual + iIncremento) ) {
        return (void *) -1;
    }*/
    if ( pvBrkNuevo == NULL ) {
        return (void *) -1;
    }

    pvBrkAnterior = pvBrkActual;
    pvBrkActual = pvBrkNuevo;

    return pvBrkAnterior;
}
