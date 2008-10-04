#include <usr/sodstd.h>
#include <usr/libsodium.h>

static void *pvBrkActual = NULL;

/**
 * @brief Cambia el limite del segmento actual
 * @param pvLimite Nuevo limite de segmento
 * @returns 0 con exito, -1 con error 
 */
int brk(void *pvLimite) {
  void *pvBrkNuevo;

  pvBrkNuevo = __brk(pvLimite);
  if (pvBrkNuevo != pvLimite) return -1;
     pvBrkActual = pvBrkNuevo;
  return 0;
}


/**
 * @brief Cambia el limite del segmento actual
 * @param iIncremento Incremento al limite del segmento
 * @returns Direccion de fin anterior del segmento. -1 con error
 */
void * sbrk(int iIncremento) {
  void *pvBrkAnterior,*pvBrkNuevo;

  if (!pvBrkActual) pvBrkActual = __brk(NULL);
    pvBrkNuevo = __brk(pvBrkActual+iIncremento);
  if (pvBrkNuevo != pvBrkActual+iIncremento) return (void *) -1;
    pvBrkAnterior = pvBrkActual;
    pvBrkActual = pvBrkNuevo;
  return pvBrkAnterior;
}
