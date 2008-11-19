/**
	\file usr/libsodium.h
	\brief Funciones de biblioteca del usuario del sistema
*/
#ifndef __LIB_SODIUM_H
#define __LIB_SODIUM_H

#include <kernel/syscall.h>
#include <usr/tipos.h>  //Tipos de variables 

#define RR 1
#define FIFO 0
#define BTS 2


/*
 * \brief Variable errno, analoga a errno de libc
 */
extern int errno;

/**
	\defgroup libSodi Funciones de biblioteca del sodium
*/
/*@{*/
void exit( int iStatus );
int fork();
int read( int iFd, void *vpBuffer, size_t uiTamanio );
int write( int iFd, const void *covpBuffer, size_t uiTamanio );
long systest( long iNumero );
int waitpid( int iPid, int *piStatus, int iOptions );
int execve( const char *stNombreArchivo, char *const argv[], char *const envp[] );
long time( long *liTiempo );
long getpid();
int kill( int iPid, int iSignal );
long getppid();
int reboot( int iflag );
int sem_init( sem_t *sem, int pshared, unsigned int value );
int sem_post( sem_t *sem );
int sem_wait( sem_t *sem );
int sem_close( sem_t *sem );
int shmget( key_t key, size_t size );
int shmat( int shmid, void * shmAddr );
int shmdt( int shmid );
int sumar(int a, int b);
long nice(int iIncremento);
long getpriority(int which, int who);
long setpriority(int,int,int);
long clone(int (*fn)(void * args),void * child_stack,int flags, void * arg);

/* Funciones de syscall tiempo */
int stime(time_t *newtime);
clock_t times(tms *ptmsBuffer);
int gettimeofday(timeval *ptimevalTp, timezone *ptimezoneTzp);
int settimeofday(timeval *ptimevalTp, timezone *ptimezoneTzp);
int gettimer(int iWhich, itimerval *pitimervalValue);
int settimer(int iWhich, itimerval const *pcnItimervalValue, itimerval *pitimervalOvalue);
int adjtimex(timex *ptimexBuffer);
int nanosleep(timespec const *pcnTimespecRequested_time, timespec *ptimespecRemaining);

/* SysCall proceso nulo*/
int idle();

/* SysCall planificacion*/

int sched_setparam( int p);
int sched_getparam();
int sched_setscheduler( int p);
int sched_getscheduler();
int sched_yield();
int sched_get_priority_max();
int sched_get_priority_min();
int sched_rr_get_interval();

/* SysCall ptrace */
int ptrace( int iRrequest, int iPid, void *pvAddr, void *pvData );

/* SysCall brk*/
unsigned long __brk (unsigned long limite);

/*@}*/

typedef char* va_list;

// MACROS

/**
 *  \brief Constantes que representan los colores
 *
 * Utilizada para manejar argumentos variables\n
 * Extraida del compilador GNU/gcc @see http://www.gnu.org
 * \param type Numero
 */
#define va_rounded_size(type) \
(((sizeof (type) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

/**
 * Utilizada para manejar argumentos variables
 * Extraida del compilador GNU/gcc @see http://www.gnu.org
 *
 * \param ap Lista de parametros (puntero)
 * \param v valor para calcular inicio del proximo elemento de la lista
 */
#define va_start(ap, v) \
((void) (ap = (va_list) &v + va_rounded_size (v)))

/**
 * Utilizada para manejar argumentos variables
 * Extraida del compilador GNU/gcc @see http://www.gnu.org
 * \param ap lista de parametros (puntero)
 * \param type Valor leido para pasar al siguiente valor de la lista
 */
#define va_arg(ap, type) \
(ap += va_rounded_size (type), *((type *)(ap - va_rounded_size (type))))

/**
 * Utilizada para manejar argumentos variables
 * Extraida del compilador GNU/gcc @see http://www.gnu.org
 * \param ap lista de parametros
 */
#define va_end(ap) ((void) (ap = 0))



#endif
