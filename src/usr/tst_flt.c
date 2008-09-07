//#include <usr/libsodium.h>
#include <usr/sodstdio.h>

int main(){
    double fVar1 = 3.145;
    double fVar2 = -1.010;
    double fVar3 = 99.546545;
    double fVar4 = -71.010;
    double fVar5;

    //fVar2 = 0;
	/* Esto deberia generar una excepcion si fVar2 es 0
	 */
    fVar5 = fVar1 / fVar2;

    iFnImprimir_usr("\n*** Prueba FPU lado usuario: %f + %f = %f",
						fVar1, fVar2, fVar1+fVar2);
    iFnImprimir_usr("\n*** Prueba FPU lado usuario: %f + %f = %.8f",
						fVar1, fVar3, fVar1+fVar3);
    iFnImprimir_usr("\n*** Prueba FPU lado usuario: %f - %f = %.2f",
	 					fVar1, fVar3, fVar1-fVar3);
    iFnImprimir_usr("\n*** Prueba FPU lado usuario: %f * %f = %.7f",
    					fVar2, fVar4, fVar2*fVar4);
    iFnImprimir_usr("\n*** Prueba FPU lado usuario: %f * %f = %.10f",
    					fVar3, fVar4, fVar3*fVar4);
    iFnImprimir_usr("\n*** Prueba FPU lado usuario: %f / %f = %.1f",
    					fVar3, fVar4, fVar3/fVar4);

	iFnImprimir_usr("\nSoy el hijo, y ya termine (SALGO CON 42)\n");

    exit( 42 );

    //Para que no de warning
    return 0;
}
