#include "memoria_dinamica.h"

#include <stdio.h>
#include <stdlib.h>


int main() {
    void *p1, *p2, *p3, *p4, *p5;

    
    vFnInicializarHeap();


    vFnMostrarMemLibreHeap();

    printf("\nReserva en %d\n", (unsigned int) (p1 = pvFnKMalloc(500)));
    vFnMostrarMemLibreHeap();

    printf("\nReserva en %d\n", (unsigned int) (p2 = pvFnKMalloc(1500)));
    vFnMostrarMemLibreHeap();

    //Con este pvFnKMalloc y los anteriores, deberiamos llenar 4000 bytes
    printf("\nReserva en %d\n", (unsigned int) (p3 = pvFnKMalloc(1988)));
    vFnMostrarMemLibreHeap();

    printf("\nReserva en %d\n", (unsigned int) (p4 = pvFnKMalloc(700)));
    vFnMostrarMemLibreHeap();

    vFnKFree(p1);
    printf("\nLiberando 500\n");
    vFnMostrarMemLibreHeap();

    vFnKFree(p3);
    printf("\nLiberando 1976\n");
    vFnMostrarMemLibreHeap();

    printf("\nReserva en %d\n", (unsigned int) (p5 = pvFnKMalloc(3500)));
    vFnMostrarMemLibreHeap();

    //Aca dejamos garbage (la anterior variable dinamica de 3500 inaccesible)
    printf("\nReserva en %d\n", (unsigned int) (p5 = pvFnKMalloc(490)));
    vFnMostrarMemLibreHeap();

    vFnKFree(p4);
    printf("\nLiberando 700\n");
    vFnMostrarMemLibreHeap();

    vFnKFree(p5);
    printf("\nLiberando 490\n");
    vFnMostrarMemLibreHeap();

    vFnKFree(p2);
    printf("\nLiberando 1500\n");
    vFnMostrarMemLibreHeap();

    
    printf("\n");
    return 1;
}
