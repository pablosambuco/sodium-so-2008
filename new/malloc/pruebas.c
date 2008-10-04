#include "memoria_dinamica.h"

#include <stdio.h>
#include <stdlib.h>

int main() {
    void *p1, *p2, *p3, *p4, *p5;

    vFnMostrarMemLibreHeap();

    printf("\nReserva en %d\n", (unsigned int) (p1 = malloc(500)));
    vFnMostrarMemLibreHeap();

    printf("\nReserva en %d\n", (unsigned int) (p2 = malloc(1500)));
    vFnMostrarMemLibreHeap();

    //Con este malloc y los anteriores, deberiamos llenar 4000 bytes
    printf("\nReserva en %d\n", (unsigned int) (p3 = malloc(1988)));
    vFnMostrarMemLibreHeap();

    printf("\nReserva en %d\n", (unsigned int) (p4 = malloc(700)));
    vFnMostrarMemLibreHeap();

    free(p1);
    printf("\nLiberando 500\n");
    vFnMostrarMemLibreHeap();

    free(p3);
    printf("\nLiberando 1976\n");
    vFnMostrarMemLibreHeap();
/*
    //Si ponemos esto, mas abajo dejamos garbage (la variable dinamica de 3500
    //inaccesible)
    printf("\nReserva en %d\n", (unsigned int) (p5 = malloc(3500)));
    vFnMostrarMemLibreHeap();
*/
    printf("\nReserva en %d\n", (unsigned int) (p5 = malloc(490)));
    vFnMostrarMemLibreHeap();

    free(p4);
    printf("\nLiberando 700\n");
    vFnMostrarMemLibreHeap();

    //Hacemos realloc con el mismo tamanio original
    printf("\nRealloc de %d\n", (unsigned int) p5);
    printf("\nRealloc ahora esta en %d\n",
            (unsigned int) (p5 = realloc(p5, 490)));
    vFnMostrarMemLibreHeap();

    //Hacemos realloc con un tamanio mas chico
    printf("\nRealloc de %d\n", (unsigned int) p5);
    printf("\nRealloc ahora esta en %d\n",
            (unsigned int) (p5 = realloc(p5, 484)));
    vFnMostrarMemLibreHeap();

    free(p5);
    printf("\nLiberando 484\n");
    vFnMostrarMemLibreHeap();

    //Hacemos realloc con un tamanio mas grande
    printf("\nRealloc de %d\n", (unsigned int) p2);
    printf("\nRealloc ahora esta en %d\n",
            (unsigned int) (p2 = realloc(p2, 10000)));
    vFnMostrarMemLibreHeap();

    free(p2);
    printf("\nLiberando 6000\n");
    vFnMostrarMemLibreHeap();

    
    printf("\n");
    return 1;
}
