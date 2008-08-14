/**
	\file kernel/semaforo.h
	\brief Biblioteca de funciones de Semaforos de sistema
*/
#ifndef SEMAFORO_H
#define SEMAFORO_H

/*!< cantidad maxima de semaforos*/
#define CANTMAXSEM	5
/*!< cantidad maxima de procesos en cola a espera de un recurso */
#define CANTMAXPROCCOLA	5

#define TRUE 1
#define FALSE 0

typedef struct 
{
	int inicializado; /*!< 0 NO - 1 SI */
	int valor; /*!< cantidad de recursos del semaforo*/
	unsigned long int head;
	unsigned long int tail;

	unsigned long int procesosEnCola;

	unsigned long int cola[CANTMAXPROCCOLA]; /*!< cola de procesos en espera del recurso*/
	unsigned long int pshared;
}semaforo;

typedef struct 
{
	int sem_id; /*!< identificador del semaforo*/
}sem_t;

typedef struct{
	int pshared;
	unsigned int value;
} sem_init_params;

/** \brief funcion que encola un proceso que esta a la espera de un recurso del semaforo*/
void vFnEncolarProceso(semaforo *sem, unsigned long int uliId);

/** \brief  utiliza para inicializar la cola de cada semaforo*/
void vFnInicializaCola(unsigned long int *cola);	

/** \brief funcion que verifica si hay un proceso en espera del recurso del semaforo*/
int iFnHayProcesoEnEspera(semaforo *sem);

/** \brief funcion que retorna el proceso que debe continuar su ejecucion cuando se libera un recurso del semaforo */
unsigned long int uliFnDesencolarProceso(semaforo *sem);

/** \brief funcion que inicializa el vector estatico de semaforos disponibles en el sistema*/
void vFnInicializarSemaforos();

/**
	\note The sem_init() function will fail if:\n
	\n \b [EINVAL]	\n
    The value argument exceeds SEM_VALUE_MAX. 
	\n \b [ENOSPC] \n
    A resource required to initialise the semaphore has been exhausted, or the limit on semaphores (SEM_NSEMS_MAX) has been reached. 
	\n \b [ENOSYS] \n
    The function sem_init() is not supported by this implementation. 
	\n \b [EPERM] \n 
    The process lacks the appropriate privileges to initialise the semaphore.
*/
int iFnSemInit(sem_t *sem, int pshared, unsigned int value);

/**
	\note The sem_close() function will fail if:\n
	\n \b [EINVAL] \n 
	The sem argument is not a valid semaphore descriptor. 
	\n \b [ENOSYS] \n 
	The function sem_close() is not supported by this implementation. 
*/
int iFnSemClose(sem_t *sem);

/**
\note 
	The sem_wait() and sem_trywait() functions will fail if:\n

	\n \b [EAGAIN] \n
    The semaphore was already locked, so it cannot be immediately locked by the sem_trywait() operation ( sem_trywait only). 
	\n \b [EINVAL] \n
    The sem argument does not refer to a valid semaphore. 
	\n \b [ENOSYS] \n
    The functions sem_wait() and sem_trywait() are not supported by this implementation. 

	\nThe sem_wait() and sem_trywait() functions may fail if:

	\n \b [EDEADLK] \n
    A deadlock condition was detected. 
	\n \b [EINTR] \n
    A signal interrupted this function. 
*/
int iFnSemWait(sem_t *sem);

/**
	\note The sem_post() function will fail if:\n

	\n \b [EINVAL] \n
    The sem does not refer to a valid semaphore. 
	\n \b [ENOSYS] \n
    The function sem_post() is not supported by this implementation. 
*/
int iFnSemPost(sem_t *sem);

int validaExistenciaSemaforo(sem_t *sem);

/** \brief funcion que inicializa un semaforo en el sistema, lo pone ocupado y le asigna valor*/
int agregarSemaforoAlSistema(int pshared, unsigned int value);

void vFnVerSemaforos();

#endif
