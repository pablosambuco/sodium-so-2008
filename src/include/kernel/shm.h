/**
	\file kernel/shm.h
	\brief Biblioteca de funciones para memorua compartida
*/
#ifndef SHM_H
#define SHM_H

/*!<cantidad de memorias compartidas del sistema*/
#define CANTMAXSHM	5
/*!<cantidad de procesos atacheados a una memoria compartida*/
#define CANTMAXPROCSHM	5
/*!<cantidad de memorias compartidas por proceso*/
#define MAXSHMEMPORPROCESO 5

#define TRUE 1
#define FALSE 0

typedef unsigned int size_t;
typedef int key_t; /*!<identificador de la variable compartida*/
typedef int bool;

/** \brief utilizado para ver las variables compartidas en el PCB*/
typedef struct _stuMemoriasAtachadas_
{
	bool utilizada;
	int posicionEnShMem;
	int posicionEnAttach;
}stuMemoriasAtachadas;

/** \brief utilizado para ver que procesos estan atachados a esa key*/
typedef struct _attach_
{
	int    pid; /*!< si esta desocupado implica que el pid va a ser -1*/
	void * ptrShAddr;

}attach;

/** \brief todos las variables compartidas que tengo en el sistema */
typedef struct _shMem_
{
	key_t key;
	unsigned long int tamanio;
	attach procAtt[CANTMAXPROCSHM];
	bool declarada;
}shMem;

shMem memoriasCompartidas[CANTMAXSHM];

// declaracion de funciones

/** \sa int shmget(key_t key, size_t size, int shmflg) */
int iFnShmGet(key_t key, size_t size);

/** \sa char *shmat( int shmid, void *shmaddr, int shmflg )*/
int iFnShmAt(int shmid, void * shmAddr);

/** \sa int shmdt(const void *shmaddr);*/
int iFnShmDt(int shmid);

void vFnInicializarShms();
void vFnVerShm();

/**
	\brief funcion llamada desde vfnPlanificador antes de realizar el context switch. Mantiene la memoria compartida.
*/
void vFnCopiarVariablesCompartidas();
/**
	\brief funcion llamada desde vfnPlanificador antes de realizar el context switch. Copia las variables del proceso viejo al nuevo.
*/
void vFnCopiarVariable(void * dest, void * src, size_t tamanio);

#endif
