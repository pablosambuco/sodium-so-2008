#include <usr/libsodium.h>

int iFnImprimirNumero( const char *, int  );

int main(){
	int iPid,iStatus,iOpcion;
	iFnImprimirNumero( "\nPADRE: Soy el padre, mi pid es ", getpid() );

	iPid = fork();
	if( !iPid ){
    	iFnImprimirNumero( "\nHIJO: Soy el hijo, mi pid es ", getpid() );
		/* Ejercicio para el lector: Modificar iOpcion en tiempo de ejecucion, mediante algun debugger primero, y hackeando sodium luego, para que se ejecuten las distintas pruebas */
		iOpcion = 6;
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
			case 6: execve( "TST_MEMD.BIN", 0, 0 );
				break;
			default:
				iFnImprimirNumero( "\nLa opcion no es valida! ", iStatus );
		}
        write(0, "\nHIJO: Soy el hijo, y ya termine (SALGO CON 1)\n", 0);
		exit(1);
	}
	waitpid( iPid, &iStatus, 0);
	
	iFnImprimirNumero( "\nPADRE: Mi hijo salio con el estado ", iStatus );

    exit(0);

    //Para que no de warning
	return 0;
}


int iFnImprimirNumero( const char *cncpBuffer, int iNumero ){
	write( 0, cncpBuffer, 0 );
	systest( iNumero );
	return 0;
}

