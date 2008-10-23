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

#include <fs/ramfs.h>

#include <kernel/mem/memoria_s.h>

extern dword pdwGDT;
extern stuPCB pstuPCB[CANTMAXPROCS];
extern stuTSS stuTSSTablaTareas[CANTMAXPROCS];

unsigned int uiUltimoPid = 0;
unsigned long ulProcActual = 0;
unsigned long ulUltimoProcesoEnFPU = 0; //PID del ultimo proceso que uso el FPU

unsigned char iMapaGDT[ TOTAL_ENTRADAS_GDT / 8 ]; 


#define MAX_MEMORIA_PROCESO 0x4000000   // 64 Mb
#define STACK_SIZE      0x01000 // 4 Kb - TODO: Revisar si es un valor aceptable


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

#define EN_GRANULARIDAD_4K( x ) ( ((x) + 0x0FFF) >> 12 )
#define EN_GRANULARIDAD_1B( x ) ( (x) << 12 )

#define REDONDEAR_HACIA_ARRIBA_A_4K( x ) \
                ( EN_GRANULARIDAD_1B( EN_GRANULARIDAD_4K (x) ) )


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
\brief Carga en el descriptor de la GDT indicado la direccion de memoria base y el limite (sin especificar granularidad)
\param uiPosicion Posicion en la GDT del descriptor a modificar
\param uiBase Direccion base (lineal)
\param uiLimite Longitud (afectado por la granularidad)
\returns posicion en la GDT del descriptor
\note Tener en cuenta que, el largo del segmento descripto sera ( (uiLimite+1) * GR). Por ejemplo, con granularidad 4Kb un uiLimite = 0 determina un largo de segmento descripto de ( (0+1) * 4Kb) = 4Kb.
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
        (uiOpt | D_PRESENT) >> 8;
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
         unsigned int uiLimite,
         unsigned int uiTamanioTexto,
         unsigned int uiTamanioDatosInicializados,
         unsigned int uiTamanioStack)
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

    for (iN = 0; iN < 11; iN++) {
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
   
    //Agregado 2008 - GRUPO SEGMENTACION PURA
    pstuPCB[iPosicion].uiTamanioTexto = uiTamanioTexto;
    pstuPCB[iPosicion].uiTamanioDatosInicializados=uiTamanioDatosInicializados;
    pstuPCB[iPosicion].uiTamanioStack = uiTamanioStack;
    //Fin Agregado 2008

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
            (D_TSS | D_BIG),                            //Opciones
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
             0xffffffff,        /* limite fisico (4gb) */
             0,                 /* Tamanio del codigo*/
             0,                 /* Tamanio del bloque de datos inicializados*/
             0                  /* Tamanio del Stack*/
             );


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
    return iFnCrearProceso("INIT.BIN");
}


/**
 * @brief Instancia el proceso Idle
 * @returns La posicion dentro de la Tabla de PCBs
 */
int iFnInstanciarIdle()
{
    iTareaNula = iFnCrearProceso("IDLE.BIN");
    return iTareaNula;
}


/**
\brief Crea un proceso a partir de un archivo binario
\param stArchivo Nombre del archivo binario con el cual crear el proceso
\returns Si hubo exito, la posicion dentro de la Tabla de PCBs, si no, un numero menor a 0
*/
int iFnCrearProceso(
        /*
        void* pvInicioBinario,
        unsigned long ulTamanioBinario,
        char* stNombreProceso
        */
        char* stArchivo)
{
    unsigned short int bInterrupcionesHabilitadas = 0;
    unsigned int uiIndiceGDT_CS, uiIndiceGDT_DS, uiIndiceGDT_TSS;
    unsigned int uiBaseSegmento;
    int iPosicion;
    int iFlags;

    unsigned int uiTamSegCodigo;
    unsigned int uiTamSegDatos;
    unsigned int uiTamStack;
    unsigned int uiTamInicializados;

    stuInfoEjecutable stuInfoExe;


    __asm__ ("pushf\n pop %%eax": "=a" (iFlags):);
    // Si estaban habilitadas, aqui se deshabilitan
    if (iFlags & 0x200){
        __asm__ ("cli"::);
        bInterrupcionesHabilitadas = 1;
    }

    //Se lee la cabecera del archivo binario
    if( iFnLeerCabeceraEjecutable(stArchivo, &stuInfoExe) == -1) {
        return -1;
    }
  
    //Vamos a crear descriptores con granularidad 4K, por ello convertimos los
    //datos leidos del binario a granularidad 4K
    uiTamSegCodigo     = REDONDEAR_HACIA_ARRIBA_A_4K(stuInfoExe.uiTamanioTexto);
    uiTamStack         = REDONDEAR_HACIA_ARRIBA_A_4K(stuInfoExe.uiTamanioStack);
    uiTamInicializados = REDONDEAR_HACIA_ARRIBA_A_4K(stuInfoExe.uiTamanioDatosInicializados);
    uiTamSegDatos      = REDONDEAR_HACIA_ARRIBA_A_4K(uiTamSegCodigo + uiTamStack + uiTamInicializados); //Se crea sin Heap

    //Se reserva la memoria para el proceso (se usa un unico segmento para
    //Codigo y Datos)
    uiBaseSegmento = (unsigned int) pvFnReservarSegmento( uiTamSegDatos );
    
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
    uiFnAgregarDescriptorGDT (
            uiBaseSegmento,                         //Dir base del segmento
            EN_GRANULARIDAD_4K(uiTamSegCodigo) - 1, //Longitud del segmento
            (D_CODE | D_READ | D_BIG | D_4KB),      //Opciones
            uiIndiceGDT_CS);                        //Posicion en la GDT

    /* Descriptor de DATOS */
    uiFnAgregarDescriptorGDT (
            uiBaseSegmento,                         //Dir base del segmento
            EN_GRANULARIDAD_4K(uiTamSegDatos) - 1,  //Longitud del segmento
            (D_DATA | D_WRITE | D_BIG | D_4KB),     //Opciones
            uiIndiceGDT_DS);                        //Posicion en la GDT

    /* COPIA de CODIGO */ 

    vFnLog("\nCopiando el codigo del proceso \"%s\" de 0x%x a 0x%x, ocupa %d "
            "bytes", stuInfoExe.stNombre, stuInfoExe.pvPuntoCarga,
            uiBaseSegmento, stuInfoExe.uiTamanioTexto);
    ucpFnCopiarMemoria(
        (unsigned char*)uiBaseSegmento,
        (unsigned char*)stuInfoExe.pvPuntoCarga,
        stuInfoExe.uiTamanioTexto );

    /* TSS */

    if( iPosicion != iFnCrearTSS( 0x0, /* arranca en la pos 0 del binario */
                                    (void*)(uiTamSegDatos - 0x10),
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
            (D_TSS | D_BIG),                            //Opciones
            uiIndiceGDT_TSS);                           //Posicion en la GDT

    /* PCB */
    
    iFnCrearPCB( iPosicion,     /* PCB de init */
             0x0,               /* direccion de arranque de init */
             stuInfoExe.stNombre,   /* nombre */
             uiIndiceGDT_CS,    /* Posicion del descrip CODIGO en la GDT */
             uiIndiceGDT_DS,    /* Posicion del descrip DATOS  en la GDT */
             uiIndiceGDT_TSS,   /* Posicion del descrip TSS    en la GDT */
             iPosicion,         /* posicion dentro de la tabla de TSSs */
             uiBaseSegmento,    /* dir base del segmento asignado a este proc */
             uiTamSegDatos,     /* LONGITUD del segmento */
             uiTamSegCodigo,    /* Tamanio del codigo */
             uiTamInicializados,/* Tamanio del bloque de datos inicializados */
             uiTamStack         /* Tamanio del Stack */
             );

    if (bInterrupcionesHabilitadas)
      __asm__ ("sti"::);

    return iPosicion;
}


/**
 * @brief Redimensiona un Proceso (cambia tamanio y posicion de segmento de datos, posicion de segmento de codigo y actualiza las estructuras asodiadas (GDT, PCB, TSS, etc) )
 * @param uiPid Pid del proceso a realocar
 * @param ulBrk Nuevo tamanio para el proceso 
 * @returns Ejecucion correcta: 0. Error: -1
 */
int iFnRedimensionarProceso(unsigned long ulPid, unsigned long ulBrk) {
    unsigned long ulDirBaseNueva;
    stuPCB* pPCB;
    
    pPCB = &pstuPCB[ iFnBuscaPosicionProc(ulPid) ];

    //Convertimos la direccion de BRK en multiplo de 4Kb, ya que usamos los
    //descriptores para los procesos con granularidad 4Kb
    ulBrk = REDONDEAR_HACIA_ARRIBA_A_4K(ulBrk);
 
    
    /* Comprobamos que el nuevo ulBrk no sea inferior al minimo permitido para
     * este proceso (menor al area de codigo, datos y stack), ni sea menor que
     * la direccion maxima de las memorias compartidas que tenga en este momento
     */ 
    if( ulBrk <= (pPCB->uiTamanioTexto +
                  pPCB->uiTamanioDatosInicializados +
                  pPCB->uiTamanioStack) ||
        ulBrk <= ulFnMaxDirShmProc(ulPid) ) {
        vFnLog("\niFnRedimensionarProceso: No se puede redimensionar proceso, "
                "BRK invalido (valor muy bajo)");
        return -1;
    }

    /* Comprobamos tambien que el nuevo ulBrk no supere el maximo de memoria
     * permitida para un proceso
     */
    if( ulBrk >= MAX_MEMORIA_PROCESO) {
        vFnLog("\niFnRedimensionarProceso: No se puede redimensionar proceso, "
                "BRK invalido (valor mayor al permitido por el sistema)");
        return -1;
    }


    ulDirBaseNueva = (unsigned long) pvFnRedimensionarSegmento(
                                                (void *)pPCB->uiDirBase, ulBrk);
  
    //Si no se pudo hacer el redimensionamiento, se deja el segmento como estaba
    //y devolvemos -1
    if(ulDirBaseNueva == NULL) {
        vFnLog("\niFnRedimensionarProceso: No se pudo redimensionar proceso");
        return -1;
    }
  
    //Actualizamos el PCB 
    pPCB->uiDirBase = ulDirBaseNueva;
    pPCB->uiLimite  = ulBrk;
    pPCB->uiTamProc = ulBrk;
    
    //Actualizamos los descriptores de la GDT (granularidad 4Kb)
    uiFnSetearBaseLimiteDescriptor(pPCB->uiIndiceGDT_CS, ulDirBaseNueva,
            EN_GRANULARIDAD_4K(ulBrk) - 1 );
    uiFnSetearBaseLimiteDescriptor(pPCB->uiIndiceGDT_DS, ulDirBaseNueva,
            EN_GRANULARIDAD_4K(ulBrk) - 1 );
   
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

    unsigned int uiTamSegCodigo;
    unsigned int uiTamSegDatos;
    unsigned int uiTamStack;
    unsigned int uiTamInicializados;
  
    uiTamSegCodigo = pstuPCB[uiProcPadre].uiTamanioTexto;
    uiTamSegDatos = pstuPCB[uiProcPadre].uiLimite;
    uiTamStack = pstuPCB[uiProcPadre].uiTamanioStack;
    uiTamInicializados = pstuPCB[uiProcPadre].uiTamanioDatosInicializados;

    int iPosicion;

    /* No deshabilitamos las interrupciones porque esta funcion se invoca desde
     * el syscall fork(), que a su vez es llamado por el handler de la int 0x80,
     * por lo cual al momento de comenzar la atencion de la interrupcion, se
     * deshabilitan las demas
     */

    //Se reserva la memoria para el proceso nuevo (se usa un unico segmento para
    //Codigo y Datos)
	uiBaseSegmento = (unsigned int) pvFnReservarSegmento( uiTamSegDatos );

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
    // Granularidad 4K (idem Padre) TODO: Verificar que sea 4K
    uiFnSetearBaseLimiteDescriptor( uiIndiceGDT_CS,
                                    uiBaseSegmento,
                                    EN_GRANULARIDAD_4K( uiTamSegCodigo ) - 1 ); 

    /* Descriptor de DATOS */

    ucpFnCopiarMemoria(
        (unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[uiIndiceGDT_DS], 
        (unsigned char*) &pstuTablaGdt->
                stuGdtDescriptorDescs[stuTSSTablaTareas[uiProcPadre].ds / 8],
        sizeof( stuGDTDescriptor ) );
    // Granularidad 4K (idem Padre) TODO: Verificar que sea 4K
    uiFnSetearBaseLimiteDescriptor( uiIndiceGDT_DS,
                        uiBaseSegmento,
                        EN_GRANULARIDAD_4K( uiTamSegDatos ) - 1 ); 

    /* Copio todo el segmento del proc actual al nuevo */
    ucpFnCopiarMemoria( (unsigned char*)uiBaseSegmento,
                        (unsigned char*)pstuPCB[uiProcPadre].uiDirBase, 
                        uiTamSegDatos ); 

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
                    (D_TSS | D_BIG), uiIndiceGDT_TSS);
   
 
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
             uiBaseSegmento,    /* base del segmento asignado */
             uiTamSegDatos,     /* LONGITUD del segmento */
             uiTamSegCodigo,    /* Tamanio del codigo */
             uiTamInicializados,/* Tamanio del bloque de datos inicializados */
             uiTamStack         /* Tamanio del stack */
             );


    // Finalmente, el proceso PADRE tiene un hijo mas!
    ++pstuPCB[uiProcPadre].lNHijos; 

    // Agregado 07/10/08:
    pstuPCB[iPosicion].ulParentId = pstuPCB[uiProcPadre].ulId;

    //TODO - Copiar memorias compartidas del padre y todos los demas
    //recursos que el hijo deba heredar

    return iPosicion;
}


/**
 * @brief Reemplaza el proceso indicado y ejecuta desde "cero"
 * @param ulPid PID del proceso a reemplazar
 * @param stArchivo Nombre del archivo binario por el cual reemplazar
 * @returns El PID del proceso modificado (ver nota al final del codigo), o un codigo de error
 */
int iFnReemplazarProceso( unsigned long ulPid, char * stArchivo ) {
    unsigned int uiESPOriginal,
             uiStackUsuario,
             uiNuevoStackUsuario,
             uiComienzoStackKernel;

    unsigned int uiBaseSegmento;
    unsigned int uiTamSegCodigo;
    unsigned int uiTamSegDatos;
    unsigned int uiTamStack;
    unsigned int uiTamInicializados;
    stuInfoEjecutable stuInfoExe;

    stuPCB* pPCB;

    /* No deshabilitamos las interrupciones porque esta funcion se invoca desde
     * el syscall execve(), que a su vez es llamado por el handler de la int
     * 0x80, por lo cual al momento de comenzar la atencion de la interrupcion,
     * se deshabilitan las demas
     */

    //Se lee la cabecera del archivo binario
    if( iFnLeerCabeceraEjecutable(stArchivo, &stuInfoExe) == -1) {
        return -1;
    }
  
    //Vamos a crear descriptores con granularidad 4K, por ello convertimos los
    //datos leidos del binario a granularidad 4K
    uiTamSegCodigo     = REDONDEAR_HACIA_ARRIBA_A_4K(stuInfoExe.uiTamanioTexto);
    uiTamStack         = REDONDEAR_HACIA_ARRIBA_A_4K(stuInfoExe.uiTamanioStack);
    uiTamInicializados = REDONDEAR_HACIA_ARRIBA_A_4K(stuInfoExe.uiTamanioDatosInicializados);
    uiTamSegDatos      = REDONDEAR_HACIA_ARRIBA_A_4K(uiTamSegCodigo + uiTamStack + uiTamInicializados); //Se crea sin Heap

    /* No buscamos entradas disponibles en la GDT porque lo unico que tenemos
     * que realizar es reemplazar el codigo del proceso indicado, y resetear el
     * estado, para arrancar este nuevo programa desde "cero"
     */

    /* TENEMOS que alojar el 'nuevo programa' en un nuevo espacio de memoria
     * porque sus segmentos de codigo y datos seran de diferente tamanio que los
     * del original (no siempre, pero casi siempre)
     */

    // Se reserva la memoria para el proceso (se usa un unico segmento para
    // Codigo y Datos)
    uiBaseSegmento = (unsigned int) pvFnReservarSegmento( uiTamSegDatos );
    
    if( uiBaseSegmento == NULL ) {
        return -ENOMEM;
    }

    // En teoria, si pude reservar el espacio en memoria, ya nada puede fallar
    // Ya se puede modificar el proceso original

    pPCB = &pstuPCB[ iFnBuscaPosicionProc(ulPid) ];
 
    /* Debo cambiar los descriptores de segmento del proceso para que apunten a 
     * la nueva ubicacion (y nuevo tamanio) de los segmentos
     */
    uiFnSetearBaseLimiteDescriptor(pPCB->uiIndiceGDT_CS, uiBaseSegmento,
            EN_GRANULARIDAD_4K(uiTamSegCodigo) - 1 );
    uiFnSetearBaseLimiteDescriptor(pPCB->uiIndiceGDT_DS, uiBaseSegmento,
            EN_GRANULARIDAD_4K(uiTamSegDatos) - 1 );

    /* COPIA de CODIGO */ 

    vFnLog("\niFnReemplazarProceso: Copiando el codigo del proceso \"%s\" de "
            "0x%x a 0x%x, ocupa %d bytes", stuInfoExe.stNombre,
            stuInfoExe.pvPuntoCarga, uiBaseSegmento, stuInfoExe.uiTamanioTexto);
    ucpFnCopiarMemoria(
        (unsigned char*)uiBaseSegmento,
        (unsigned char*)stuInfoExe.pvPuntoCarga,
        stuInfoExe.uiTamanioTexto );

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
    /* ADVERTENCIA: El comienzo del stack del kernel se calcula en 
     * **system_asm.asm**, cualquier cambio ahi (que implique desplazar el
     * stack), debe contemplarse aqui ya que este calculo resultaria invalido
     */
    uiComienzoStackKernel = 0x200000 - 0x08 - 0x100  *
                                            ((pPCB->uiIndiceGDT_TSS * 8)-0x20);
    
    /* Extraemos del stack del kernel, el ESP original: */
    uiESPOriginal = *(unsigned int*)(uiComienzoStackKernel-0x08);

    /* Calculamos la posicion del fin stack del usuario, tomando la base del
     * segmento del proceso (de su PCB) y el valor de ESP (apilado en el stack
     * del kernel)
     */
    uiStackUsuario = pPCB->uiDirBase + uiESPOriginal;

    /* ADVERTENCIA: Los calculos para el manejo de Stack se basan sobre todo en
     * lo que APILAN las siguientes funciones/macros (en orden de invocacion):
     *      (Stack de usuario)
     *          execve / SYS_CALL_3 (INT 0x80)
     *          ENTRADA_STACK_KRNL
     *
     *      (Stack Kernel)
     *          ENTRADA_STACK_KRNL
     *          lFnHandlerSyscall
     *
     * ADVERTENCIA: Un cambio en las funciones anteriores (o sus reciprocas para
     * el momento de 'volver') altera los valores usados a continuacion.
     */
    /* La mejor forma de comprender el manejo de la pila es hacer un diagrama
     * completo de los dos stackas (kernel y usuario) con todos los pushs
     * Algunas ayudas: (TOOODOS los valores en hexa)
     *
     * Stack usuario:
     *
     * 0x28 Es el alto de la pila de usuario que tiene (del tope a la base):
     *      StkUsr +00    +04  +08  +0c  +10  +14  +18  +1C  +20  +24     +28
     *               EFLAGS  ESI  EDI  GS   FS   ES   DS   EIP  CS   EFLAGS
     * 0x10 Parametros pasados al ejecutable EBP+XX (por el Shell?)
     *      StkUsr +0x28   +0x2c   +0x30   +0x34   +0x38
     *                 Param   Param   Param   Param         (0x38-0x28 = 0x10)
     *
     * Stack kernel:
     *
     *  -   Del tope (no aparece, faltan diez apilamientos) a la base
     *      ... -1c -18 -14 -10 -0c -08 -04 00  ComienzoStackKernel
     *      ...   EAX EBX ECX EDX EBP EDI ESI
     *                                (*) (*)
     *
     *      (*) Los valores de EDI y ESI se usan en realidad como ESP y SS
     *          respectivamente (ver ENTRADA_STACK_KRNL)
     */

    uiNuevoStackUsuario = uiBaseSegmento + uiTamSegDatos - 0x28 - 0x10;
    // IDEM: uiBaseSegmento + (uiTamSegDatos - pPCB->uiLimite) + uiESPOriginal;
  
    /* Reapuntamos ESP y EBP para la vuelta con SALIDA_STACK_KRNL, cuando
     * volvamos al proceso (se calculan con los nuevos tamanos de segmento)
     */
    *(unsigned int*)(uiComienzoStackKernel-0x08) =
                                uiNuevoStackUsuario - uiBaseSegmento;    // ESP

    *(unsigned int*)(uiComienzoStackKernel-0x0c) = uiTamSegDatos - 0x10; // EBP

    *(unsigned int*)(uiComienzoStackKernel-0x10) = 0x00; /* EDX */
    *(unsigned int*)(uiComienzoStackKernel-0x14) = 0x00; /* ECX */
    *(unsigned int*)(uiComienzoStackKernel-0x18) = 0x00; /* EBX */
    *(unsigned int*)(uiComienzoStackKernel-0x18) = 0x00; /* EAX */ 

    /* Las modificaciones en el Stack del usuario deben hacerse en el nuevo
     * segmento de datos, por lo que primero lo copiamos (solo el stack) desde
     * el segmento original (ya que no cambiaremos todos sus datos)
     */
    ucpFnCopiarMemoria(
        (unsigned char*)uiNuevoStackUsuario,
        (unsigned char*)uiStackUsuario,
        0x28 ); //TODO: Ver: no copiamos los 'parametros' (EBP+XX)

    // Y ahora se trabaja sobre el stack de usuario del nuevo segmento
    *(unsigned int*)(uiNuevoStackUsuario+0x04) = 0x00;   /* ESI */ 
    *(unsigned int*)(uiNuevoStackUsuario+0x08) = 0x00;   /* EDI */
    *(unsigned int*)(uiNuevoStackUsuario+0x1c) = 0x00;   /* EIP */
    *(unsigned int*)(uiNuevoStackUsuario+0x24) = 0x202L; /* EFLAGS */

    //TODO - Limpiar valores de FPU
    
    // Ya podemos liberar el bloque de memoria que ocupaba el proceso
    vFnLiberarSegmento( (void *)(pPCB->uiDirBase) );

    /* PCB */

    /* TODO: Actualizar lo que falte actualizar en el PCB
     * Solo hay que cambiar en el PCB lo que haya que cambiar; ver: man execve
     */
    pPCB->vFnFuncion    = 0x00;
    pPCB->uiTamProc     = uiTamSegDatos;
    pPCB->uiDirBase     = uiBaseSegmento;
    pPCB->uiLimite      = uiTamSegDatos;
    pPCB->uiTamanioTexto                = uiTamSegCodigo;
    pPCB->uiTamanioDatosInicializados   = uiTamInicializados;
    pPCB->uiTamanioStack                = uiTamStack;

    // TODO: Liberar todos los recursos que haya que liberar

    //Desadjuntamos todas las memorias compartidas
    iFnShmDtAllProc( ulPid );

    // LISTO!
    /* En realidad, si execve no falla, no retorna nada (el nuevo programa ni
     * siquiera sabe que se llamo a execve). Esta devolucion se pierde por los
     * manejos del stack anteriores, pero es necesaria para que compile (podemos
     * devolver cualquier cosa)
     */
    return (int) ulPid;
}


/**
 * @brief Elimina el proceso indicado (PCB, segmentos y entradas en la GDT)
 * @param Posicion del proceso dentro de la tabla de PCBs
 * @returns La posicion dentro de la tabla de PCBs del proceso eliminado
 */
int iFnEliminarProceso( unsigned int uiProceso ) {
    int iPCBPadre;

    vFnLog( "\niFnEliminarProceso(): Liberando el segmento base %x (%d bytes)",
             pstuPCB[ uiProceso ].uiDirBase, pstuPCB[ uiProceso ].uiLimite);

    vFnLiberarSegmento( (void *)pstuPCB[ uiProceso ].uiDirBase);

    /* Liberamos las 3 entradas de la GDT reservadas para el descriptor
     * de segmento de codigo, datos y TSS: */
    vFnLog( "\n iFnEliminarProceso(): Liberando entradas %d, %d y %d de la GDT",
             pstuPCB[ uiProceso ].uiIndiceGDT_CS,
             pstuPCB[ uiProceso ].uiIndiceGDT_DS,
             pstuPCB[ uiProceso ].uiIndiceGDT_TSS );
    mapa_gdt_set( pstuPCB[ uiProceso ].uiIndiceGDT_CS,  0 );
    mapa_gdt_set( pstuPCB[ uiProceso ].uiIndiceGDT_DS,  0 );
    mapa_gdt_set( pstuPCB[ uiProceso ].uiIndiceGDT_TSS, 0 );

    // Despertamos al padre, si es que lo estaba esperando
    iPCBPadre = iFnBuscaPosicionProc( pstuPCB[ uiProceso ].ulParentId );
    if (pstuPCB[iPCBPadre].iEstado == PROC_ESPERANDO) {
        pstuPCB[iPCBPadre].iEstado = PROC_LISTO;
    }

    //Desadjuntamos todas las memorias compartidas
    iFnShmDtAllProc( pstuPCB[ uiProceso ].ulId );

    /* Liberamos la PCB que utilizaba (y por consiguiente, la entrada en la
     * tabla de TSS */
    vFnLog( "\n iFnEliminarProceso(): Eliminando PCB %d (PID %d)", uiProceso,
             pstuPCB[ uiProceso ].ulId );
    pstuPCB[ uiProceso ].iEstado = PROC_ELIMINADO;

    return uiProceso;
}


/**
\brief Abre un archivo ejecutable y recupera la informacion de su cabecera
\param stArchivo Nombre del archivo ejecutable
\param pstuInfo Puntero a una estructura stuInfoEjecutable, donde se almacenan los datos leidos
\returns 0 si concluye con exito, -1 si hay error
*/
int iFnLeerCabeceraEjecutable(char* stArchivo, stuInfoEjecutable* pstuInfo) {

    /**
     * \note Hoy en dia los ejecutables que acepta Sodium son binarios planos,
     * por lo que no hay cabecera a leer; se toman los valores desde el RamFS.
     * Cuando Sodium tenga un mejor soporte de FS e interprete archivos ELF, se 
     * deberia modificar esta funcion para que los interprete.
     */

    int i;
    stEntradaLS *pstEntLs;
    stDirectorio *pstDirBusqueda;

    if (iFnObtenerDirectorio("/mnt/usr", &pstDirBusqueda) < 0) {
        vFnImprimir("\nSodium Dice: Error! Directorio /mnt/usr no existe");
        return -1;
    }
    if( iFnBuscarArchivo(pstDirBusqueda, stArchivo, &pstEntLs) < 0) {
        vFnImprimir("\nSodium Dice: Error! El archivo %s no existe", stArchivo);
        return -1;
    } 

    pstuInfo->pvPuntoCarga = (void*)DIR_LINEAL(pstEntLs->wSeg,pstEntLs->wOff);
    pstuInfo->uiTamanioTexto  = pstEntLs->dwTamanio;
    pstuInfo->uiTamanioDatosInicializados = 0;
    pstuInfo->uiTamanioStack  = STACK_SIZE;

    for (i = 0; i < 11; i++) {
        pstuInfo->stNombre[i] = pstEntLs->strNombreArchivo[i];
    }
    pstuInfo->stNombre[i] = '\0';

    return 0;
}


/******************************************************************************/


/* Agregado por el grupo */ /* ANIO: ??? */

/* 12/10/2008:
 * TODO - Rehacer esta funcion. NO ANDA!
 * Tener en cuenta que el codigo de la funcion esta basado en la implementacion
 * anterior de segmentacion en Sodium (segmentos fijos). No se actualizo la
 * funcion para que soporte segmentos de tamanos variables, ya que no anda.
 * Esta funcion es indispensable para el trabajo con threads
 */

#define SEGMENT_SIZE    0x01000 // 32 Kb

/**
 * @brief NO ANDA! REHACER... (Supuestamente) Crea un nuevo proceso, pero compartiendo el segmento de datos y codigo del proceso que lo llamo
 * @returns 0 si fue exitoso, distinto de 0 si hubo error
 */
int iFnClonarProceso() {
    unsigned int uiIndiceGDT_TSS;
    unsigned int uiIndiceGDT_DS;
    unsigned int uiBaseSegmento;
    int iPosicion;
    unsigned int uiComienzoStackKernel, uiStackUsuario;
    
    if(!uiIndiceGDT_TSS) {
        vFnImprimir("ENOMEM\n");
        return -ENOMEM;
    }

    iPosicion = iFnBuscarPCBLibre();
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
                                    (SEGMENT_SIZE / 4096) - 1 );
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
                    (D_TSS | D_BIG), uiIndiceGDT_TSS);
    
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
            SEGMENT_SIZE,
            0,
            0,
            0);

    return 0;
}

