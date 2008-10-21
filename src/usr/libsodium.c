#include <usr/libsodium.h>

/* definicion de la variable global 'errno' */
int errno = 0;

#define ASM_SYS_CALL_0( liRetorno, sys_num ) 			\
	__asm__ volatile ( 				\
		"movl	%1, %%eax \n\t" 		\
		"int	$0x80	  \n\t" 		\
		"movl	%%eax, %0 \n\t" 		\
		: "=g"( liRetorno ) 				\
		: "g"( sys_num ) )

#define ASM_SYS_CALL_1( liRetorno, sys_num, arg1 )		\
	__asm__ volatile ( 				\
		"movl	%1, %%eax \n\t" 		\
		"movl	%2, %%ebx \n\t" 		\
		"int	$0x80	  \n\t" 		\
		"movl	%%eax, %0 \n\t" 		\
		: "=g"( liRetorno ) 				\
		: "g"( sys_num ), 			\
		  "g"( arg1 ) )

#define ASM_SYS_CALL_2( liRetorno, sys_num, arg1, arg2 )		\
	__asm__ volatile ( 				\
		"movl	%1, %%eax \n\t" 		\
		"movl	%2, %%ebx \n\t" 		\
		"movl	%3, %%ecx \n\t" 		\
		"int	$0x80	  \n\t" 		\
		"movl	%%eax, %0 \n\t" 		\
		: "=g"( liRetorno ) 				\
		: "g"( sys_num ), 			\
		  "g"( arg1 ), 				\
		  "g"( arg2 ) ) 			\

#define ASM_SYS_CALL_3( liRetorno, sys_num, arg1, arg2, arg3 )	\
	__asm__ volatile ( 				\
		"movl	%1, %%eax \n\t" 		\
		"movl	%2, %%ebx \n\t" 		\
		"movl	%3, %%ecx \n\t" 		\
		"movl	%4, %%edx \n\t" 		\
		"int	$0x80	  \n\t" 		\
		"movl	%%eax, %0 \n\t" 		\
		: "=g"( liRetorno ) 				\
		: "g"( sys_num ), 			\
		  "g"( arg1 ), 				\
		  "g"( arg2 ), 				\
		  "g"( arg3 ) ) 			\

#define ASM_SYS_CALL_4( liRetorno, sys_num, arg1, arg2, arg3, arg4 )	\
	__asm__ volatile ( 				\
		"movl	%1, %%eax \n\t" 		\
		"movl	%2, %%ebx \n\t" 		\
		"movl	%3, %%ecx \n\t" 		\
		"movl	%4, %%edx \n\t" 		\
		"movl	%5, %%edi \n\t" 		\
		"int	$0x80	  \n\t" 		\
		"movl	%%eax, %0 \n\t" 		\
		: "=g"( liRetorno ) 				\
		: "g"( sys_num ), 			\
		  "g"( arg1 ), 				\
		  "g"( arg2 ), 				\
		  "g"( arg3 ), 				\
		  "g"( arg4 ) ) 			\

#define SET_ERROR( liRetorno, errno )				\
	do { 				 		\
		if( (liRetorno) < 0 ){			\
			(errno) = -(liRetorno);		\
			(liRetorno) = -1;			\
		}					\
	} while( 0 );

#define SYS_CALL_0( liRetorno, errno, sys_num ) 		\
	do { 				 		\
		ASM_SYS_CALL_0( liRetorno, sys_num );		\
		SET_ERROR( liRetorno, errno );		\
	} while( 0 );

#define SYS_CALL_1( liRetorno, errno, sys_num, arg1 ) 		\
	do { 				 		\
		ASM_SYS_CALL_1( liRetorno, sys_num, arg1 );	\
		SET_ERROR( liRetorno, errno );		\
	} while( 0 );

#define SYS_CALL_2( liRetorno, errno, sys_num, arg1, arg2 ) 	\
	do { 				 		\
		ASM_SYS_CALL_2( liRetorno, sys_num, arg1, arg2 );	\
		SET_ERROR( liRetorno, errno );		\
	} while( 0 );

#define SYS_CALL_3( liRetorno, errno, sys_num, arg1, arg2, arg3 ) 	\
	do { 				 			\
		ASM_SYS_CALL_3( liRetorno, sys_num, arg1, arg2, arg3 );	\
		SET_ERROR( liRetorno, errno );			\
	} while( 0 );

#define SYS_CALL_4( liRetorno, errno, sys_num, arg1, arg2, arg3, arg4 ) 	\
	do { 				 			\
		ASM_SYS_CALL_4( liRetorno, sys_num, arg1, arg2, arg3, arg4);	\
		SET_ERROR( liRetorno, errno );			\
	} while( 0 );

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void exit( int iStatus ){
	long liRetorno;
	SYS_CALL_1( liRetorno, errno, __NR_exit, iStatus );
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int fork(){
	long liRetorno;
	SYS_CALL_0( liRetorno, errno, __NR_fork );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int read( int iFd, void *vpBuffer, size_t uiTamanio ){
	long liRetorno;
	SYS_CALL_3( liRetorno, errno, __NR_read, iFd, vpBuffer, uiTamanio );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int write( int iFd, const void *covpBuffer, size_t uiTamanio ){
	long liRetorno;
	SYS_CALL_3( liRetorno, errno, __NR_write, iFd, covpBuffer, uiTamanio );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long systest( long liNumero ){
	long liRetorno;
	SYS_CALL_1( liRetorno, errno, __NR_test, liNumero );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int waitpid( int iPid, int *piStatus, int iOptions ){
	long liRetorno;
	SYS_CALL_3( liRetorno, errno, __NR_waitpid, iPid, piStatus, iOptions );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int execve( const char *stNombreArchivo, char *const argv[], char *const envp[] ){
	long liRetorno;
	SYS_CALL_3( liRetorno, errno, __NR_execve, stNombreArchivo, argv, envp );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long time( long *liTiempo ){
	long liRetorno;
	SYS_CALL_1( liRetorno, errno, __NR_time, liTiempo );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long getpid(){
	long liRetorno;
	SYS_CALL_0( liRetorno, errno, __NR_getpid );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int kill( int iPid, int iSignal ){
	long liRetorno;
	SYS_CALL_2( liRetorno, errno, __NR_kill, iPid, iSignal );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
long getppid(){
	long liRetorno;
	SYS_CALL_0( liRetorno, errno, __NR_getppid );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int reboot( int iFlag ){
	long liRetorno;
	SYS_CALL_1( liRetorno, errno, __NR_reboot, iFlag );
	return liRetorno;
}

// ****************     TP 3 - 2007 - Syscalls de IPC    *****************

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sem_init( sem_t *sem, int pshared, unsigned int value ){
    /* 21/10/2008
     * Se declaraba params como un puntero y no se inicializaba, generando un
     * SEGFAULT.
     * Agregamos el puntero paux porque la macro de syscall no esta preparada
     * para recibir &params
     */
	//sem_init_params  * params;
	sem_init_params params, *paux;
	long liRetorno;
	params.pshared = pshared;
	params.value = value;

    paux = &params;

	SYS_CALL_2( liRetorno, errno, __NR_seminit, sem, paux );

	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sem_post( sem_t *sem ){
	long liRetorno;
	SYS_CALL_1( liRetorno, errno, __NR_sempost, sem);
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sem_wait( sem_t *sem ){
	long liRetorno;
	SYS_CALL_1( liRetorno, errno, __NR_semwait, sem);
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sem_close( sem_t *sem ){
	long liRetorno;
	SYS_CALL_1( liRetorno, errno, __NR_semclose, sem);
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int shmget( key_t key, size_t size ){	
	long liRetorno;
	SYS_CALL_2( liRetorno, errno, __NR_shmget, key, size);
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int shmat( int shmid, void * shmAddr ){
	long liRetorno;
	SYS_CALL_2( liRetorno, errno, __NR_shmat, shmid, shmAddr);
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int shmdt( int shmid ){
	long liRetorno;
	SYS_CALL_1( liRetorno, errno, __NR_shmdt, shmid);
	return liRetorno;
}

// ****************     TP 3 - 2007 - Syscalls de tiempo    *****************
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int stime(time_t *newtime){
	long liRetorno;
	SYS_CALL_1( liRetorno, errno, __NR_stime, newtime );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
clock_t times(tms *ptmsBuffer){
	long liRetorno;
	SYS_CALL_1( liRetorno, errno, __NR_times, ptmsBuffer );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int gettimeofday(timeval *ptimevalTp, timezone *ptimezoneTzp){
	long liRetorno;
	SYS_CALL_2( liRetorno, errno, __NR_gettimeofday, ptimevalTp, ptimezoneTzp );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int settimeofday(timeval *ptimevalTp, timezone *ptimezoneTzp){
	long liRetorno;
	SYS_CALL_2( liRetorno, errno, __NR_settimeofday, ptimevalTp, ptimezoneTzp );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int gettimer(int iWhich, itimerval *itimervalValue){
	long liRetorno;
	SYS_CALL_2( liRetorno, errno, __NR_gettimer, iWhich, itimervalValue );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int settimer(int iWhich, itimerval const *pcnitimervalValue, itimerval *pitimervalOvalue){
	long liRetorno;
	SYS_CALL_3( liRetorno, errno, __NR_settimer, iWhich, pcnitimervalValue, pitimervalOvalue );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int adjtimex(timex *ptimexBuffer){
	long liRetorno;
	SYS_CALL_1( liRetorno, errno, __NR_adjtimex, ptimexBuffer );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int nanosleep(timespec const *pcntimespecRequested_time, timespec *ptimespecRemaining){
	long liRetorno;
	SYS_CALL_2( liRetorno, errno, __NR_nanosleep, pcntimespecRequested_time, ptimespecRemaining );
	return liRetorno;
}
// ***************************************************************************

/*SysCall Planificacion*/
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int idle()
{
	long liRetorno;
	SYS_CALL_0( liRetorno, errno, __NR_idle );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sched_setparam (int p)
{
	
	long liRetorno;
	SYS_CALL_1( liRetorno, errno, __NR_sched_setparam, p);
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sched_getparam()
{
	long liRetorno;
	SYS_CALL_0( liRetorno, errno, __NR_sched_getparam );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sched_setscheduler( int p)
{
	
	long liRetorno;
	SYS_CALL_1( liRetorno, errno, __NR_sched_setscheduler,p );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sched_getscheduler()
{
	long liRetorno;
	SYS_CALL_0( liRetorno, errno, __NR_sched_getscheduler);
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sched_yield()
{
	long liRetorno;
	SYS_CALL_0( liRetorno, errno, __NR_sched_yield );
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sched_get_priority_max ()
{
	long liRetorno;
	SYS_CALL_0( liRetorno, errno, __NR_sched_get_priority_max);
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sched_get_priority_min ()
{
	long liRetorno;
	SYS_CALL_0( liRetorno, errno, __NR_sched_get_priority_min);
	return liRetorno;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int sched_rr_get_interval ()
{
	long liRetorno;
	SYS_CALL_0( liRetorno, errno, __NR_sched_rr_get_interval);
	return liRetorno;
}
/*
int modify_ldt (int func, void *ptr, unsigned long bytecount)
{
	long liRetorno;
	SYS_CALL_3( liRetorno, errno, __NR_modify_ldt, func, ptr, bytecount );
	return liRetorno;
}*/

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int ptrace( int iRequest, int iPid, void *pvAddr, void *pvData ){
	long liRetorno;

	/* Declarando estructura local de la funcion */
	__ptrace_param *pstuParam;
	/*  */
	
	/* Todo se puede psara como void* */
	void *pvDirParam;
	/*  */
	
	/* Inicializando estructura de parametros */
	pstuParam->iPid = iPid;
	pstuParam->pvAddr = pvAddr;
	pstuParam->pvData = pvData;
	/*  */

	/* Estableciendo direccion de estructura */
	pvDirParam = (void *)pstuParam;
	/*  */

	/* Llamando a la syscall con el puntero a la estructura */
	SYS_CALL_2( liRetorno, errno, __NR_ptrace, iRequest, pvDirParam );
	/*  */

	
	/* Recolectando modificaciones (si es que las hay) de la syscall.
	 * El contenido se castea a int: no importa el tipo de dato, pero
	 * si importa que sea de 4 bytes (se tracea de 1 palabra); luego
	 * desde el proceso se puede interpretar como sea necesario. */
	*(int*)pvAddr = (int)pstuParam->pvAddr;
	if ( iRequest != PTRACE_GETREGS && iRequest != PTRACE_SETREGS )
		*(int*)pvData = (int)pstuParam->pvData;
	/*  */

	return liRetorno;
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
unsigned long __brk (unsigned long limite)
{
	long liRetorno;
	SYS_CALL_1( liRetorno, errno, __NR_brk, limite);
	return liRetorno;
}
