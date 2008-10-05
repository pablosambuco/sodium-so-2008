#include <kernel/gdt.h>
#include <kernel/pcb.h>
#include <video.h>
#include <kernel/registros.h>
#include <kernel/init.h>
#include <kernel/idle.h>
#include <kernel/sched.h>
#include <kernel/syscall.h>
#include <kernel/libk/string.h>
#include <kernel/shm.h>

#include <kernel/mem/memoria_s.h>

extern dword pdwGDT;

unsigned int uiUltimoPid = 0;
unsigned long ulProcActual = 0;
unsigned long ulUltimoProcesoEnFPU = 0; //PID del ultimo proceso que uso el FPU

//stuPCB pstuPCB[CANTMAXPROCS];
stuTSS stuTSSTablaTareas[CANTMAXPROCS];

#define TOTAL_ENTRADAS_GDT (sizeof(stuEstructuraGdt) / sizeof(stuGDTDescriptor))

unsigned char iMapaGDT[ TOTAL_ENTRADAS_GDT / 8 ]; 

//TODO - lala Limpiar
//#define SEGMENT_SIZE    0x20000 // 128 Kb
#define SEGMENT_SIZE    0x08000 // 32 Kb
//#define SEGMENT_SIZE    0x00800 // 2 Kb //Init no entra en 2Kb y explota!

#define _SET_BIT( bitmap, pos, set )                     \
    do{                                \
        if( set ){                        \
            bitmap[ (pos-(pos%8)) / 8 ] |= (1 << (7-((pos)%8))); \
        } else    {                        \
            bitmap[ (pos-(pos%8)) / 8 ] &= ~(1 << (7-((pos)%8))); \
        }                            \
    }while( 0 )

#define _GET_BIT( bitmap, pos )                     \
            (bitmap[ (pos) / 8 ] & (1 << (7-((pos)%8))))     \

#define mapa_gdt_set( pos, set ) _SET_BIT( iMapaGDT, pos, set )
#define mapa_gdt_get( pos ) _GET_BIT( iMapaGDT, pos )

#define mapa_segmentos_set( pos, set ) _SET_BIT( iMapaSegmentos, pos, set )
#define mapa_segmentos_get( pos ) _GET_BIT( iMapaSegmentos, pos )


/**
 * @brief Inicializa la GDT y la tabla de PCBs
 * @param Puntero a la GDT
 */
void vFnGdtInicializar (dword pdwGDT) {
    int iN;
    pstuTablaGdt = (stuEstructuraGdt *) pdwGDT;

    /* Inicialización de GDT */

    for( iN = 0; iN < sizeof( iMapaGDT ) / sizeof( iMapaGDT[0]); iN++ )
        iMapaGDT[iN] = 0; // bloque de entradas de la GDT no utilizadas

    mapa_gdt_set( 0, 1 ); /* descriptor nulo */
    mapa_gdt_set( 1, 1 ); /* descriptor codigo del kernel */
    mapa_gdt_set( 2, 1 ); /* descriptor datos+stack del kernel */

    /* Inicialización de tabla de PCBs */

    for (iN = 0; iN < CANTMAXPROCS; iN++) {
      pstuPCB[iN].iEstado = PROC_NO_DEFINIDO;
    }

}


/**
 * @brief Devuelve la posicion en el vector de procesos del pid indicado
 * @param PID del proceso
 * @returns Posición en el vector de procesos o -1 si no existe
 * @date 04/08/2008
 */
int iFnBuscaPosicionProc (unsigned long ulPid) {
    int iN = 0;

    while (iN < CANTMAXPROCS) {
        if( (pstuPCB[iN].ulId == ulPid) &&
            (pstuPCB[iN].iEstado != PROC_NO_DEFINIDO) &&
            (pstuPCB[iN].iEstado != PROC_ELIMINADO) ) {
                return iN;
        }
        iN++;
    }

    return -1;
}


/**
 * @brief Carga en el descriptor de la GDT indicado la direccion de memoria
 *        base y el limite (sin especificar granularidad)
 * @param uiPosicion Posicion en la GDT del descriptor a modificar
 * @param uiBase Direccion base (lineal)
 * @param uiLimite Longitud (afectado por la granularidad)
 * @returns posicion en la GDT del descriptor
 */
unsigned int
uiFnSetearBaseLimiteDescriptor( int uiPosicion,
                unsigned int uiBase, 
                unsigned int uiLimite ) {

    pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].usBaseBajo =
        uiBase & 0xFFFF;
    pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].ucBaseMedio =
        (uiBase >> 16) & 0xFF;
    pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].usBaseAlto =
        uiBase >> 24;
    pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].usLimiteBajo =
        uiLimite & 0xFFFF;
    pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].bitLimiteAlto =
        (uiLimite >> 16) & 0x0F;
    
    return uiPosicion;
}


/**
 * @brief Agrega un descriptor en la GDT
 * @param uiBase Direccion inicial del segmento en memoria
 * @param uiLimite Longitud del segmento (afectado por la granularidad)
 * @param uiOpt (tipo de descriptor, acceso, crecimiento, granularidad, etc)
 * @returns Offset desde la base de la GDT
 * @date 09/04/2006
 */
unsigned int
uiFnAgregarDescriptorGDT (
        unsigned int uiBase,
        unsigned int uiLimite,
        unsigned int uiOpt,
        int uiPosicion)
{
    uiFnSetearBaseLimiteDescriptor( uiPosicion, uiBase, uiLimite );
    pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].ucAcesso =
        (uiOpt + D_PRESENT) >> 8;
    pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].bitGranularidad =
        ((uiOpt & 0xff) >> 4);
  
    return uiPosicion;
}



//TODO - Verificar que la funcionalidad de uiFnAgregarDescriptorGDT sea la misma
//Se deja con fines explicativos
/* unsigned int
uiFnAgregarDescriptorGDT2(int uiPosicion,
              unsigned int uiBase, 
              unsigned int uiLimite,
              unsigned char type,
              unsigned char bit_s,
              unsigned char dpl,
              unsigned char bit_p,
              unsigned char avl,
              unsigned char bit_d_b,
              unsigned char bit_g )
{
    uiFnSetearBaseLimiteDescriptor( uiPosicion, uiBase, uiLimite );
  
    pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].ucAcesso =
        (type & 0x0f) |
        ((bit_s & 1) << 4) |
        ((dpl & 1) << 5) |
        ((bit_p & 1) << 7);

    pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].bitGranularidad =
        (avl & 1) |
        ((bit_d_b & 1) << 1) |
        ((bit_g & 1) << 1);

    return (uiPosicion);
}*/


/**
 * @brief Busca una posicion libre en el vector de procesos
 * @returns La posicion libre en el vector de procesos o -1 si no hay posiciones
 * libres
 * @date 04/08/2008
 */
int iFnBuscarPCBLibre() {
    int iPosicion = 0;
   
    while ( iPosicion < CANTMAXPROCS ) {
        if (pstuPCB[iPosicion].iEstado == PROC_ELIMINADO ||
            pstuPCB[iPosicion].iEstado == PROC_NO_DEFINIDO ) {
                return iPosicion;
        }
        iPosicion++;
    }
 
    return -1;
}


/**
 * @brief Busca una posicion libre en la GDT (mapa)
 * @returns La posicion libre en la GDT o 0 si no hay posicion libre
 * @date 04/08/2008
 */
unsigned int uiFnBuscarEntradaGDTLibre() {
    unsigned int uiPosicion = 0;
    
    while ( uiPosicion < TOTAL_ENTRADAS_GDT ) {
        if ( !mapa_gdt_get( uiPosicion ) ) {
            mapa_gdt_set( uiPosicion, 1 ); // marcamos como usada
            return uiPosicion;
        }
        uiPosicion++;
    }

  return 0;
}


/**
\brief Crea una TSS para una nueva TAREA_ESPECIAL (llamamos TAREA_ESPECIAL a los procesos 'simulados' por Sodium (Shell, Reloj y los lanzados con exec), aquellos que tienen su stack dentro de la TSS en espacio0, espacio1, etc).
\param pEIP puntero al codigo de la función que queremos ejecutar
\param iPosicion posicion dentro de la GDT
\param uiCS direccion del CS
\param uiDS direccion del DS
\param uiSS direccion del SS
\returns posicion en la tabla de procesos
\sa iFnCrearTSS
\date 02/10/2008
*/
int
iFnCrearTSSTareaEspecial (void *pEIP,
        int iPosicion,
        unsigned int uiCS,
        unsigned int uiDS,
        unsigned int uiSS)
{
    int iN = 0;

    stuTSSTablaTareas[iPosicion].trapbit = 0;
    stuTSSTablaTareas[iPosicion].uIOMapeoBase = sizeof (stuTSS);
    stuTSSTablaTareas[iPosicion].cs = uiCS;
    stuTSSTablaTareas[iPosicion].fs = uiDS;
    stuTSSTablaTareas[iPosicion].gs = uiDS;
    stuTSSTablaTareas[iPosicion].ds = uiDS;
    stuTSSTablaTareas[iPosicion].es = uiDS;
    stuTSSTablaTareas[iPosicion].ss = uiSS;
    stuTSSTablaTareas[iPosicion].ss0 = uiSS;
    stuTSSTablaTareas[iPosicion].ss1 = uiSS;
    stuTSSTablaTareas[iPosicion].ss2 = uiSS;
    
    stuTSSTablaTareas[iPosicion].esp0 =
        (unsigned int) &stuTSSTablaTareas[iPosicion].
        espacio0[TSS_TAMANIO_STACK_R0 - 8];
    
    stuTSSTablaTareas[iPosicion].esp1 =
        (unsigned int) &stuTSSTablaTareas[iPosicion].
        espacio1[TSS_TAMANIO_STACK_R1 - 8];
    
    stuTSSTablaTareas[iPosicion].esp2 =
        (unsigned int) &stuTSSTablaTareas[iPosicion].
        espacio2[TSS_TAMANIO_STACK_R2 - 8];
    
    stuTSSTablaTareas[iPosicion].esp =
        (unsigned int) &stuTSSTablaTareas[iPosicion].
        espacio0[TSS_TAMANIO_STACK_R0 - 8];
    
    stuTSSTablaTareas[iPosicion].ebp =
        (unsigned int) &stuTSSTablaTareas[iPosicion].
        espacio0[TSS_TAMANIO_STACK_R0 - 8];

    //Rellenamos los stacks de los distintos rings de ejecucion con patrones
    //faciles de distinguir. Facilita mucho el debugueo del contenido del stack
    //en tiempo de ejecucion con los comandos "stack" y "dump"
    for (iN = 0; iN < TSS_TAMANIO_STACK_R0; iN++) {
        stuTSSTablaTareas[iPosicion].espacio0[iN] = 0x11L;
    }
    for (iN = 0; iN < TSS_TAMANIO_STACK_R1; iN++) {
        stuTSSTablaTareas[iPosicion].espacio1[iN] = 0x22L;
    }
    for (iN = 0; iN < TSS_TAMANIO_STACK_R2; iN++) {
        stuTSSTablaTareas[iPosicion].espacio2[iN] = 0x33L;
    }

    //0x3202L; para tareas ring3
    stuTSSTablaTareas[iPosicion].uiEBandera = 0x202L;

    stuTSSTablaTareas[iPosicion].eip = (unsigned int) pEIP;
    stuTSSTablaTareas[iPosicion].ldt = 0;
    stuTSSTablaTareas[iPosicion].eax = 0;
    stuTSSTablaTareas[iPosicion].ebx = 0;
    stuTSSTablaTareas[iPosicion].ecx = 0;
    stuTSSTablaTareas[iPosicion].edx = 0;

    /* Valores para FPU (17-07-08) */
    //Redondeo al mas cercano, Doble precision, Interrupcion por
    //Precision enmascarada (bit PM),
    //el resto de las interrupciones sin enmascarar y Stack vacio. 
    stuTSSTablaTareas[iPosicion].fpu.control = 0x0360; 
    stuTSSTablaTareas[iPosicion].fpu.status = 0x0000;
    stuTSSTablaTareas[iPosicion].fpu.tag = 0xFFFF;
    /*    stuTSSTablaTareas[iPosicion].fpu.ip = 0;
    stuTSSTablaTareas[iPosicion].fpu.cs = 0;
    stuTSSTablaTareas[iPosicion].fpu.dp = 0;
    stuTSSTablaTareas[iPosicion].fpu.ds = 0;*/
    /* FIN Valores para FPU */

    return (iPosicion);
}


/**
\brief Crea una TSS para un nuevo proceso NORMAL (ver iFnCrearTSSTareaEspecial)
\param pEIP Puntero al codigo de la funcion que queremos ejecutar
\param pESP Puntero al stack del proceso
\param iPosicion Posicion dentro de la GDT
\param uiCS Direccion del CS
\param uiDS Direccion del DS
\param uiSS Direccion del SS
\returns Posicion en la tabla de TSSs
\sa iFnCrearTSSTareaEspecial
\date 02/10/2008
*/
int
iFnCrearTSS (void *pEIP,
        void *pESP,
        int iPosicion,
        unsigned int uiCS,
        unsigned int uiDS,
        unsigned int uiSS)
{
    stuTSSTablaTareas[iPosicion].trapbit = 0;
    stuTSSTablaTareas[iPosicion].uIOMapeoBase = sizeof (stuTSS);
    stuTSSTablaTareas[iPosicion].cs = uiCS;
    stuTSSTablaTareas[iPosicion].fs = uiDS;
    stuTSSTablaTareas[iPosicion].gs = uiDS;
    stuTSSTablaTareas[iPosicion].ds = uiDS;
    stuTSSTablaTareas[iPosicion].es = uiDS;
    stuTSSTablaTareas[iPosicion].ss = uiSS;

    /* Estos selectores nunca se usan, porque al estar ya en ring0, una
     * interrupcion no nos cambia de nivel de proteccion
     */
    stuTSSTablaTareas[iPosicion].ss0 = 0x00;
    stuTSSTablaTareas[iPosicion].ss1 = 0x00;
    stuTSSTablaTareas[iPosicion].ss2 = 0x00;
    
    stuTSSTablaTareas[iPosicion].esp0 = (unsigned int) 0;
    stuTSSTablaTareas[iPosicion].esp1 = (unsigned int) 0;
    stuTSSTablaTareas[iPosicion].esp2 = (unsigned int) 0;
    
    stuTSSTablaTareas[iPosicion].esp = (unsigned int) pESP;
    stuTSSTablaTareas[iPosicion].ebp = (unsigned int) pESP;
    
    //0x3202L; para tareas ring3
    stuTSSTablaTareas[iPosicion].uiEBandera = 0x202L;
    
    stuTSSTablaTareas[iPosicion].eip = (unsigned int) pEIP;
    stuTSSTablaTareas[iPosicion].ldt = 0;
    stuTSSTablaTareas[iPosicion].eax = 0;
    stuTSSTablaTareas[iPosicion].ebx = 0;
    stuTSSTablaTareas[iPosicion].ecx = 0;
    stuTSSTablaTareas[iPosicion].edx = 0;

    /* Valores para FPU (17-07-08) */
    //Redondeo al mas cercano, Doble precision, Interrupcion por
    //Precision enmascarada (bit PM),
    //el resto de las interrupciones sin enmascarar y Stack vacio. 
    stuTSSTablaTareas[iPosicion].fpu.control = 0x0360; 
    stuTSSTablaTareas[iPosicion].fpu.status = 0x0000;
    stuTSSTablaTareas[iPosicion].fpu.tag = 0xFFFF;
    /*    stuTSSTablaTareas[iPosicion].fpu.ip = 0;
    stuTSSTablaTareas[iPosicion].fpu.cs = 0;
    stuTSSTablaTareas[iPosicion].fpu.dp = 0;
    stuTSSTablaTareas[iPosicion].fpu.ds = 0;*/
    /* FIN Valores para FPU */

    return (iPosicion);
}


/**
\brief Crea un PCB
\param iPosicion Posicion dentro de la tabla de PCBs
\param pEIP Puntero al inicio del codigo
\param stNombre Nombre del proceso
\param uiIndiceGDT_CS Posicion del descriptor del segmento de codigo en la GDT
\param uiIndiceGDT_DS Posicion del descriptor del segmento de datos en la GDT
\param uiIndiceGDT_TSS Posicion del descriptor de la TSS en la GDT
\param uiPosTSS Posicion de la TSS en la tabla de TSS (idem iPosicion)
\param uiDirBase Direccion base de memoria (absoluta)
\param uiLimite Longitud de memoria (aqui NO existe granularidad)
\returns iPosicion
*/
int iFnCrearPCB( int iPosicion, 
         void *pEIP,
         char *stNombre,
         unsigned int uiIndiceGDT_CS,
         unsigned int uiIndiceGDT_DS,
         unsigned int uiIndiceGDT_TSS,
         unsigned int uiPosTSS,
         unsigned int uiDirBase,
         unsigned int uiLimite )
{
    int iN;
    int iJ;
    tms tmsDefault;
    itimerval itimervalDefault;
    
    tmsDefault.tms_utime = 0;
    tmsDefault.tms_stime = 0;
    tmsDefault.tms_cutime = 0;
    tmsDefault.tms_cstime = 0;
    itimervalDefault.it_interval.tv_sec = 0;
    itimervalDefault.it_interval.tv_usec = 0;
    itimervalDefault.it_value.tv_sec = 0;
    itimervalDefault.it_value.tv_usec = 0;

    pstuPCB[iPosicion].uiIndiceGDT_CS = uiIndiceGDT_CS;
    pstuPCB[iPosicion].uiIndiceGDT_DS = uiIndiceGDT_DS;
    pstuPCB[iPosicion].uiIndiceGDT_TSS = uiIndiceGDT_TSS;

    pstuPCB[iPosicion].vFnFuncion   = pEIP;
    pstuPCB[iPosicion].iEstado      = PROC_LISTO;    //Lista para ejecucion
    pstuPCB[iPosicion].ulLugarTSS   = uiPosTSS;
    pstuPCB[iPosicion].ulId         = uiUltimoPid++;
    pstuPCB[iPosicion].ulParentId   = pstuPCB[ulProcActual].ulId;
    pstuPCB[iPosicion].uiDirBase    = uiDirBase;
    pstuPCB[iPosicion].uiLimite     = uiLimite;
    pstuPCB[iPosicion].stuTmsTiemposProceso = tmsDefault;
    pstuPCB[iPosicion].timers[0]    = itimervalDefault;
    pstuPCB[iPosicion].timers[1]    = itimervalDefault;
    pstuPCB[iPosicion].timers[2]    = itimervalDefault;
    pstuPCB[iPosicion].lNanosleep   = 0;
    pstuPCB[iPosicion].puRestoDelNanosleep  = NULL;
    //TODO - Revisar cambio:
    //pstuPCB[iPosicion].uiTamProc    = 0;
    pstuPCB[iPosicion].uiTamProc    = uiLimite;
    pstuPCB[iPosicion].lPidTracer   = PROC_WOTRACER;

    for (iN = 0; iN < 12; iN++) {
        pstuPCB[iPosicion].stNombre[iN] = stNombre[iN];
    }

    pstuPCB[iPosicion].stNombre[iN] = '\0';
    
    /*Agregado por el grupo*/
    pstuPCB[iPosicion].iPrioridad = 0;
    
    pstuPCB[iPosicion].uiLimite     = uiLimite;

    //Inicializo el vector de memorias comartidas
    for (iJ = 0; iJ < MAXSHMEMPORPROCESO; iJ ++) {
        pstuPCB[iPosicion].memoriasAtachadas[iJ].utilizada = FALSE;
    }
    
    return iPosicion;
}


/**
\brief Crea una nueva TAREA_ESPECIAL (llamamos TAREA_ESPECIAL a los procesos 'simulados' por Sodium (Shell, Reloj y los lanzados con exec), aquellos que tienen su stack dentro de la TSS en espacio0, espacio1, etc).
\param puntero al codigo de la funcion que queremos ejecutar
\param nombre de la tarea
\returns posicion en la tabla de procesos
*/
int iFnNuevaTareaEspecial( void *pEIP, char *stNombre )
{
    unsigned short int bInterrupcionesHabilitadas = 0;
    unsigned long int *puliParametroStack;
    unsigned int uiIndiceGDT_TSS;
    int iPosicion;
    int iFlags;

    __asm__ ("pushf\n pop %%eax": "=a" (iFlags):);
    // si estaban habilitadas, aqui se deshabilitan
    if (iFlags & 0x200){
        __asm__ ("cli"::);
        bInterrupcionesHabilitadas = 1;
    }

    //Se crean todas las estructuras necesarias
    iPosicion = iFnBuscarPCBLibre();
    uiIndiceGDT_TSS = uiFnBuscarEntradaGDTLibre();
  
    //Si no hay entradas libres suficientes en la PCB (y TSS) o en la GDT,
    //liberamos los recursos tomados (las entradas en la GDT)
    if( iPosicion == -1 || !uiIndiceGDT_TSS ) {
            if(uiIndiceGDT_TSS) { mapa_gdt_set( uiIndiceGDT_TSS, 0 ); }
            return -EAGAIN;
    }


    /* TSS */

    if( iPosicion != iFnCrearTSSTareaEspecial( pEIP,
                                                iPosicion,
                                                wFnGetCS(),
                                                wFnGetDS(),
                                                wFnGetSS() ) ) {
        /* XXX 
         * Si llegamos aqui es porque no se pudo crear la TSS del proceso init.
         * Hoy en día iFnCrearTSS siempre retorna iPosicion, por lo que nunca
         * se alacanza este punto.
         */
    }

    // Se 'agrega' un descriptor en la GDT para que apunte a la nueva TSS
    uiFnAgregarDescriptorGDT (
            (unsigned int)&stuTSSTablaTareas[iPosicion],//Dir base del segmento
            sizeof( stuTSS ),                           //Longitud del segmento
            (D_TSS + D_BIG),                            //Opciones
            uiIndiceGDT_TSS);                           //Posicion en la GDT

    /* PCB */

    iFnCrearPCB( iPosicion,     /* PCB asignada */
             pEIP,              /* funcion a ejecutar */
             stNombre,          /* nombre del proceso */
             1,                 /* posicion del CS del kernel */
             2,                 /* posicion del DS del kernel */
             uiIndiceGDT_TSS,   /* indice del descript de la TSS en la GDT */
             iPosicion,         /* posicion de la TSS en la tabla de TSSs
                                   (igual a la de la tabla de PCBs) */
             0x000000,          /* base del segmento (usa el del kernel, por lo
                                   tanto, es cero) */
             0xffffffff         /* limite fisico (4gb) */ );


    //ESCRIBIMOS EN EL STACK SPACE (RING 0) DEL PROCESO CREADO EL CONTENIDO DE
    //LA VARIABLE PID QUE DICHO PROCESO TOMA COMO PARAMETRO
    puliParametroStack =
        (unsigned long int *)
        &( stuTSSTablaTareas[iPosicion].espacio0[TSS_TAMANIO_STACK_R0 - 4] );
    *puliParametroStack = (unsigned long int) pstuPCB[iPosicion].ulId;

    if (bInterrupcionesHabilitadas)
      __asm__ ("sti"::);

    return iPosicion;
}


/**
 * @brief Instancia el proceso INIT para programas de usuario
 * @returns La posicion dentro de la Tabla de PCBs
 */
int iFnInstanciarInit()
{
    return iFnCrearProceso(__init_begin(), __init_size(), "PR_Init()...");
}


/**
 * @brief Instancia el proceso Idle
 * @returns La posicion dentro de la Tabla de PCBs
 */
int iFnInstanciarIdle()
{
    //iTareaNula es variable GLOBAL
    iTareaNula = iFnCrearProceso(__idle_begin(), __idle_size(), "PR_Idle()...");
    return iTareaNula;
}


/**
 * @brief Crea un proceso
 * @returns La posicion dentro de la Tabla de PCBs
 */
int iFnCrearProceso(void* pvInicioBinario,
        unsigned long ulTamanioBinario,
        char* stNombreProceso)
{
    unsigned short int bInterrupcionesHabilitadas = 0;
    unsigned int uiIndiceGDT_CS, uiIndiceGDT_DS, uiIndiceGDT_TSS;
    unsigned int uiBaseSegmento;
    int iPosicion;
    int iFlags;

    __asm__ ("pushf\n pop %%eax": "=a" (iFlags):);
    // Si estaban habilitadas, aqui se deshabilitan
    if (iFlags & 0x200){
        __asm__ ("cli"::);
        bInterrupcionesHabilitadas = 1;
    }

    //Se reserva la memoria para el proceso (se usa un unico segmento para
    //Codigo y Datos)
    uiBaseSegmento = (unsigned int) pvFnReservarSegmento( SEGMENT_SIZE );
    
    if( uiBaseSegmento == NULL ) {
        return -ENOMEM;
    }

    //Se crean todas las estructuras necesarias
    iPosicion = iFnBuscarPCBLibre(); //Es la misma posicion para la TSS
    uiIndiceGDT_CS = uiFnBuscarEntradaGDTLibre();
    uiIndiceGDT_DS = uiFnBuscarEntradaGDTLibre();
    uiIndiceGDT_TSS = uiFnBuscarEntradaGDTLibre();

    //Si no hay entradas libres suficientes en la PCB (y TSS) o en la GDT,
    //liberamos los recursos tomados (la memoria y las entradas en la GDT)
    if( iPosicion == -1 ||
        !uiIndiceGDT_CS ||!uiIndiceGDT_DS || !uiIndiceGDT_TSS ) {
            vFnKFree( (void*)uiBaseSegmento );
            if(uiIndiceGDT_CS ) { mapa_gdt_set( uiIndiceGDT_CS,  0 ); }
            if(uiIndiceGDT_DS ) { mapa_gdt_set( uiIndiceGDT_DS,  0 ); }
            if(uiIndiceGDT_TSS) { mapa_gdt_set( uiIndiceGDT_TSS, 0 ); }
            return -EAGAIN;
    }

    /* Descriptor de CODIGO */

    // Se crea tomando como base el descriptor de CODIGO del Kernel
    ucpFnCopiarMemoria(
        (unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[uiIndiceGDT_CS], 
        (unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[wFnGetCS() / 8],
        sizeof( stuGDTDescriptor ) );
    /* Divido por 4k porque me estoy copiando el descriptor de CODIGO del kernel
     * que tiene granularidad 4Kb
     */
    uiFnSetearBaseLimiteDescriptor( uiIndiceGDT_CS, uiBaseSegmento,
                                    SEGMENT_SIZE / 4096 ); 

    /* Descriptor de DATOS */

    // Se crea tomando como base el descriptor de DATOS del Kernel
    ucpFnCopiarMemoria(
        (unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[uiIndiceGDT_DS], 
        (unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[wFnGetDS() / 8],
        sizeof( stuGDTDescriptor ) );
    /* Divido por 4k porque me estoy copiando el descriptor de DATOS del kernel
     * que tiene granularidad 4Kb
     */
    uiFnSetearBaseLimiteDescriptor( uiIndiceGDT_DS, uiBaseSegmento,
                                    SEGMENT_SIZE / 4096 );

    /* COPIA de CODIGO */ 

    vFnLog("\nCopiando el codigo del proceso \"%s\" de 0x%x a 0x%x, ocupa %d "
            "bytes", stNombreProceso, pvInicioBinario, uiBaseSegmento,
            ulTamanioBinario);
    ucpFnCopiarMemoria(
        (unsigned char*)uiBaseSegmento,
        (unsigned char*)pvInicioBinario,
        SEGMENT_SIZE );

    /* TSS */

    if( iPosicion != iFnCrearTSS( 0x0, /* arranca en la pos 0 del binario */
                                    (void*)(SEGMENT_SIZE - 0x10),
                                    iPosicion,
                                    uiIndiceGDT_CS * 8,
                                    uiIndiceGDT_DS * 8,
                                    uiIndiceGDT_DS * 8 ) ) {
        /* XXX 
         * Si llegamos aqui es porque no se pudo crear la TSS del proceso init.
         * Hoy en día iFnCrearTSS siempre retorna iPosicion, por lo que nunca
         * se alacanza este punto.
         */
    }

    // Se 'agrega' un descriptor en la GDT para que apunte a la nueva TSS
    uiFnAgregarDescriptorGDT (
            (unsigned int)&stuTSSTablaTareas[iPosicion],//Dir base del segmento
            sizeof( stuTSS ),                           //Longitud del segmento
            (D_TSS + D_BIG),                            //Opciones
            uiIndiceGDT_TSS);                           //Posicion en la GDT

    /* PCB */
    
    iFnCrearPCB( iPosicion,     /* PCB de init */
             0x0,               /* direccion de arranque de init */
             stNombreProceso,   /* nombre */
             uiIndiceGDT_CS,    /* Posicion del descrip CODIGO en la GDT */
             uiIndiceGDT_DS,    /* Posicion del descrip DATOS  en la GDT */
             uiIndiceGDT_TSS,   /* Posicion del descrip TSS    en la GDT */
             iPosicion,         /* posicion dentro de la tabla de TSSs */
             uiBaseSegmento,    /* dir base del segmento asignado a este proc */
             SEGMENT_SIZE       /* LONGITUD del segmento */
             );

    if (bInterrupcionesHabilitadas)
      __asm__ ("sti"::);

    return iPosicion;
}


/**
 * @brief Redimensiona un Proceso (cambia tamanios de segmentos de codigo y datos y actualiza las estructuras asodiadas (GDT, PCB, TSS, etc) )
 * @param uiPid Pid del proceso a realocar
 * @param ulBrk Nuevo tamanio para el proceso 
 * @returns Ejecucion correcta: 0. Error: -1
 */
int iFnRedimensionarProceso(unsigned long ulPid, unsigned long ulBrk) {
    unsigned long ulDirBaseNueva;
    stuPCB* pPCB;
    
    pPCB = &pstuPCB[ iFnBuscaPosicionProc(ulPid) ];
  
    //TODO - lala - comprobar que el nuevo ulBrk no sea inferior al minimo
    // permitido para este proceso (menor al area de codio [y stack?]) ni sea
    // mayor al maximo de memoria permitido para un proceso [hoy en dia no
    // existe tal cosa]

    //TODO - lala - cambiar realloc por una funcion que llame a realloc
    ulDirBaseNueva = (unsigned long) pvFnKRealloc(
                        (void *)pPCB->uiDirBase, ulBrk, MEM_ALTA | MEM_USUARIO);
  
    //Si no se pudo hacer el realloc, se deja el segmento como estaba y
    //devolvemos -1
    if(ulDirBaseNueva == NULL) {
        vFnLog("\niFnRedimensionarProceso: No se pudo redimensionar proceso");
        return -1;
    }
  
    //Actualizamos el PCB
    pPCB->uiDirBase = ulDirBaseNueva;
    pPCB->uiLimite  = ulBrk;
    pPCB->uiTamProc = ulBrk;
    uiFnSetearBaseLimiteDescriptor(pPCB->uiIndiceGDT_CS, ulDirBaseNueva, ulBrk);
    uiFnSetearBaseLimiteDescriptor(pPCB->uiIndiceGDT_DS, ulDirBaseNueva, ulBrk);
    
    vFnLog("\niFnRedimensionarProceso: Se redimensiono Proceso PID=%d", ulPid);
    return 0;
}


/**
 * @brief Duplica un Proceso
 * @param uiProcPadre La posicion dentro de la Tabla de PCBs del proceso padre
 * @returns La posicion dentro de la Tabla de PCBs del proceso creado
 */
int iFnDuplicarProceso( unsigned int uiProcPadre ){
    unsigned int uiIndiceGDT_CS,
             uiIndiceGDT_DS,
             uiIndiceGDT_TSS,
             uiStackUsuario,
             uiComienzoStackKernel,
             uiBaseSegmento;
    int iPosicion = 0;

    /* No deshabilitamos las interrupciones porque esta funcion se invoca desde
     * el syscall fork(), que a su vez es llamado por el handler de la int 0x80,
     * por lo cual al momento de comenzar la atencion de la interrupcion, se
     * deshabilitan las demas
     */

    //Se reserva la memoria para el proceso (se usa un unico segmento para
    //Codigo y Datos)
	uiBaseSegmento = (unsigned int) pvFnReservarSegmento( SEGMENT_SIZE );

    if( uiBaseSegmento == NULL ) {
        return -ENOMEM;
    }

    //Se crean todas las estructuras necesarias
    iPosicion = iFnBuscarPCBLibre(); //Es la misma posicion para la TSS
    uiIndiceGDT_CS = uiFnBuscarEntradaGDTLibre();
    uiIndiceGDT_DS = uiFnBuscarEntradaGDTLibre();
    uiIndiceGDT_TSS = uiFnBuscarEntradaGDTLibre();

    //Si no hay entradas libres suficientes en la PCB (y TSS) o en la GDT,
    //liberamos los recursos tomados (la memoria y las entradas en la GDT)
    if( iPosicion == -1 ||
        !uiIndiceGDT_CS ||!uiIndiceGDT_DS || !uiIndiceGDT_TSS ) {
            vFnKFree( (void*)uiBaseSegmento );
            if(uiIndiceGDT_CS ) { mapa_gdt_set( uiIndiceGDT_CS,  0 ); }
            if(uiIndiceGDT_DS ) { mapa_gdt_set( uiIndiceGDT_DS,  0 ); }
            if(uiIndiceGDT_TSS) { mapa_gdt_set( uiIndiceGDT_TSS, 0 ); }
            return -EAGAIN;
    }

#if 1
    vFnLog( "\n fork(): iFnBuscarPCBLibre: %d", iPosicion );
    vFnLog( "\n fork(): Inicio nuevo segmento: 0x%x", uiBaseSegmento );
    vFnLog( "\n fork(): uiFnBuscarEntradaGDTLibre(CS): %d, 0x%x",
            uiIndiceGDT_CS, uiIndiceGDT_CS * 8 );
    vFnLog( "\n fork(): uiFnBuscarEntradaGDTLibre(DS): %d, 0x%x",
            uiIndiceGDT_DS, uiIndiceGDT_DS * 8 );
    vFnLog( "\n fork(): uiFnBuscarEntradaGDTLibre(TSS): %d, 0x%x",
            uiIndiceGDT_TSS, uiIndiceGDT_TSS * 8 );
#endif

    /* Descriptor de CODIGO */

    ucpFnCopiarMemoria(
        (unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[uiIndiceGDT_CS], 
        (unsigned char*) &pstuTablaGdt->
                stuGdtDescriptorDescs[stuTSSTablaTareas[uiProcPadre].cs / 8],
        sizeof( stuGDTDescriptor ) );
    /* Divido por 4k porque me estoy copiando el descriptor de CODIGO del kernel
     * que tiene granularidad 4Kb
     */
    uiFnSetearBaseLimiteDescriptor( uiIndiceGDT_CS, uiBaseSegmento,
                                    SEGMENT_SIZE / 4096 ); 

    /* Descriptor de DATOS */

    ucpFnCopiarMemoria(
        (unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[uiIndiceGDT_DS], 
        (unsigned char*) &pstuTablaGdt->
                stuGdtDescriptorDescs[stuTSSTablaTareas[uiProcPadre].ds / 8],
        sizeof( stuGDTDescriptor ) );
    /* Divido por 4k porque me estoy copiando el descriptor de DATOS del kernel
     * que tiene granularidad 4Kb
     */
    uiFnSetearBaseLimiteDescriptor( uiIndiceGDT_DS, uiBaseSegmento,
                                    SEGMENT_SIZE / 4096 );

    /* Copio todo el segmento del proc actual al nuevo */
    ucpFnCopiarMemoria( (unsigned char*)uiBaseSegmento,
                        (unsigned char*)pstuPCB[uiProcPadre].uiDirBase, 
                        //TODO - lala Cambiar esto:
                        SEGMENT_SIZE );

    /* TSS */

    /* Armamos la TSS del proceso hijo */
    stuTSSTablaTareas[iPosicion].trapbit = 0;
    stuTSSTablaTareas[iPosicion].uIOMapeoBase = sizeof (stuTSS);

    // Le asignamos los selectores de segmento que encontramos con
    // uiFnBuscarEntradaGDTLibre()
    stuTSSTablaTareas[iPosicion].cs = uiIndiceGDT_CS * 8;
    stuTSSTablaTareas[iPosicion].ds = uiIndiceGDT_DS * 8;
    stuTSSTablaTareas[iPosicion].ss = uiIndiceGDT_DS * 8;
    stuTSSTablaTareas[iPosicion].es = uiIndiceGDT_DS * 8;
    stuTSSTablaTareas[iPosicion].fs = uiIndiceGDT_DS * 8;
    stuTSSTablaTareas[iPosicion].gs = uiIndiceGDT_DS * 8;

    stuTSSTablaTareas[iPosicion].ss0 = 0x00;
    stuTSSTablaTareas[iPosicion].ss1 = 0x00;
    stuTSSTablaTareas[iPosicion].ss2 = 0x00;
    
    stuTSSTablaTareas[iPosicion].esp0 = (unsigned int) 0;
    stuTSSTablaTareas[iPosicion].esp1 = (unsigned int) 0;
    stuTSSTablaTareas[iPosicion].esp2 = (unsigned int) 0;
   
 
    /* ahora comenzamos a copiar los valores de los registros, sacandolos de lo
     * que se fue apilando al momento de comenzar el manejo de la syscall */
    /* el comienzo del stack del kernel se calcula en **system_asm.asm**,
     * cualquier cambio ahi (que implique desplazar el stack), debe contemplarse
     * aqui ya que este calculo resultaria invalido */
    uiComienzoStackKernel = 0x200000 - 0x08 - 0x100 *
        ((pstuPCB[uiProcPadre].uiIndiceGDT_TSS * 8) - 0x20);

    stuTSSTablaTareas[iPosicion].esp =
        *(unsigned int*)(uiComienzoStackKernel-0x08);
    stuTSSTablaTareas[iPosicion].ebp =
        *(unsigned int*)(uiComienzoStackKernel-0x0c);

    stuTSSTablaTareas[iPosicion].edx =
        *(unsigned int*)(uiComienzoStackKernel-0x10);
    stuTSSTablaTareas[iPosicion].ecx =
        *(unsigned int*)(uiComienzoStackKernel-0x14);
    stuTSSTablaTareas[iPosicion].ebx =
        *(unsigned int*)(uiComienzoStackKernel-0x18);
    stuTSSTablaTareas[iPosicion].eax = 0x00; // al hijo se le devuelve cero!!

    /* Calculamos la posicion del fin stack del usuario, tomando la base del
     * segmento del proceso (de su PCB) y el valor de ESP (apilado en el stack
     * del kernel)
     */
    uiStackUsuario = pstuPCB[uiProcPadre].uiDirBase 
                      + stuTSSTablaTareas[iPosicion].esp;
    
    stuTSSTablaTareas[iPosicion].edi = *(unsigned int*)(uiStackUsuario+0x04);
    stuTSSTablaTareas[iPosicion].esi = *(unsigned int*)(uiStackUsuario+0x08);
    stuTSSTablaTareas[iPosicion].eip = *(unsigned int*)(uiStackUsuario+0x1c);
    stuTSSTablaTareas[iPosicion].uiEBandera =
        *(unsigned int*)(uiStackUsuario+0x24);

    stuTSSTablaTareas[iPosicion].ldt = stuTSSTablaTareas[uiProcPadre].ldt;

    
    /* Si el ultimo proceso en usar la FPU es el padre del nuevo proceso,
     * su TSS estara desactualizada
     * y los valores habra que sacarlos directamente de la FPU */
    if(ulUltimoProcesoEnFPU == pstuPCB[uiProcPadre].ulId) {
          //Se guarda el estado actual de la FPU, en la TSS del proceso hijo
          asm volatile(
              "fnsave %0\n" //guarda en la TSS del hijo y reinicializa el FPU
              "fwait    \n" 
              "frstor %0\n" //vuelve a cargar el FPU con sus valores anteriores
              : "=m" (stuTSSTablaTareas[iPosicion].fpu));
    } else {
        //La TSS del padre esta actualizada
        //asi que tomamos los valores de la misma
        ucpFnCopiarMemoria( (unsigned char*)&stuTSSTablaTareas[iPosicion].fpu,
                            (unsigned char*)&stuTSSTablaTareas[uiProcPadre].fpu,
                            sizeof(stuFpu));
    }

    uiFnAgregarDescriptorGDT ((unsigned int)
                    &stuTSSTablaTareas[iPosicion], sizeof( stuTSS ),
                    (D_TSS + D_BIG), uiIndiceGDT_TSS);
   
 
#if 0
vFnImprimir( "\n fork(): EIP padre (de la TSS): %x", stuTSSTablaTareas[uiProcPadre].eip);
vFnImprimir( "\n fork(): EIP padre (del stack): %x", stuTSSTablaTareas[iPosicion].eip);
vFnImprimir( "\n fork(): CS padre (del stack): %x", *(unsigned int*)(uiStackUsuario+0x20));
vFnImprimir( "\n fork(): EFLAGS padre (del stack): %x", *(unsigned int*)(uiStackUsuario+0x24));

vFnImprimir( "\n fork(): TSS padre: %x", pstuPCB[uiProcPadre].uiIndiceGDT * 8);
vFnImprimir( "\n fork(): comienzo stack kernel para esta instancia: %x", uiComienzoStackKernel );
vFnImprimir( "\n fork(): fin stack usuario para esta instancia: %x", uiStackUsuario );
vFnImprimir( "\n fork(): ESP original: %x", stuTSSTablaTareas[iPosicion].esp );
vFnImprimir( "\n fork(): EBP original: %x", stuTSSTablaTareas[iPosicion].ebp );

vFnImprimir( "\n fork(): EDX original: %x", stuTSSTablaTareas[iPosicion].edx );
vFnImprimir( "\n fork(): ECX original: %x", stuTSSTablaTareas[iPosicion].ecx );
vFnImprimir( "\n fork(): EBX original: %x", stuTSSTablaTareas[iPosicion].ebx );
vFnImprimir( "\n fork(): EAX (modificado): %x", stuTSSTablaTareas[iPosicion].eax );
#endif
    
    /* Ajustamos el stack en el hijo para que quede como estaba en el padre
     * antes de ejecutar el syscall fork() (que provoca que se utilizen 0x28
     * bytes en el stack entre lo que pushea el micro y lo que pusheamos
     * nosotros en system_asm.asm)
     */
    stuTSSTablaTareas[iPosicion].esp += 0x28;  

    /* PCB */

    iFnCrearPCB( iPosicion,     /* PCB asignado al nuevo proceso */
             0,                 /* no interesa guardar punteros en la PCB */
             pstuPCB[uiProcPadre].stNombre,     /* nombre del proceso padre */
             uiIndiceGDT_CS,    /* Posicion del descrip CODIGO en la GDT */
             uiIndiceGDT_DS,    /* Posicion del descrip DATOS  en la GDT */
             uiIndiceGDT_TSS,   /* Posicion del descrip TSS    en la GDT */
             iPosicion,         /* posicion dentro de la tabla de TSSs */
             uiBaseSegmento,    /* base del segmento asignado*/
             SEGMENT_SIZE       /* LONGITUD del segmento */
             );


    // Finalmente, el proceso PADRE tiene un hijo mas!
    ++pstuPCB[uiProcPadre].lNHijos;    

    return iPosicion;
}


/**
 * @brief Reemplaza el CODIGO del proceso indicado y ejecuta desde "cero"
 * @param Posicion del proceso dentro de la tabla de PCBs
 * @param Direccion lineal donde se ubica el codigo a copiar
 * @param Cantidad de bytes de codigo a copiar
 * @returns La posicion dentro de la tabla de PCBs del proceso modificado
 */
int iFnReemplazarProceso(
        unsigned int uiProceso,
        unsigned char *ucpCodigo,
        unsigned int uiTamanio ) {
    
    unsigned int uiESPOriginal,
             uiStackUsuario,
             uiComienzoStackKernel;

    /* No deshabilitamos las interrupciones porque esta funcion se invoca desde
     * el syscall execve(), que a su vez es llamado por el handler de la int
     * 0x80, por lo cual al momento de comenzar la atencion de la interrupcion,
     * se deshabilitan las demas
     */
    
    /* No buscamos un segmento libre, ni buscamos entradas disponibles en la GDT     * porque lo unico que tenemos q realizar es reemplazar el codigo del
     * proceso indicado, y resetear el estado, para arrancar este nuevo programa
     * desde "cero" */

    /* Copio todo el segmento de codigo al segmento del proceso actual */
    ucpFnCopiarMemoria( (unsigned char*)pstuPCB[uiProceso].uiDirBase,
                        ucpCodigo,
                        uiTamanio );


    /* TODO - REVISAR
     * Que pasa si el codigo a copiar no entra en el segmento actual del proceso
     * a modificar?
     */


    /* La TSS no se altera; no tiene sentido alterar la TSS del proceso, ya que
     * en el momento que se haga context-switch a otro proceso, lo que este
     * ejecutando en ese momento es lo que va a quedar realmente.
     * Todo lo que pongamos en la TSS aqui se pierde...
     *
     * La estrategia consiste en reemplazar los valores en el stack, tanto la
     * parte que esta en el kernel como la del usuario, de manera que al volver
     * del syscall, no vuelva a la proxima instruccion despues de la invocacion
     * a la int0x80, sino al arranque del proceso nuevo (o sea, tampoco se toca
     * la PCB)
     */
    
    /* Ahora comenzamos a copiar los valores de los registros, sacandolos de lo
     * que se fue apilando al momento de comenzar el manejo de la syscall
     */
    /* El comienzo del stack del kernel se calcula en **system_asm.asm**,
     * cualquier cambio ahi (que implique desplazar el stack), debe contemplarse
     * aqui ya que este calculo resultaria invalido
     */
    uiComienzoStackKernel = 0x200000 - 0x08 - 0x100 
                * ((pstuPCB[uiProceso].uiIndiceGDT_TSS * 8)-0x20);
    
    /* Extraemos del stack del kernel, el ESP original: */
    uiESPOriginal = *(unsigned int*)(uiComienzoStackKernel-0x08);

    /* Calculamos la posicion del fin stack del usuario, tomando la base del
     * segmento del proceso (de su PCB) y el valor de ESP (apilado en el stack
     * del kernel)
     */
    uiStackUsuario = pstuPCB[uiProceso].uiDirBase + uiESPOriginal;
    
    //*(unsigned int*)(uiComienzoStackKernel-0x08) = SEGMENT_SIZE - 0x10; // ESP
    //*(unsigned int*)(uiComienzoStackKernel-0x0c) = SEGMENT_SIZE - 0x10; // EBP

    *(unsigned int*)(uiComienzoStackKernel-0x10) = 0x00; /* EDX */
    *(unsigned int*)(uiComienzoStackKernel-0x14) = 0x00; /* ECX */
    *(unsigned int*)(uiComienzoStackKernel-0x18) = 0x00; /* EBX */
    *(unsigned int*)(uiComienzoStackKernel-0x18) = 0x00; /* EAX */ 

    *(unsigned int*)(uiStackUsuario+0x04) = 0x00;   /* EDI */ 
    *(unsigned int*)(uiStackUsuario+0x08) = 0x00;   /* ESI */
    *(unsigned int*)(uiStackUsuario+0x1c) = 0x00;   /* EIP */
    *(unsigned int*)(uiStackUsuario+0x24) = 0x202L; /* EFLAGS */

    //TODO - Limpiar valores de FPU

    return (int) uiProceso;
}


/**
 * @brief Elimina el proceso indicado (PCB, segmentos y entradas en la GDT)
 * @param Posicion del proceso dentro de la tabla de PCBs
 * @returns La posicion dentro de la tabla de PCBs del proceso eliminado
 */
int iFnEliminarProceso( unsigned int uiProceso ) {
    /* Marcamos el segmento que ocupaba como disponible: */
/*    vFnImprimir( "\n waitpid(): Liberando el segmento %d, base %x",
             pstuPCB[ uiProceso ].uiDirBase / SEGMENT_SIZE,
             pstuPCB[ uiProceso ].uiDirBase );*/
/* CHAU    mapa_segmentos_set( pstuPCB[ uiProceso ].uiDirBase / SEGMENT_SIZE, 0 );
*/
    //TODO - lala - SACAR!
//vFnLiberarSegmento( (void *)pstuPCB[ uiProceso ].uiDirBase,
//                    pstuPCB[ uiProceso ].uiLimite);
    vFnKFree( (void *)pstuPCB[ uiProceso ].uiDirBase );

    /* Liberamos las 3 entradas de la GDT reservadas para el descriptor
     * de segmento de codigo, datos y TSS: */
    vFnLog( "\n iFnEliminarProceso(): Liberando entradas %d, %d y %d de la GDT",
             pstuPCB[ uiProceso ].uiIndiceGDT_CS,
             pstuPCB[ uiProceso ].uiIndiceGDT_DS,
             pstuPCB[ uiProceso ].uiIndiceGDT_TSS );
    mapa_gdt_set( pstuPCB[ uiProceso ].uiIndiceGDT_CS,  0 );
    mapa_gdt_set( pstuPCB[ uiProceso ].uiIndiceGDT_DS,  0 );
    mapa_gdt_set( pstuPCB[ uiProceso ].uiIndiceGDT_TSS, 0 );

    /* Liberamos la PCB que utilizaba (y por consiguiente, la entrada en la
     * tabla de TSS */
    vFnLog( "\n iFnEliminarProceso(): Eliminando PCB %d (PID %d)", uiProceso,
             pstuPCB[ uiProceso ].ulId );
    pstuPCB[ uiProceso ].iEstado = PROC_ELIMINADO;

    return uiProceso;
}


/* Agregado por el grupo */
/**
 * @brief Crea un nuevo proceso, pero compartiendo el segmento de datos y codigo
 * del proceso que lo llamo
 *
 * @returns 0 si fue exitoso, distinto de 0 si hubo error
 */
int iFnClonarProceso() {
    unsigned int uiIndiceGDT_TSS;
    unsigned int uiIndiceGDT_DS;
    unsigned int uiBaseSegmento;
    int iPosicion;
    unsigned int uiComienzoStackKernel, uiStackUsuario;
    
    //TODO - Revisar donde se usa esta funcion, tiene apariencia dudosa (anda?)

    if(!uiIndiceGDT_TSS) {
        vFnImprimir("ENOMEM\n");
        return -ENOMEM;
    }

    iPosicion = iFnBuscarPCBLibre();
/* CHAU    uiBaseSegmento = uiFnBuscarSegmentoLibre() * SEGMENT_SIZE;*/
    uiBaseSegmento = (unsigned int) pvFnReservarSegmento( SEGMENT_SIZE );

    uiIndiceGDT_DS = uiFnBuscarEntradaGDTLibre();
    uiIndiceGDT_TSS = uiFnBuscarEntradaGDTLibre();
    //vFnImprimir("Indice GDT: %d\n",uiIndiceGDT_TSS);

    ucpFnCopiarMemoria(
        (unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[uiIndiceGDT_DS], 
        (unsigned char*) &pstuTablaGdt->
                stuGdtDescriptorDescs[stuTSSTablaTareas[ulProcActual].ds / 8],
        sizeof( stuGDTDescriptor ) );
    uiFnSetearBaseLimiteDescriptor( uiIndiceGDT_DS, uiBaseSegmento,
                                    SEGMENT_SIZE / 4096 );
    //vFnImprimir("Copio ds\n");

    ucpFnCopiarMemoria( (unsigned char*)uiBaseSegmento, 
                        (unsigned char*)pstuPCB[ulProcActual].uiDirBase, 
                        SEGMENT_SIZE );
    //vFnImprimir("Copio segmento\n");

    stuTSSTablaTareas[iPosicion].trapbit = 0;
    stuTSSTablaTareas[iPosicion].uIOMapeoBase = sizeof(stuTSS);

    // Ver diferencia con iFnDuplicarProceso
    stuTSSTablaTareas[iPosicion].cs = stuTSSTablaTareas[ulProcActual].cs;

    stuTSSTablaTareas[iPosicion].ds = uiIndiceGDT_DS * 8;
    stuTSSTablaTareas[iPosicion].ss = uiIndiceGDT_DS * 8;
    stuTSSTablaTareas[iPosicion].es = uiIndiceGDT_DS * 8;
    stuTSSTablaTareas[iPosicion].fs = uiIndiceGDT_DS * 8;
    stuTSSTablaTareas[iPosicion].gs = uiIndiceGDT_DS * 8;

    stuTSSTablaTareas[iPosicion].ss0 = 0;
    stuTSSTablaTareas[iPosicion].ss1 = 0;
    stuTSSTablaTareas[iPosicion].ss2 = 0;
    stuTSSTablaTareas[iPosicion].esp0 = 0;
    stuTSSTablaTareas[iPosicion].esp1 = 0;
    stuTSSTablaTareas[iPosicion].esp2 = 0;
    /*vFnImprimir("ds padre %x ds hijo %x\n",
                stuTSSTablaTareas[ulProcActual].ds,
                stuTSSTablaTareas[iPosicion].ds);*/
    vFnImprimir("\nVerificacion de Clone\n CS padre: %x CS hijo: %x\n",
                stuTSSTablaTareas[ulProcActual].cs,
                stuTSSTablaTareas[iPosicion].cs );

    uiComienzoStackKernel = 0x200000 - 0x08 - 0x100 *
        ((pstuPCB[ulProcActual].uiIndiceGDT_TSS * 8)-0x20);

    stuTSSTablaTareas[iPosicion].esp =
        *(unsigned int*)(uiComienzoStackKernel-0x08);
    stuTSSTablaTareas[iPosicion].ebp =
        *(unsigned int*)(uiComienzoStackKernel-0x0c);

    stuTSSTablaTareas[iPosicion].edx =
        *(unsigned int*)(uiComienzoStackKernel-0x10);
    stuTSSTablaTareas[iPosicion].ecx =
        *(unsigned int*)(uiComienzoStackKernel-0x14);
    stuTSSTablaTareas[iPosicion].ebx =
        *(unsigned int*)(uiComienzoStackKernel-0x18);
    stuTSSTablaTareas[iPosicion].eax = 0x00;


    uiStackUsuario = pstuPCB[ulProcActual].uiDirBase 
                      + stuTSSTablaTareas[iPosicion].esp;
    
    stuTSSTablaTareas[iPosicion].edi = *(unsigned int*)(uiStackUsuario+0x04);
    stuTSSTablaTareas[iPosicion].esi = *(unsigned int*)(uiStackUsuario+0x08);
    stuTSSTablaTareas[iPosicion].eip = *(unsigned int*)(uiStackUsuario+0x1c);
    stuTSSTablaTareas[iPosicion].uiEBandera =
        *(unsigned int*)(uiStackUsuario+0x24);

    stuTSSTablaTareas[iPosicion].ldt = 0; //stuTSSTablaTareas[ulProcActual].ldt;

    
    /* Si el ultimo proceso en usar la FPU es el padre del nuevo proceso,
     * su TSS estara desactualizada
     * y los valores habra que sacarlos directamente de la FPU */
    if(ulUltimoProcesoEnFPU == pstuPCB[ulProcActual].ulId) {
      //Se guarda el estado actual de la FPU, en la TSS del proceso hijo
      asm volatile(
              "fnsave %0\n" //guarda en la TSS del hijo y reinicializa el FPU
              "fwait    \n" 
              "frstor %0\n" //vuelve a cargar el FPU con sus valores anteriores
              : "=m" (stuTSSTablaTareas[iPosicion].fpu));
    } else {
        //La TSS del padre esta actualizada
        //asi que tomamos los valores de la misma
        ucpFnCopiarMemoria((unsigned char*)&stuTSSTablaTareas[iPosicion].fpu,
                           (unsigned char*)&stuTSSTablaTareas[ulProcActual].fpu,
                           sizeof(stuFpu));
    }
    
    uiFnAgregarDescriptorGDT ((unsigned int)
                    &stuTSSTablaTareas[iPosicion], sizeof( stuTSS ),
                    (D_TSS + D_BIG), uiIndiceGDT_TSS);
    
    ++pstuPCB[ulProcActual].lNHijos;    

    stuTSSTablaTareas[iPosicion].esp += 0x28;
            
    iFnCrearPCB(iPosicion, 
            0,
            pstuPCB[ulProcActual].stNombre,
            pstuPCB[ulProcActual].uiIndiceGDT_CS,
            pstuPCB[ulProcActual].uiIndiceGDT_DS,
            uiIndiceGDT_TSS,
            iPosicion,
            uiBaseSegmento,
            SEGMENT_SIZE);

    return 0;
}

