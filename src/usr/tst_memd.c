#include <usr/sodstdio.h>   //Para iFnImprimir_usr
#include <usr/libsodium.h>  //Para malloc, free

int main(){
    void * pvPuntero1;
    void * pvPuntero2;
    void * pvPuntero3;


    //Probando limites del segmento: suponiendo que nuestro proceso tiene 32Kb
        char * pcPunteroMortal;
    //Cada uno de estos es una violacion de segmento:
        //pcPunteroMortal = (char *) (36 * 1024);         *pcPunteroMortal = 1;
        //pcPunteroMortal = (char *) (36 * 1024 - 1);     *pcPunteroMortal = 1;
        //pcPunteroMortal = (char *) (32 * 1024);         *pcPunteroMortal = 1;
    //Esto NO es violacion de segmento:
        //...pero seguramente ensucia el stack del proceso
        pcPunteroMortal = (char *) (32 * 1024 - 1);     *pcPunteroMortal = 1;


    pvPuntero1 = malloc( 100 );
    pvPuntero2 = malloc( 200 );
    pvPuntero3 = malloc( 300 );

    pvPuntero1 = realloc( pvPuntero1, 450 );

    free( pvPuntero2 );
    free( pvPuntero1 );
    free( pvPuntero3 );
    
    exit( 0 );

    //Para que no de warning
    return 0;
}
