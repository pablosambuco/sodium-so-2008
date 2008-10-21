#include <kernel/semaforo.h>
#include <kernel/shm.h>
#include <kernel/definiciones.h>
#include <kernel/pcb.h>
#include <kernel/gdt.h>
#include <kernel/system.h>
#include <video.h>


extern stuPCB pstuPCB[CANTMAXPROCS];
extern unsigned long ulProcActual;

/* 17/10/2008 - Grupo SEGMENTEACION PURA:
 *
 * Se modifican las funciones de memoria compartida para que trabajen con
 * direcciones relativas a los segmentos de los procesos, ya que ahora los
 * procesos son reubicables.
 */


/**
\brief Crea un 'segmento' de memoria compartida
\param key Clave de la memoria compartida
\param tamanio Tamanio del 'segmento' de memoria compartida
\returns Si hubo exito, devuelve el identificador de la memoria compartida (SHMID: la posicion del descriptor de memoria compartida dentro de la tabla de memorias compartidas); si hubo error, devuelve -1
*/
int iFnShmGet(key_t key, size_t tamanio)
{

    int i;
    int j;
    //verifico si existe la key
    for(i=0;i<CANTMAXSHM;i++)
    {    
        /*
        Verifico si ya existe el key. si existe, se verifica si
        si el tamaño es el mismo, si es el mismo, retorno la
        posicion de la memoria compartida que ya existes
        */
        if( memoriasCompartidas[i].declarada == TRUE &&
            memoriasCompartidas[i].key==key)
        {
            if(memoriasCompartidas[i].tamanio!=tamanio)
            {
                return -1;
            }
            return i;
        }
    }

    for(i=0;i<CANTMAXSHM;i++)
    {        
        if(memoriasCompartidas[i].declarada==FALSE)
        {
            memoriasCompartidas[i].declarada=TRUE;
            memoriasCompartidas[i].key=key;
            memoriasCompartidas[i].tamanio=tamanio;
            /*
            Inicializo vector de procesos atachados como vacio
            (todos los pids van en -1)
            */
            for(j=0;j<CANTMAXPROCSHM;j++)
            {
                memoriasCompartidas[i].procAtt[j].pid=-1;
            }
            return i;
        }
    }

    //No hay entrada libre en el vector
    return -1;
}


/**
\brief Adjunta un 'segmento' de memoria compartida
\param shmid Identificador de la memoria compartida (SHMID)
\param shmAddr Direccion de memoria del proceso a partir de la cual se 'adjuntara' el 'segmento' de memoria compartida
\returns Si hubo exito, devuelve 0; si hubo error, devuelve -1
*/
int iFnShmAt(int shmid, void * shmAddr)
{

    int espacioLibrePCB=0;
    int espacioLibreShMem=0;
    int iPosicionPCBProcComparte;
    int i;

    if( shmid >= CANTMAXSHM ) {
        return -1;
    }

    /*
    validamos que la memoria compartida este declarada en el
    vector de memoria compartida global
    */
    if (memoriasCompartidas[shmid].declarada==FALSE)    
    {
        return -1;
    }

    /*
    buscamos dentro del vector de memoria compartida global, una 
    posicion libre en los procesos que estan atacheados a esa
    memoria compartida    
    */
    while (espacioLibreShMem < CANTMAXPROCSHM && 
    memoriasCompartidas[shmid].procAtt[espacioLibreShMem].pid!=-1)
    {
        espacioLibreShMem++;
    }

    if (espacioLibreShMem==CANTMAXPROCSHM){
        return -1;
    }

    /*
    buscamos un espacio libre dentro del vector de memoria compartida
    del proceso
    */
    while (espacioLibrePCB < MAXSHMEMPORPROCESO && pstuPCB[ulProcActual].memoriasAtachadas[espacioLibrePCB].utilizada == TRUE)
    {
        espacioLibrePCB++;
    }

    
    if (espacioLibrePCB==MAXSHMEMPORPROCESO){
        return -1;
    }

    /* 18/10/2008
     * Verificamos que la memoria compartida entre completamente en el segmento
     * del proceso
     */
    if( (char * )shmAddr + memoriasCompartidas[shmid].tamanio >=
            (char *)pstuPCB[ulProcActual].uiLimite ) {
        return -1;
    }

    //TODO - Mejorar estos comentarios:
    // aca en teoria puedo agregar

    pstuPCB[ulProcActual].memoriasAtachadas[espacioLibrePCB].utilizada = TRUE;
    pstuPCB[ulProcActual].memoriasAtachadas[espacioLibrePCB].posicionEnShMem =
        shmid;
    pstuPCB[ulProcActual].memoriasAtachadas[espacioLibrePCB].posicionEnAttach =
        espacioLibreShMem;

    memoriasCompartidas[shmid].procAtt[espacioLibreShMem].pid =
        pstuPCB[ulProcActual].ulId;

    /* Antes se grababa la posicion absoluta donde se 'atacheaba' la memoria
     * compartida. Ahora, con memoria virtual, los procesos pueden ser movidos,
     * por lo que grabamos la direccion relativa
     */
    memoriasCompartidas[shmid].procAtt[espacioLibreShMem].ptrShAddr = shmAddr;

    /* INICIALIZACION */

    /*
    nos fijamos si alguien ya esta atacheado a la memoria compartida y,
    en caso de estarlo, nos copiamos su valor
    PARCHE: ESTO NO ES RESPONSABILIDAD DE LA FUNCION ATTACH PERO SE HIZO DADA LA
    IMPLEMENTACION CONCRETA DE MEMORIA COMPARTIDA
    */

    for(i=0;i<CANTMAXPROCSHM;i++)
    {
        if( memoriasCompartidas[shmid].procAtt[i].pid !=
                pstuPCB[ulProcActual].ulId &&
                memoriasCompartidas[shmid].procAtt[i].pid != -1) {

                iPosicionPCBProcComparte = iFnBuscaPosicionProc(
                        memoriasCompartidas[shmid].procAtt[i].pid);

                //Debo pasar las direcciones de memoria relativas a absolutas
                ucpFnCopiarMemoria(
                   (unsigned char*)pstuPCB[ulProcActual].uiDirBase +
                        (unsigned int)shmAddr,
                   (unsigned char*)pstuPCB[iPosicionPCBProcComparte].uiDirBase +
                        (unsigned int)
                            memoriasCompartidas[shmid].procAtt[i].ptrShAddr,
                   memoriasCompartidas[shmid].tamanio);

                //Lo hago solo para el primer proceso attacheado que encuentre
                return 0;
        } 
    }

    //retorno exito
    return 0;
}


/**
\brief Des-adjunta un 'segmento' de memoria compartida del proceso actual
\param shmid Identificador de la memoria compartida (SHMID)
\returns Si hubo exito, devuelve 0; si hubo error, devuelve -1
*/
int iFnShmDt(int shmid){
    return iFnShmDtProc(shmid, pstuPCB[ulProcActual].ulId);
}


/**
\brief Des-adjunta un 'segmento' de memoria compartida de un proceso dado
\param shmid Identificador de la memoria compartida (SHMID)
\param uiPid PID del proceso
\returns Si hubo exito, devuelve 0; si hubo error, devuelve -1
*/
int iFnShmDtProc(int shmid, unsigned int uiPid) {
    int i, j;
    bool desocuparShm = TRUE;
    unsigned long ulPosProc;

    ulPosProc = iFnBuscaPosicionProc(uiPid);

    if( ulPosProc == -1 ) {
        return -1;
    }

    if( shmid >= CANTMAXSHM ) {
        return -1;
    }

    for(i=0;i<CANTMAXPROCSHM;i++)
    {
        if(memoriasCompartidas[shmid].procAtt[i].pid == uiPid)
        {
            memoriasCompartidas[shmid].procAtt[i].pid = -1;

            //Quito la entrada en el PCB del proceso
            for(j=0; j<MAXSHMEMPORPROCESO; j++) {
                if(pstuPCB[ulPosProc].memoriasAtachadas[j].posicionEnShMem == shmid) {
                    pstuPCB[ulPosProc].memoriasAtachadas[j].utilizada = FALSE;
                }
            }
        } 
        else if(memoriasCompartidas[shmid].procAtt[i].pid != -1)
        {
            //No se debe liberar la memoria compartida si hay algun otro proceso
            //adjunto
            desocuparShm = FALSE;
        }
    }
    
    //Si no hay otro proceso adjuntado a la memoria compartida, se debe liberar
    if(desocuparShm == TRUE)
    {
        memoriasCompartidas[shmid].declarada = FALSE;
        memoriasCompartidas[shmid].key = 0;
        memoriasCompartidas[shmid].tamanio = 0;
    }

    return 0;
}


/**
\brief Des-adjunta todos los 'segmentos' de memoria compartida de un proceso dado
\param uiProcId PID del proceso
\returns Si hubo exito, devuelve 0; si hubo error, devuelve -1
*/
int iFnShmDtAllProc(unsigned int uiPid) {
    int i;
    unsigned long ulPosProc;

    ulPosProc = iFnBuscaPosicionProc(uiPid);

    if( ulPosProc == -1 ) {
        return -1;
    }

    for(i=0; i<MAXSHMEMPORPROCESO; i++) {
        if( pstuPCB[ulPosProc].memoriasAtachadas[i].utilizada == TRUE ) {
            iFnShmDtProc(pstuPCB[ulPosProc].memoriasAtachadas[i].posicionEnShMem, uiPid);
        }
    }

    return 0;
}


/**
\brief Devuelve la direccion mas alta de memoria compartida del proceso dado
\param uiPid PID del proceso
\returns Direccion mas alta de memoria compartida del proceso, o 0 si no tiene memorias compartidas atachadas
*/
unsigned long ulFnMaxDirShmProc(unsigned int uiPid) {
    int i;
    int iPosProc;
    int shmid;
    int iPosEnAtt;
    unsigned long ulMaxDir;

    iPosProc = iFnBuscaPosicionProc(uiPid);

    if( iPosProc == -1 ) {
        return -1;
    }

    ulMaxDir = 0;
    for(i=0; i<MAXSHMEMPORPROCESO; i++) {
        if( pstuPCB[iPosProc].memoriasAtachadas[i].utilizada == TRUE ) {
            shmid = pstuPCB[iPosProc].memoriasAtachadas[i].posicionEnShMem;
            iPosEnAtt= pstuPCB[iPosProc].memoriasAtachadas[i].posicionEnAttach;
            if(((char*)memoriasCompartidas[shmid].procAtt[iPosEnAtt].ptrShAddr +
                memoriasCompartidas[shmid].tamanio) > (char*)ulMaxDir) {
                ulMaxDir = (unsigned long)(char*)
                    memoriasCompartidas[shmid].procAtt[iPosEnAtt].ptrShAddr +
                    memoriasCompartidas[shmid].tamanio;
            }
        }
    }

    return ulMaxDir;
}


/**
\brief Inicializa la tabla de descriptores de memoria compartida
*/
void vFnInicializarShms(){

    int i;

    shMem shm;
    shm.key = 0;
    shm.tamanio = 0;
    shm.declarada = FALSE;

    for (i=0;i<CANTMAXPROCSHM;i++)
    {
        shm.procAtt[i].pid=-1;
    }

    for (i=0;i<CANTMAXSHM;i++)
    {
        memoriasCompartidas[i]=shm;
    }
}


/**
\brief Muestra por pantalla el estado de la memoria compartida
*/
void vFnVerShm()
{
    int i,j;

    vFnImprimir("\n\nMemorias Compartidas del Sistema:\n");
    for (i=0;i<CANTMAXSHM;i++)
    {
        vFnImprimir("Key: %d, Tamanio: %d, Declarada %d, Pids: ", memoriasCompartidas[i].key, memoriasCompartidas[i].tamanio, memoriasCompartidas[i].declarada);

        for (j=0;j<CANTMAXPROCSHM;j++)
        {
            vFnImprimir("[%d]",memoriasCompartidas[i].procAtt[j].pid);
        }

        vFnImprimir("\n");
    }
}


/**
\brief Copia los 'segmentos' de memoria compartida del proceso 'actual' (el que acaba de salir de ejecucion) a los procesos adjuntados a esos 'segmentos'. Esta funcion hace que el 'segmento' APARENTE estar compartido
*/
void vFnCopiarVariablesCompartidas()
{
    int srcIndiceShMem; 
    int srcIndiceEnAttach;
    int iPosicionPCBProcComparte;
        
    int i, j;
    unsigned char *pcAuxSrc;
    unsigned char *pcAuxDest;
    
    //Me fijo lasvariables comprtidas que tiene el proceso que se esta por ir
    for (i=0; i<MAXSHMEMPORPROCESO; i++) {
        // Si tengo una variable compartida, la copio a los demas procesos
        if (pstuPCB [ulProcActual].memoriasAtachadas[i].utilizada == TRUE) {
            srcIndiceShMem =
                pstuPCB [ulProcActual].memoriasAtachadas[i].posicionEnShMem;
            srcIndiceEnAttach =
                pstuPCB [ulProcActual].memoriasAtachadas[i].posicionEnAttach;

            //Debo pasar la direccion de memoria relativa a absoluta
            pcAuxSrc = (unsigned char*) pstuPCB [ulProcActual].uiDirBase +(unsigned int) (memoriasCompartidas[srcIndiceShMem].procAtt[srcIndiceEnAttach].ptrShAddr);

            // Copio mi variable en todos los demas que tienen el mismo key
            for (j=0; j<CANTMAXPROCSHM; j++) {
                // Si el pid no es -1 entonces es porque esa posicion esta usada
                // por otro proceso
                if (memoriasCompartidas[srcIndiceShMem].procAtt[j].pid != -1) { 

                    //Debo pasar la direccion de memoria relativa a absoluta
                    iPosicionPCBProcComparte = iFnBuscaPosicionProc(
                        memoriasCompartidas[srcIndiceShMem].procAtt[j].pid);
                    pcAuxDest = (unsigned char*) pstuPCB [iPosicionPCBProcComparte].uiDirBase + (unsigned int) (memoriasCompartidas[srcIndiceShMem].procAtt[j].ptrShAddr);
                                    
                    ucpFnCopiarMemoria(
                        pcAuxDest,
                        pcAuxSrc,
                        memoriasCompartidas[srcIndiceShMem].tamanio);
                }
            }
        }
    }
}

