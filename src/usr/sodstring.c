#include <usr/sodstring.h>

/* ATENCION: Este archivo esta basado en kernel/libk/string.c
 * Cualquier modificacion que se haga en este archivo debe ser tenida en cuenta
 * para ser incluida en el archivo original mencionado y viceversa.
 *
 * Puede que muchas de las funciones y partes del codigo de hallen comentadas.
 * Esto no quiere decir que no funcionen o sean inapropiadas, es solo que al
 * momento de generar este archivo no se tuvo el tiempo necesario para probarlas
 */


/**
 * @brief Copia 'size' bytes de 'orig' a 'dest'
 */
inline unsigned char* memcpy(
        unsigned char *ucpDestino,
        unsigned char *ucpOrigen,
        unsigned int uiTamanio )
{
    /* Esta version en assembler es de 3 a 12 veces mas rapida (depende de la
     * cantidad de bytes a copiar) que la version anterior hecha en C.
     */
    int d0, d1, d2;

    __asm__ __volatile__(
        "rep ; movsl\n\t"
        "testb $2,%b4\n\t"
        "je 1f\n\t"
        "movsw\n"
        "1:\ttestb $1,%b4\n\t"
        "je 2f\n\t"
        "movsb\n"
        "2:"
        : "=&c" (d0), "=&D" (d1), "=&S" (d2)
        :"0" (uiTamanio/4), "q" (uiTamanio),
            "1" ((long) ucpDestino),"2" ((long) ucpOrigen)
        : "memory");

    return ucpDestino;
}


/**
 * @brief Inicializa a 0 uiTamanio bytes a partir de ucpDirInicio
 * @param ucpDirInicio Direccion de comienzo
 * @param uiTamanio Cantidad de bytes a inicializar
 */
inline unsigned char* memcero(
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
