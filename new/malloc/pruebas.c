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
    printf("\nReserva en %d\n", (unsigned int) (p3 = malloc(1976)));
    vFnMostrarMemLibreHeap();

    printf("\nReserva en %d\n", (unsigned int) (p4 = malloc(700)));
    vFnMostrarMemLibreHeap();

    free(p1);
    printf("\nLiberando 500\n");
    vFnMostrarMemLibreHeap();

    free(p3);
    printf("\nLiberando 1976\n");
    vFnMostrarMemLibreHeap();

    printf("\nReserva en %d\n", (unsigned int) (p5 = malloc(3500)));
    vFnMostrarMemLibreHeap();

    //Aca dejamos garbage (la anterior variable dinamica de 3500 inaccesible)
    printf("\nReserva en %d\n", (unsigned int) (p5 = malloc(500)));
    vFnMostrarMemLibreHeap();

    free(p4);
    printf("\nLiberando 700\n");
    vFnMostrarMemLibreHeap();

    free(p5);
    printf("\nLiberando 500\n");
    vFnMostrarMemLibreHeap();

    free(p2);
    printf("\nLiberando 1500\n");
    vFnMostrarMemLibreHeap();

    
    printf("\n");
    return 1;
}
