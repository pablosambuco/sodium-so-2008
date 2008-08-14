#include <usr/libsodium.h>
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int main(){
	write( 0, "\n hola desde el programa invocado con execve()", 0 );
	write( 0, "\n mi pid es ", 0 );
	systest( getpid() );
	exit(1);
	return(1);
}
