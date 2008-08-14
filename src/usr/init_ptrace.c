#include <usr/libsodium.h>

int iFnImprimirNumero( const char *, int  );
void vFnLimpiarPantalla();
int iFnImprimirNumeroSinEnter( const char *, int  );
void vFnMostrarStructRegistros(struct stuParametros *);
	
int main(){
int iVariable=0;
int	iHijo = -1, iEstadoHijo=0;
struct stuParametros stuParametros;
		
iHijo = fork();

if( !iHijo ){ 
	kill(getpid(),SIGSTOP);
	ptrace (PTRACE_TRACEME,&stuParametros,0,0);
	write(0,"Hijo: El proceso va a ejecutar syscall exit().\n",0);
	//Esto va a llamar a ptrace por la syscall write
	//Despues se llama a ptrace por la syscall exit
}else
{
	int modo=4;
	// 1= Testeo de flujo
	// 2= Testeo de PEEKDATA y POKEDATA
	// 3= Testeo de PEEKTEXT y POKETEXT
	// 4= Testeo de GETREGS y SETREGS

switch(modo)
{
case 1: //Testeo de flujo
	vFnLimpiarPantalla();
	write(0, "Testeo de flujo de la system call ptrace\n----------------------------------------\n",0);
	iFnImprimirNumero("Padre: El proceso hijo con pid: ",iHijo); write(0," esta siendo trazado.\n",0);
	stuParametros.pid=iHijo;
	kill(iHijo,SIGCONT);


	iFnImprimirNumero("Padre: Proceso padre con pid: ",getpid()); write(0," se detiene a esperar al hijo.\n",0); 	
	kill(getpid(),SIGSTOP);// esto vendria a cumplir la funcion del wait null, cuando el hijo llama a una system call se destraba
	write( 0, "Padre: Proceso padre despertado por ptrace\n", 0 );
	write(0,"Padre: Se continua con la ejecucion del proceso hijo.\n",0);
	ptrace (PTRACE_CONT,&stuParametros,0,0);//fuerza al hijo a continuar

	iFnImprimirNumero("Padre: Proceso padre con pid: ",getpid()); write(0," se detiene a esperar al hijo.\n",0); 	
	kill(getpid(),SIGSTOP);// esto vendria a cumplir la funcion del wait null, cuando el hijo llama a una system call se destraba
	write( 0, "Padre: Proceso padre despertado por ptrace\n", 0 );


	write(0,"Padre: Se finaliza el traceo del proceso hijo.\n",0);
	ptrace (PTRACE_DETACH,&stuParametros,0,0);//termina la traza del hijo
	write(0,"Padre: Se continua con la ejecucion del proceso hijo.\n",0);
	ptrace (PTRACE_CONT,&stuParametros,0,0);//fuerza al hijo a continuar
	//kill(getpid(),SIGSTOP); //para que pare todo y podamos ver el estado. Uno se encuentra zombie (el pid hijo) mientras el otro esta detenido 
	write(0, "Padre: Finaliza el proceso padre\n", 0 );


break;
case 2: //Testeo de PEEKDATA y POKEDATA
	vFnLimpiarPantalla();
	write(0, "Testeo de PTRACE_PEEKDATA y PTRACE_POKEDATA\n------------------------------------------\n",0);
	iFnImprimirNumero("Padre: El proceso hijo con pid: ",iHijo); write(0," esta siendo trazado.\n",0);
	stuParametros.pid=iHijo;

	kill(iHijo,SIGCONT);
	iFnImprimirNumero("Padre: Proceso padre con pid: ",getpid()); write(0," se detiene a esperar al hijo.\n",0); 	
	kill(getpid(),SIGSTOP);// esto vendria a cumplir la funcion del wait null, cuando el hijo llama a una system call se destraba

	write( 0, "Padre: Proceso padre despertado por ptrace\n", 0 );
	stuParametros.addr=22;

	ptrace (PTRACE_PEEKDATA,&stuParametros,0,0); iVariable = stuParametros.data;

	iFnImprimirNumeroSinEnter("Padre: Leo posicion 22 con PTRACE_PEEKDATA. Valor: ", iVariable ); write(0,"\n",0);
	stuParametros.data=25;
	iFnImprimirNumeroSinEnter("Padre: Seteo via PTRACE_POKEDATA la posicion 22 con valor: ", stuParametros.data);	write(0, "\n", 0 );
	ptrace (PTRACE_POKEDATA,&stuParametros,0,0);
	//kill(getpid(),SIGSTOP);// esto vendria a cumplir la funcion del wait null, cuando el hijo llama a una system call se destraba
	ptrace (PTRACE_PEEKDATA,&stuParametros,0,0); iVariable = stuParametros.data;
	iFnImprimirNumeroSinEnter("Padre: Obtengo con PTRACE_PEEKDATA el valor de la posicion 22. Valor: ",iVariable); 
	write(0, "\n", 0 );

	write(0,"Padre: Se finaliza el traceo del proceso hijo.\n",0);
	ptrace (PTRACE_DETACH,&stuParametros,0,0);//termina la traza del hijo
	write(0,"Padre: Se continua con la ejecucion del proceso hijo.\n",0);
	ptrace (PTRACE_CONT,&stuParametros,0,0);//fuerza al hijo a continuar

	//kill(getpid(),SIGSTOP); //para que pare todo y podamos ver el estado. Uno se encuentra zombie (el pid hijo) mientras el otro esta detenido 
	write(0, "Padre: Finaliza el proceso padre\n", 0 );

break;
case 3: //Testeo de PEEKTEXT y POKETEXT
	vFnLimpiarPantalla();
	write(0, "Testeo de PTRACE_PEEKTEXT y PTRACE_POKETEXT\n------------------------------------------\n",0);
	iFnImprimirNumero("Padre: El proceso hijo con pid: ",iHijo); write(0," esta siendo trazado.\n",0);
	stuParametros.pid=iHijo;

	kill(iHijo,SIGCONT);
	iFnImprimirNumero("Padre: Proceso padre con pid: ",getpid()); write(0," se detiene a esperar al hijo.\n",0); 	
	kill(getpid(),SIGSTOP);// esto vendria a cumplir la funcion del wait null, cuando el hijo llama a una system call se destraba

	write( 0, "Padre: Proceso padre despertado por ptrace\n", 0 );
	stuParametros.addr=31;

	ptrace (PTRACE_PEEKTEXT,&stuParametros,0,0); iVariable = stuParametros.data;

	iFnImprimirNumeroSinEnter("Padre: Leo posicion 31 con PTRACE_PEEKTEXT. Valor: ", iVariable ); write(0,"\n",0);
	stuParametros.data=1234;
	iFnImprimirNumeroSinEnter("Padre: Seteo via PTRACE_POKETEXT la posicion 31 con valor: ", stuParametros.data);	write(0, "\n", 0 );
	ptrace (PTRACE_POKETEXT,&stuParametros,0,0);
	//kill(getpid(),SIGSTOP);// esto vendria a cumplir la funcion del wait null, cuando el hijo llama a una system call se destraba
	ptrace (PTRACE_PEEKTEXT,&stuParametros,0,0); iVariable = stuParametros.data;
	iFnImprimirNumeroSinEnter("Padre: Obtengo con PTRACE_PEEKTEXT el valor de la posicion 31. Valor: ",iVariable); 
	write(0, "\n", 0 );

	write(0,"Padre: Se finaliza el traceo del proceso hijo.\n",0);
	ptrace (PTRACE_DETACH,&stuParametros,0,0);//termina la traza del hijo
	write(0,"Padre: Se continua con la ejecucion del proceso hijo.\n",0);
	ptrace (PTRACE_CONT,&stuParametros,0,0);//fuerza al hijo a continuar

	//kill(getpid(),SIGSTOP); //para que pare todo y podamos ver el estado. Uno se encuentra zombie (el pid hijo) mientras el otro esta detenido 
	write(0, "Padre: Finaliza el proceso padre\n", 0 );

break;
case 4: //Testeo de GETREGS y SETREGS
	vFnLimpiarPantalla();
	write(0, "Testeo de PTRACE_GETREGS y PTRACE_SETREGS\n----------------------------------------\n",0);
	iFnImprimirNumero("Padre: El proceso hijo con pid: ",iHijo); write(0," esta siendo trazado.\n",0);
	stuParametros.pid=iHijo;

	kill(iHijo,SIGCONT);
	iFnImprimirNumero("Padre: Proceso padre con pid: ",getpid()); write(0," se detiene a esperar al hijo.\n",0); 	
	kill(getpid(),SIGSTOP);// esto vendria a cumplir la funcion del wait null, cuando el hijo llama a una system call se destraba

	write( 0, "Padre: Proceso padre despertado por ptrace\n", 0 );
	
	write( 0, "Obtengo valores de los registros:\n", 0 ); stuParametros.pid=iHijo;
	ptrace (PTRACE_GETREGS,&stuParametros,0,0);
	vFnMostrarStructRegistros(&stuParametros);

	stuParametros.xcs=10;
	stuParametros.xds=20;
	stuParametros.eflags=30;
	write( 0, "Seteo valores de los registros:\n", 0 ); stuParametros.pid=iHijo;
	vFnMostrarStructRegistros(&stuParametros);
	ptrace (PTRACE_SETREGS,&stuParametros,0,0);		

	ptrace (PTRACE_GETREGS,&stuParametros,0,0);
	write( 0, "Obtengo valores de los registros:\n", 0 );
	vFnMostrarStructRegistros(&stuParametros);
	//iFnImprimirNumeroSinEnter("xcs: ",stuParametros.xcs);iFnImprimirNumeroSinEnter("  xds: ",stuParametros.xds);iFnImprimirNumeroSinEnter("  eflags: ",stuParametros.eflags); write(0,"\n",0);

	write(0,"Padre: Se finaliza el traceo del proceso hijo.\n",0);
	ptrace (PTRACE_DETACH,&stuParametros,0,0);//termina la traza del hijo
	write(0,"Padre: Se continua con la ejecucion del proceso hijo.\n",0);
	ptrace (PTRACE_CONT,&stuParametros,0,0);//fuerza al hijo a continuar
	
	//kill(getpid(),SIGSTOP); //para que pare todo y podamos ver el estado. Uno se encuentra zombie (el pid hijo) mientras el otro esta detenido 
	write(0, "Soy el padre me voy a terminar\n", 0 );

break;
}
	
} //End if de if hijo	
	exit( 0 ); //Tanto hijo como padre
}

void vFnLimpiarPantalla()
{	int i=0;
	for (;i<1;i++)
	{
		write(0,"\n",0);
	}
}

void vFnMostrarStructRegistros(struct stuParametros *pstuParametros)
{
//int eax,ebx,ecx,edx,esi,edi,ebp,xds,xes,eip,xcs,eflags,esp;
	write(0," MP:",0);
	iFnImprimirNumeroSinEnter(" xcs: ",pstuParametros->xcs);
	iFnImprimirNumeroSinEnter("  xds: ",pstuParametros->xds);
	iFnImprimirNumeroSinEnter("  eflags: ",pstuParametros->eflags);
	iFnImprimirNumeroSinEnter("  eax: ",pstuParametros->eax);
	iFnImprimirNumeroSinEnter("  ebx: ",pstuParametros->ebx);
	iFnImprimirNumeroSinEnter("  ecx: ",pstuParametros->ecx);
	iFnImprimirNumeroSinEnter("  edx: ",pstuParametros->edx);
	//write(0,"\n    ",0);
	iFnImprimirNumeroSinEnter(" esi: ",pstuParametros->esi);
	iFnImprimirNumeroSinEnter("  edi: ",pstuParametros->edi);	
	iFnImprimirNumeroSinEnter("  ebp: ",pstuParametros->ebp);
	iFnImprimirNumeroSinEnter("  xes: ",pstuParametros->xes);
	iFnImprimirNumeroSinEnter("  eip: ",pstuParametros->eip);
	iFnImprimirNumeroSinEnter("  esp: ",pstuParametros->esp);
	write(0,"\n",0);
}

int iFnImprimirNumero( const char *cncpBuffer, int iNumero ){
	write( 0, "\n ", 0 );
	write( 0, cncpBuffer, 0 );
	systest( iNumero );
}

int iFnImprimirNumeroSinEnter( const char *cncpBuffer, int iNumero ){
	write( 0, cncpBuffer, 0 );
	systest( iNumero );
}
