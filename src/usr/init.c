#include <usr/libsodium.h>

int iFnImprimirNumero( const char *, int  );

int main(){
	int iPid,iStatus,iOpcion;
	iFnImprimirNumero( "\nSoy el padre, mi pid es ", getpid() );


    /*   
        int * tnt;
        tnt = (int *)0xFFFFFFFF;
        write(0, "\n\nBANG!!!\n\n", 0);
        *tnt = 1000;
    */


	iPid = fork();
	if( !iPid ){
    	iFnImprimirNumero( "\nSoy el hijo, mi pid es ", getpid() );
		/* Ejercicio para el lector: Modificar iOpcion en tiempo de ejecucion, mediante algun debugger primero, y hackeando sodium luego, para que se ejecuten las distintas pruebas */
		iOpcion=5;
		switch(iOpcion)
		{
			case 1: execve( "TST_PTRC.BIN", 0, 0 );
				break;
			case 2: execve( "TST_SGNL.BIN", 0, 0 );
				break;
			case 3: execve( "TST_TIME.BIN", 0, 0 );
				break;
			case 4: execve( "TST_IPC.BIN", 0, 0 );
				break;
			case 5: execve( "TST_FLT.BIN", 0, 0 );
				break;
			default:
				iFnImprimirNumero( "\nLa opcion no es valida! ", iStatus );
		}
        write(0, "\nSoy el hijo, y ya termine (SALGO CON 1)\n", 0);
		exit(1);
	}
	waitpid( iPid, &iStatus, 0);
	
	iFnImprimirNumero( "\nMi hijo salio con el estado ", iStatus );
	exit(0);
}


int iFnImprimirNumero( const char *cncpBuffer, int iNumero ){
//	write( 0, "\n ", 0 );
	write( 0, cncpBuffer, 0 );
	systest( iNumero );
}

