#include <usr/libsodium.h>

int iFnImprimirNumero( const char *cncpBuffer, int iNumero );
int iFnImprimir( const char *cncpBuffer);
int main(){
	int hijo;
	int x;
	int r;
	void* data;

	stuRegs stuGetTss;
	stuRegs stuSetTss;

	x = 15151515;
	r = 20;

	if( ( hijo = fork() ) == 0 )
	{
		/****   Proceso Hijo   ****/
		iFnImprimirNumero( "\n--------->ID DEL HIJO: ", getpid() );

// 		if ( !(ptrace(PTRACE_TRACEME, 0, (void *)0, (void *)0)) ) // 0 ---> OK   -1 ---> Error
// 			iFnImprimir( "\nTRACEMECEADO!!!!!");
// 		else
// 			iFnImprimir( "\nERROR EN TRACEME!!!!!");
		
		while( x );
		iFnImprimirNumero( "--------->HIJO: Prueba POKETEXT: Valor de X: ", x );
		
		x = 10;
		while( x ); // vulvo a ponerlo en espera activa
		iFnImprimir( "--------->HIJO: ESTOY VIVO!!!! " );
	}
	else
	{
		/****   Proceso Padre   ****/
		iFnImprimirNumero( "\n->ID DEL PADRE: ", getpid() );
		
		r = 0;  // Para probar PEEKDATA

		//int error = 0; //Prueba
		if ( !(ptrace(PTRACE_ATTACH, hijo, (void *)0, (void *)0)) ) // 0 ---> OK   -1 ---> Error
			iFnImprimirNumero( "->PADRE: Se Atacho el Hijo con ID: ", hijo);
		else
			iFnImprimir( "**PADRE: Error en Attach ");

  		stuSetTss.eax = 10;
  		stuSetTss.ebx = 15;
  		stuSetTss.ecx = 20;
  		stuSetTss.edx = 25;

		/* Prueba SETREGS */
   		if ( !(ptrace( PTRACE_SETREGS, hijo, (void *)0, (void *)&stuSetTss )) )
   			iFnImprimir( "->PADRE: SETREGS de stuSetTss al Hijo");
   		else
   			iFnImprimir( "**PADRE: Error en el SETREGS Hijo");
  
 		/* Prueba GETREGS */
   		if ( !(ptrace( PTRACE_GETREGS, hijo, (void *)0, (void *)&stuGetTss )) )
   			iFnImprimirNumero( "->PADRE: GETREGS del EAX del Hijo: ", (int) stuGetTss.eax );
   		else
   			iFnImprimir( "**PADRE: Error en el GETREGS del EAX del Hijo: ");

		/* Prueba PEEKDATA */
   		if ( !ptrace( PTRACE_PEEKTEXT, hijo, &r, data ) )
   			iFnImprimirNumero( "->PADRE: PEEKTEXT del valor de la variable de R: ", *(int*)data );
   		else
   			iFnImprimir( "**PADRE: Error en el PEEKTEXT de R ");

  		/* Prueba POKETDATA */
   		if ( !ptrace( PTRACE_POKETEXT, hijo, &x, (void*)0 ) )
   			iFnImprimir( "->PADRE: POKETEXT del valor 0 a la variable X");
   		else
   			iFnImprimir( "**PADRE: Error en el POKETEXT de R ");

		/* Prueba PEEKDATA */
   		if ( !ptrace( PTRACE_PEEKTEXT, hijo, &x, data ) )
   			iFnImprimirNumero( "->PADRE: PEEKTEXT_2 del valor de la variable de X: ", *(int*)data );
   		else
   			iFnImprimir( "**PADRE: Error en el PEEKTEXT_2 de X ");
		
		/* Prueba DETACH */
   		if ( !ptrace( PTRACE_DETACH, hijo, (void *)0, (void*)0 ) )
   			iFnImprimir( "->PADRE: DETACH del Hijo");
   		else
   			iFnImprimir( "**PADRE: Error en el DETACH ");

  		/* Prueba DETACH-PEKETDATA 
		** Esta prueba deberìa salir por ERROR, ya que hicimos un DETACH
		*/
   		if ( !ptrace( PTRACE_POKETEXT, hijo, &x, (void*)0 ) )
   			iFnImprimir( "->PADRE: POKETEXT_2 del valor 0 a la variable X");
   		else
   			iFnImprimir( "**PADRE: Error en el POKETEXT_2 de X ");

		if ( !(ptrace(PTRACE_ATTACH, hijo, (void *)0, (void *)0)) ) // 0 ---> OK   -1 ---> Error
			iFnImprimirNumero( "->SE ATACHO_2 EL HIJO CON ID: ", hijo);
		else
			iFnImprimir( "**ERROR EN ATTACH_2: ");

   		if ( !ptrace( PTRACE_KILL, hijo, (void*)0, (void*)0 ) )
   			iFnImprimir( "->PADRE: MATO AL HIJO");
   		else
   			iFnImprimir( "**PADRE: Error en el KILL del Hijo ");

   		if ( !ptrace( PTRACE_POKETEXT, hijo, &x, (void*)0 ) )
   			iFnImprimir( "->PADRE: POKETEXT_3 del valor 0 a la variable X");
   		else
   			iFnImprimir( "**PADRE: Error en el POKETEXT_3 de X ");
		
	}
	
	exit( 0 );
	
	return 0;

}


int iFnImprimirNumero( const char *cncpBuffer, int iNumero ){
	write( 0, cncpBuffer, 0 );
	systest( iNumero );
	write( 0, "\n", 0 );
	return 0;
}

int iFnImprimir( const char *cncpBuffer){
	write( 0, cncpBuffer, 0 );
	write( 0, "\n", 0 );
	return 0;
}
