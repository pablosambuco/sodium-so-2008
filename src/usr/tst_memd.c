#include <usr/sodstdio.h>   //Para iFnImprimir_usr
#include <usr/libsodium.h>  //Para malloc, free

int main(){
    void * pvPuntero1;
    void * pvPuntero2;
    void * pvPuntero3;


    //Esto deberia ser una violacion de segmento si nuestro proceso tiene < 4Mb
    char * pcPunteroMortal = (char *) (4 * 1024 * 1024);
    *pcPunteroMortal = 1;


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
