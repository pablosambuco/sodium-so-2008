#include <usr/libsodium.h>

int iFnImprimirNumero( const char *cncpBuffer, int iNumero );

int main_IPC(){
	int i = 5;
    int iPid,iStatus;
	sem_t semaforo1;
	sem_t semaforo2;
//	unsigned long j;
	int shmid;
	int * shmvar;

	if((sem_init(&semaforo1, 1, 1))<0)
	{
		write( 0, "\n Error al iniciar el semaforo 1", 0 );
		exit(1);
	}

	if((sem_init(&semaforo2, 1, 0))<0)
	{
		write( 0, "\n Error al iniciar el semaforo 2", 0 );
		exit(1);
	}

    iPid = fork();
    if( !iPid ){

		//HIJO
		*shmvar = -999;

		if((shmid = shmget(100, sizeof(shmvar))) < 0)
		{
			write( 0, "\n Hijo: error en shmget", 0 );
			exit(1);
		}

		if((shmat(shmid, shmvar)) < 0)
		{
			write( 0, "\n Hijo: error en shmat", 0 );
			exit(1);
		}

		while(i>0)
		{
			sem_wait(&semaforo2);
			i--;
			iFnImprimirNumero("\n Hijo: ", *shmvar);
			sem_post(&semaforo1);
		}

		if((sem_close(&semaforo1))<0)
		{
			write( 0, "\n Error al cerrar el semaforo 1", 0 );
			exit(1);
		}
		if((sem_close(&semaforo2))<0)
		{
			write( 0, "\n Error al cerrar el semaforo 2", 0 );
			exit(1);
		}
		if((shmdt(shmid)) < 0){
			write( 0, "\n Hijo: error en shmdt", 0 );
			exit(1);
		}

		write( 0, "\n", 0 );
		exit(0);
	} else {

		//PADRE

		*shmvar = -999;

		if((shmid = shmget(100, sizeof(shmvar))) < 0)
		{
			write( 0, "\n Padre: error en shmget", 0 );
			exit(1);
		}

		if((shmat(shmid, shmvar)) < 0)
		{
			write( 0, "\n Padre: error en shmat", 0 );
			exit(1);
		}

		while(i>0)
		{
			sem_wait(&semaforo1);
			*shmvar=i--*100;
			iFnImprimirNumero("\n Padre: ", *shmvar);
			sem_post(&semaforo2);
		}

		if((shmdt(shmid)) < 0){
			write( 0, "\n Padre: error en shmdt", 0 );
			exit(1);
		}

        //TODO - Revisar por que si no esperamos al hijo se genera una excepcion
        waitpid( iPid, &iStatus, 0);
		exit(0);
	}
	
  return 0;
}


int iFnImprimirNumero( const char *cncpBuffer, int iNumero ){
	write( 0, "\n ", 0 );
	write( 0, cncpBuffer, 0 );
	systest( iNumero );
	
	return 0;
}
