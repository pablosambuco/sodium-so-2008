#include <usr/libsodium.h>

int iFnImprimirNumero( const char *cncpBuffer, int iNumero );

int main_signal(){
	int	iPid2 = 5,
		iPid3 = 6,
		iPid4 = 7,
		iStatus2 = 0,
		iStatus3 = 0,
		iStatus4 = 0,
		iValor2 = 2,
		iValor3 = 3,
		iValor4 = 4;

	//iPid2 = iPid3 / iStatus2;
	iPid2 = fork();
	iFnImprimirNumero( "mi pid es ", getpid() );
	if( !iPid2 ){
		write( 0, "\n soy el hijo, voy a llamar a execve()", 0 );
		execve( "PROG.BIN", 0, 0 );
		exit(1);
	}
	waitpid( iPid2, &iStatus2, 0 );
	iFnImprimirNumero( "mi hijo salio con el estado ", iStatus2 );

	while(1){}

	if( iPid2 > 0 ){ // somos el padre (p1)
		iPid3 = fork();
		if( iPid3 > 0 ){ // somos el padre (p1)
			//kill( iPid3, SIGKILL ); // como su hijo se hace el vivo lo mata
			waitpid( iPid3, &iStatus3, 0 );
		} else if( iPid3 == 0 ){ // somos p3
			while(1){} // este se hace el vivo y no sale
			exit( iValor3 );
		}
		waitpid( iPid2, &iStatus2, 0);
		iFnImprimirNumero( "init(): resultado final prueba: ", iStatus2 + iStatus3 );
	} else if( iPid2 == 0 ){ // somos el hijo (p2)
		iPid4 = fork();
		if( iPid4 > 0 ){ // somos el padre (p2)
			waitpid( iPid4, &iStatus4, 0 );
			exit( iStatus4 + iValor2 );
		} else if( iPid4 == 0 ){ // somos el hijo (p4)
			exit( iValor4 );
		}
	} else { // error, imprimimos errno
		iFnImprimirNumero( "init(): fork() fallo, error: ", errno );
	}

	while(1){}

	exit( 0 );
}

int iFnImprimirNumero( const char *cncpBuffer, int iNumero ){
	write( 0, "\n ", 0 );
	write( 0, cncpBuffer, 0 );
	systest( iNumero );
	
	return 0;
}
