#include <kernel/semaforo.h>
#include <kernel/definiciones.h>
#include <kernel/pcb.h>
#include <kernel/system.h>
#include <video.h>

extern stuPCB pstuPCB[CANTMAXPROCS];
extern unsigned long ulProcActual;

/*lista de semaforos disponibles*/
semaforo semaforosEnElSistema[CANTMAXSEM];	
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnInicializarSemaforos()
{
	int pid,i;
	semaforo sem;
	
	pid=0;
	sem.valor = 0;
	sem.pshared = 0;
	sem.head = 0;
	sem.tail = 0;
	sem.procesosEnCola = 0;

	sem.inicializado = FALSE;

	/*Inicializo la cola del semaforo*/
	for (i=0;i<CANTMAXPROCCOLA;i++)
	{
		sem.cola[i]=pid;
	}
	/*Inicializo el vector de semaforos*/	
	for (i=0;i<CANTMAXSEM;i++)
	{
		semaforosEnElSistema[i]=sem;
	}
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnSemInit(sem_t *sem, int pshared, unsigned int value)
{
	sem = (sem_t*)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)sem);

	sem->sem_id = agregarSemaforoAlSistema(pshared,value);

	if(sem->sem_id != -1)
	{
		return 0;
	}
	else
	{	
		return -1;
	}
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnSemClose(sem_t *sem)
{
	int posicion,i;

	sem = (sem_t*)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)sem);

	//BUSCO SEMAFORO EN EL SISTEMA
	if(validaExistenciaSemaforo(sem) != -1)
	{
		posicion = sem->sem_id;

		/*Inicializo la cola del semaforo*/
		for (i=0;i<CANTMAXPROCCOLA;i++)
		{
			while(iFnHayProcesoEnEspera(&semaforosEnElSistema[posicion]))
			{
				unsigned long int procesoADespertar = uliFnDesencolarProceso(&semaforosEnElSistema[posicion]);
				pstuPCB[procesoADespertar].iEstado = PROC_LISTO;
				vFnPlanificador();
			}
			semaforosEnElSistema[posicion].cola[i] = 0;
		}

		//MARCAR COMO SEMAFORO QUE NO SE USA
		semaforosEnElSistema[posicion].valor=0;
		semaforosEnElSistema[posicion].pshared=0;
		semaforosEnElSistema[posicion].inicializado = FALSE;

		return 0;
	}

	return -1;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnHayLugarEnLaCola(semaforo *sem){
	return !(sem->procesosEnCola==CANTMAXPROCCOLA);
}


/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnHayProcesoEnEspera(semaforo *sem)
{	
	return sem->procesosEnCola>0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
unsigned long int uliFnDesencolarProceso(semaforo *sem)
{
	unsigned long retorno;
	retorno = sem->cola[sem->head];
	sem->cola[sem->head] = 0;
	sem->head = (sem->head+1)%CANTMAXPROCCOLA;
	sem->procesosEnCola--;
	return retorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnEncolarProceso(semaforo *sem, unsigned long int uliId)
{
	sem->cola[sem->tail]=uliId;
	sem->procesosEnCola++;
	sem->tail=(sem->tail+1)%CANTMAXPROCCOLA;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnSemWait(sem_t *sem)
{
	sem = (sem_t*)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)sem);

	semaforosEnElSistema[sem->sem_id].valor--;
	if(semaforosEnElSistema[sem->sem_id].valor < 0){
		if(iFnHayLugarEnLaCola(&semaforosEnElSistema[sem->sem_id])){
			vFnEncolarProceso(&semaforosEnElSistema[sem->sem_id], pstuPCB[ulProcActual].ulId);
			pstuPCB[ulProcActual].iEstado = PROC_DETENIDO;
			vFnPlanificador();
		}
		else
		{
			//TODO ver q onda esto, como manejamos este error de no espacio en la cola
			return -1;
		}
	}
	return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnSemPost(sem_t *sem)
{
	int i;
	sem = (sem_t*)(pstuPCB[ ulProcActual ].uiDirBase + (unsigned int)sem);

	//validamos si el semaforo esta inicializado
	if(semaforosEnElSistema[sem->sem_id].inicializado==FALSE)
	{
		return -1;
	}

	semaforosEnElSistema[sem->sem_id].valor++;
	if(iFnHayProcesoEnEspera(&semaforosEnElSistema[sem->sem_id]))
	{
		unsigned long int idProcesoADespertar = uliFnDesencolarProceso(&semaforosEnElSistema[sem->sem_id]);
		for(i=0;i<CANTMAXPROCS;i++)
		{
			if(pstuPCB[i].ulId == idProcesoADespertar)
			{
				pstuPCB[i].iEstado = PROC_LISTO;
				vFnPlanificador();
				return 0;
			}
		}
	}

	return -1;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int agregarSemaforoAlSistema(int pshared, unsigned int value)
{
	int i;

	for(i=0;i<CANTMAXSEM;i++)
	{
	   if(semaforosEnElSistema[i].inicializado == 0)
	   {
		   semaforosEnElSistema[i].valor = value;
		   semaforosEnElSistema[i].inicializado = TRUE;
			semaforosEnElSistema[i].pshared = pshared;
			return i;
	   }
	}

	return -1;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int validaExistenciaSemaforo(sem_t *sem)
{
	//COMPRUEBO QUE EXISTE EL SEMAFORO
	if(semaforosEnElSistema[sem->sem_id].inicializado != FALSE)
		return 0;
	else
		return -1;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnVerSemaforos()
{
	int i,j;


	vFnImprimir("\n\nSemaforos del Sistema:\n");
	for (i=0;i<CANTMAXSEM;i++)
	{
		vFnImprimir("Valor: %d, Pshared: %d, Inicializado: %d, Cola: ",semaforosEnElSistema[i].valor,semaforosEnElSistema[i].pshared,semaforosEnElSistema[i].inicializado);

		for (j=0;j<CANTMAXPROCCOLA;j++){
			vFnImprimir("[%d]",semaforosEnElSistema[i].cola[j]);
		}

		vFnImprimir("\n");
	}
}
