#include <usr/sodstdio.h>   //Para iFnImprimir_usr
#include <usr/libsodium.h>  //Para malloc, free

int main(){
    void * pvPuntero1;
    void * pvPuntero2;
    void * pvPuntero3;

    char * pcBrk;

    /* Probando limites del segmento */

    /* Obtenemos la direccion de BRK de nuestro segmento. Dicha direccion es la
     * siguiente a la ultima de nustro segmento, por lo que si intentamos
     * acceder a una direccion >= BRK obtendremos 'Segmentation Fault'
     */
    pcBrk = (char *) sbrk(0);

    // Genera SEGFAULT
    iFnImprimir_usr("\nIntentando acceder a %d", (int)( pcBrk ) );
    * pcBrk = 1;
    // NO Genera SEGFAULT
    iFnImprimir_usr("\nIntentando acceder a %d", (int)( pcBrk - 1 ) );
    *( pcBrk - 1 ) = 1;

    /* No se recomienda usar brk/sbrk para cambiar los limites del segmento y a
     * la vez usar malloc/free/etc, puede causar resultados imprevistos
     */

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
