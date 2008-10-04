/**
	\file kernel/syscall.h
	\brief Contiene constantes y funciones de las syscall del lado del kernel
*/
#ifndef __SYSCALL_H
#define __SYSCALL_H

#include <kernel/signal.h>
#include <kernel/semaforo.h>
#include <kernel/shm.h>
#include <kernel/tiempo.h>

/**
 * \defgroup syscallNum Numeros de syscall
 */
/*@{*/
#define __NR_exit         1
#define __NR_fork         2
#define __NR_test         3
#define __NR_read         4
#define __NR_write        5
#define __NR_waitpid      7
#define __NR_execve       11
#define __NR_time         13
#define __NR_getpid       20
#define __NR_kill         37
#define __NR_getppid      64
#define __NR_reboot       88

/* Numeros de syscalls de proceso agregadas por el grupo IPC */

#define __NR_seminit      200
#define __NR_semclose     201
#define __NR_sempost      202
#define __NR_semwait      203
#define __NR_shmget       204
#define __NR_shmat        205
#define __NR_shmdt        206
#define __NR_sumar        500
#define __NR_imprimir_4_params	89

/* Numeros de syscalls de proceso agregadas por el grupo CLONE */

#define __NR_nice         34
#define __NR_getpriority  96
#define __NR_setpriority  97
#define __NR_clone        120

/* Numeros de syscalls de proceso agregadas por el grupo TIEMPO */

#define __NR_stime        25
#define __NR_times        43
#define __NR_gettimeofday 78
#define __NR_settimeofday 79
#define __NR_gettimer     104
#define __NR_settimer     105
#define __NR_adjtimex     124
#define __NR_nanosleep    162

/*syscall del proceso nulo*/
#define __NR_idle         112


/*numeros de syscalls de planificacion, propias de sodium */

#define __NR_sched_setparam         250
#define __NR_sched_getparam         251
#define __NR_sched_setscheduler     252
#define __NR_sched_getscheduler     253
#define __NR_sched_yield            254
#define __NR_sched_get_priority_max 255
#define __NR_sched_get_priority_min 256
#define __NR_sched_rr_get_interval  257
//#define __NR_modify_ldt   123  

/*numeros de syscalls de ptrace, propias de sodium */
#define __NR_ptrace       26

/*numeros de syscalls de brk*/
#define __NR_brk          45


/*@}*/
/*!< <!--fin de grupo de doxygen syscallNum --> */


/*constantes utilizadas en las syscalls getpriority 
 * y setpriority
 */
#define PRIO_PROCESS 0 /*!< se refiere a la prioridad del proceso */
#define PRIO_PGRP    1 /*!< grupo de procesos. No esta implementada*/
#define PRIO_USER    2 /*!< usuario. No esta implementada */

/*constantes utilizadas en la syscall clone
 */

#define CLONE_VM      0
#define CLONE_FS      1
#define CLONE_FILES   2
#define CLONE_SIGHAND 3
#define CLONE_PID     4

/**
 * \defgroup syscallErrno Retornos de las Syscall
 * \note se almacenan en errno
 */
/*@{*/
#define EXITO     0
#define ENOSYS    2
#define EINVAL    3
#define ECHILD    4
#define EAGAIN    5
#define ENOMEM    6
#define EFAULT    7
#define EPERM     8
#define ESRCH     9
#define ENOENT    10
#define ENOMNT    11
/*@}*/

/** \brief Opciones de la syscall lFnPtrace */
enum __ptrace_request {
	PTRACE_TRACEME,
	PTRACE_PEEKTEXT,
	PTRACE_PEEKDATA,
	PTRACE_PEEKUSER,
	PTRACE_POKETEXT,
	PTRACE_POKEDATA,
	PTRACE_POKEUSER,
	PTRACE_GETREGS,
	PTRACE_GETFPREGS,
	PTRACE_SETREGS,
	PTRACE_SETFPREGS,
	PTRACE_CONT,
	PTRACE_KILL,
	PTRACE_ATTACH,
	PTRACE_DETACH
};

/** \brief Estructura para el paso de parametros de la syscall lFnPtrace */
typedef struct {
	int iPid;
	void *pvAddr;
	void *pvData;
}__ptrace_param;


/** \brief Estructura usada para poder pasar mas de 2 parametros a un syscall */
typedef struct _stuRegs_
{
  long ebx;
  long ecx;
  long edx;
  long esi;
  long edi;
  long ebp;
  long eax;
  int xds;
  int xes;
  long orig_eax;
  long eip;
  int xcs;
  long eflags;
  long esp;
  int xss;
} stuRegs;


/**
 * \defgroup syscalls Prototipos de llamadas a syscall
 */
/*@{*/
long lFnSysExit( int status );
long lFnSysFork();
long lFnSysTest( long num );
long lFnSysRead( int, void *buf, size_t count );
long lFnSysWrite( int, const void *buf, size_t count );
long lFnSysWaitPid( int pid, int *status, int options );
long lFnSysExecve( const char *filename, char *const argv[], char *const envp[] );
long lFnSysTime( long *t );
long lFnSysGetPid();
long lFnSysKill( int pid, int sig );
long lFnSysGetPPid();
long lFnSysReboot( int flag );
long lFnSysSemInit( sem_t *sem, sem_init_params * params);
long lFnSysSemPost( sem_t *sem );
long lFnSysSemWait( sem_t *sem );
long lFnSysSemClose( sem_t *sem );
long lFnSysShmGet( key_t key, size_t size );
long lFnSysShmAt( int shmid, void * shmAddr );
long lFnSysShmDt( key_t key );
/*!< prueba de syscall 4-9*/
int iFnSysSumar(int a, int b, int * res);

/*syscalls de procesos*/

long lFnSysNice( int );
long lFnSysGetpriority( int, int );
long lFnSysSetpriority( int, int, int );
long lFnSysClone(stuRegs *,int flags);

/*SysCall proceso nulo*/
long lFnSysIdle();

/*SysCall planificacion*/

long lFnSysSchedSetParam(int p);
long lFnSysSchedGetParam();
long lFnSysSchedSetScheduler(int p);
long lFnSysSchedGetScheduler();
long lFnSysSchedYield();
long lFnSysSchedGetPriorityMax();
long lFnSysSchedGetPriorityMin();
long lFnSysSchedRrGetInterval();
//long lFnSysModifyLdt( int func, void *ptr, unsigned long bytecount);


//TP 3 - 2007 - Syscalls de tiempo
//Funciones
long lFnSysStime(time_t *newtime);
long lFnSysTimes(tms *ptmsbuffer);
long lFnSysGettimeofday(timeval *ptimervalTp, timezone *ptimezoneTzp);
long lFnSysSettimeofday(timeval *ptimevalTp, timezone *ptimezoneTzp);
long lFnSysGettimer(int iWhich, itimerval *pitimervalValue);
long lFnSysSettimer(int iWhich, itimerval const *pcnitimervalValue, itimerval *ptimervalOvalue);
long lFnSysAdjtimex(timex *ptimexBuffer);
long lFnSysNanosleep(timespec const *pcntimespecRequested_time, timespec *ptimespecRemaining);


// TP 3 - 2007 - Syscalls de ptrace
long lFnSysPtrace( int request, void *pvDirParam );
/*@}*/

/*
 * TP 2 - 2008 - Syscalls de Memoria Dinamica   
 */
unsigned long ulFnSysBrk(unsigned long brk);




#endif
