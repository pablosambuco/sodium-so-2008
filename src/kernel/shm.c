#include <kernel/semaforo.h>
#include <kernel/shm.h>
#include <kernel/definiciones.h>
#include <kernel/pcb.h>
#include <kernel/gdt.h>
#include <kernel/system.h>
#include <video.h>


extern stuPCB pstuPCB[CANTMAXPROCS];
extern unsigned long ulProcActual;

/* 11/10/2008 - Grupo SEGMENTEACION PURA:
 *
 * Se modifican las funciones de memoria compartida para que trabajen con
 * direcciones relativas a los segmentos de los procesos, ya que ahora los
 * procesos son reubicables.
 *
 * Se modifica iFnShmAt para que inicialice solo la parte de la memoria
 * compartida que este dentro del segmento de datos del proceso. Se determina
 * primero que parte (cuantos bytes) del bloque compartido entra en el segmento
 * del proceso. Se inicializa esa parte del bloque con los datos provenientes
 * de:
 *   - El primer proceso que posea esa parte del bloque compartido
 *   - El proceso que abarque la mayor parte del bloque comparitdo, si ninguno
 * de los procesos posee la totalidad del bloque compartido dentro de su
 * segmento. El resto de la memoria compartida queda sin inicializar.
 *
 * Se modifica vFnCopiarVariablesCompartidas para que copie solamente las
 * direcciones de los bloques de memoria compartida que se encuentren dentro de
 * los segmentos de datos de los procesos. Surge esta necesidad ya que ahora los
 * segmentos de los procesos son redimensionables, y tras una llamada a brk, una
 * variable compartida (o parte de ella) podria quedar fuera del segmento del
 * proceso (origen o destino). De no tenerse en cuenta el limite del segmento
 * del proceso usuario, el kernel podria sobreescribir memoria en uso por otro
 * proceso, etc.
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
        if(memoriasCompartidas[i].declarada == TRUE && memoriasCompartidas[i].key==key)
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
\date 11/10/2008
\note El 'segmento' de memoria compartida debe estar dentro del segmento de datos direccionable por el proceso.
*/
int iFnShmAt(int shmid, void * shmAddr)
{

    int espacioLibrePCB=0;
    int espacioLibreShMem=0;
    int iPosicionPCBProcComparte;
    int i;

    /* 11/10/2008 - Grupo SEGMENTEACION PURA:
     *
     * Se modifica iFnShmAt para que inicialice solo la parte de la memoria
     * compartida que este dentro del segmento de datos del proceso.
     * Se determina primero, que parte (cuantos bytes) del bloque compartido
     * entra en el segmento del proceso.
     * Se inicializa esa parte del bloque con los datos provenientes de:
     *   - El primer proceso que posea esa parte del bloque compartido
     *   - El proceso que abarque la mayor parte del bloque comparitdo, si
     * ninguno de los procesos posee la totalidad del bloque compartido dentro
     * de su segmento. El resto de la memoria compartida queda sin inicializar.
     */
    unsigned char *pcAuxSrc;
    unsigned char *pcAuxDest;
    unsigned int uiBytesSHMSrc;
    unsigned int uiBytesSHMDest;


    if( shmid >= CANTMAXSHM ) {
        return -1;
    }

    /*
    validamos que la memoria compartida este declarada en el
    vector de memoria compartida global
    */
    if ( memoriasCompartidas[shmid].declarada == FALSE ) {
        return -1;
    }

    /*
    buscamos dentro del vector de memoria compartida global, una 
    posicion libre en los procesos que estan atacheados a esa
    memoria compartida    
    */
    while (espacioLibreShMem < CANTMAXPROCSHM && 
            memoriasCompartidas[shmid].procAtt[espacioLibreShMem].pid!=-1) {
        espacioLibreShMem++;
    }

    if (espacioLibreShMem==CANTMAXPROCSHM){
        return -1;
    }

    /*
    buscamos un espacio libre dentro del vector de memoria compartida
    del proceso
    */
    while (espacioLibrePCB < MAXSHMEMPORPROCESO && pstuPCB[ulProcActual].memoriasAtachadas[espacioLibrePCB].utilizada == TRUE) {
        espacioLibrePCB++;
    }

    
    if (espacioLibrePCB==MAXSHMEMPORPROCESO){
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

    //Si la memoria compartida esta completamente fuera del segmento, no hay que
    //inicializar nada
    if( pstuPCB[ulProcActual].uiLimite - (unsigned int) shmAddr <= 0 ) {
        return 0;
    }

    //Determino cuantos bytes necesito para inicializacion (cuantos entran en el
    //segmento)
    if( memoriasCompartidas[shmid].tamanio <
        pstuPCB[ulProcActual].uiLimite - (unsigned int) shmAddr ) {
            //Entra completa en el segmento, necesito todos sus bytes
            uiBytesSHMDest = memoriasCompartidas[shmid].tamanio;
    } else {
            //NO entra completa en el segmento, necesito los bytes que entren
            uiBytesSHMDest =
                pstuPCB[ulProcActual].uiLimite - (unsigned int) shmAddr;
    }

    uiBytesSHMSrc = 0; //En principio, no tengo bytes para ser copiados

    //Busco entre los procesos adjuntos a esa memoria compartida, cual es que se
    //usa para incializar el bloque nuevo (ver comentarios al principio)
    for( i=0; i<CANTMAXPROCSHM; i++ ) {
        if( memoriasCompartidas[shmid].procAtt[i].pid !=
            pstuPCB[ulProcActual].ulId &&
            memoriasCompartidas[shmid].procAtt[i].pid != -1) {

            iPosicionPCBProcComparte = iFnBuscaPosicionProc(
                    memoriasCompartidas[shmid].procAtt[i].pid);

            //Determino si el proceso 'i' posee suficientes bytes como para
            //inicializar el bloque
            if((pstuPCB[iPosicionPCBProcComparte].uiLimite -
               (unsigned int) memoriasCompartidas[shmid].procAtt[i].ptrShAddr)>=
                uiBytesSHMDest ) {
                //SI tiene, puedo realizar la copia
                //Guardo cuantos bytes tiene y la direc absoluta donde comienza
                uiBytesSHMSrc = pstuPCB[iPosicionPCBProcComparte].uiLimite -
                                (unsigned int)
                                memoriasCompartidas[shmid].procAtt[i].ptrShAddr;
                pcAuxSrc = (unsigned char*)
                            pstuPCB[iPosicionPCBProcComparte].uiDirBase +
                            (unsigned int)
                            memoriasCompartidas[shmid].procAtt[i].ptrShAddr;
                //Dejo de buscar
                break;
            } else if( (pstuPCB[iPosicionPCBProcComparte].uiLimite -
                        (unsigned int)
                        memoriasCompartidas[shmid].procAtt[i].ptrShAddr) >
                        uiBytesSHMSrc) {
                //NO tiene. Actualizo si es el que posee mas bytes
                uiBytesSHMSrc = pstuPCB[iPosicionPCBProcComparte].uiLimite -
                                (unsigned int)
                                memoriasCompartidas[shmid].procAtt[i].ptrShAddr;
                pcAuxSrc = (unsigned char*)
                            pstuPCB[iPosicionPCBProcComparte].uiDirBase +
                            (unsigned int)
                            memoriasCompartidas[shmid].procAtt[i].ptrShAddr;
                //Y sigo buscando
            }
        }
    }

    //Ya encontre cuanto copiar y desde donde; hago la copia
    if( uiBytesSHMSrc > 0 && uiBytesSHMDest > 0 ) {

        pcAuxDest = (unsigned char*)
            pstuPCB [ulProcActual].uiDirBase + (unsigned int)shmAddr;

        if( uiBytesSHMSrc < uiBytesSHMDest ) {
            ucpFnCopiarMemoria( pcAuxDest, pcAuxSrc, uiBytesSHMSrc );
        } else {
            ucpFnCopiarMemoria( pcAuxDest, pcAuxSrc, uiBytesSHMDest );
        }
    }

    return 0;
}


/**
\brief Des-adjunta un 'segmento' de memoria compartida
\param shmid Identificador de la memoria compartida (SHMID)
\returns Si hubo exito, devuelve 0; si hubo error, devuelve -1
*/
int iFnShmDt(int shmid){

    int i;
    bool desocuparShm = TRUE;

    if( shmid >= CANTMAXSHM ) {
        return -1;
    }

    for( i=0; i<CANTMAXPROCSHM; i++ ) {
        if( memoriasCompartidas[shmid].procAtt[i].pid == pstuPCB[ulProcActual].ulId ) {
            memoriasCompartidas[shmid].procAtt[i].pid = -1;
        } 
        else if( memoriasCompartidas[shmid].procAtt[i].pid != -1) {
            //No se debe liberar la memoria compartida si hay algun otro proceso
            //adjuntado
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
\date 11/10/2008
*/
void vFnCopiarVariablesCompartidas()
{
    int srcIndiceShMem; 
    int srcIndiceEnAttach;
    int iPosicionPCBProcComparte;
        
    int i, j;
    unsigned char *pcAuxSrc;
    unsigned char *pcAuxDest;

    /* 11/10/2008 - Grupo SEGMENTEACION PURA:
     *
     * Se modifica vFnCopiarVariablesCompartidas para que copie solamente las
     * direcciones de los bloques de memoria compartida que se encuentren dentro
     * de los segmentos de datos de los procesos. Surge esta necesidad ya que
     * ahora los segmentos de los procesos son redimensionables, y tras una
     * llamada a brk, una variable compartida (o parte de ella) podria quedar
     * fuera del segmento del proceso (origen o destino). De no tenerse en
     * cuenta el limite del segmento del proceso usuario, el kernel podria
     * sobreescribir memoria en uso por otro proceso, etc.
     */
    unsigned int uiBytesSHMSrc;
    unsigned int uiBytesSHMDest;
    
    //Me fijo las variables comprtidas que tiene el proceso que se esta por ir
    for (i=0; i<MAXSHMEMPORPROCESO; i++) {
        // Si tengo una variable compartida, la copio a los demas procesos
        if (pstuPCB [ulProcActual].memoriasAtachadas[i].utilizada == TRUE) {
            srcIndiceShMem =
                pstuPCB [ulProcActual].memoriasAtachadas[i].posicionEnShMem;
            srcIndiceEnAttach =
                pstuPCB [ulProcActual].memoriasAtachadas[i].posicionEnAttach;

            //Debo pasar la direccion de memoria relativa a absoluta
            pcAuxSrc = (unsigned char*) pstuPCB [ulProcActual].uiDirBase + (unsigned int) (memoriasCompartidas[srcIndiceShMem].procAtt[srcIndiceEnAttach].ptrShAddr);

            //Calculo cuantos bytes de la memoria compartida estan dentro del
            //segmento del proceso 'actual' 
            if( (unsigned int)
                (pcAuxSrc + memoriasCompartidas[srcIndiceShMem].tamanio) <=
                (pstuPCB[ulProcActual].uiDirBase+pstuPCB[ulProcActual].uiLimite)
                ) {
                    uiBytesSHMSrc = memoriasCompartidas[srcIndiceShMem].tamanio;
            } else {
                    uiBytesSHMSrc = pstuPCB[ulProcActual].uiDirBase +
                        pstuPCB[ulProcActual].uiLimite - 
                        (unsigned int) pcAuxSrc;
            }

            // Copio mi variable en todos los demas que tienen el mismo key
            for (j=0; j<CANTMAXPROCSHM; j++) {
                // Si el pid no es -1 entonces es porque esa posicion esta usada
                // por otro proceso
                if (memoriasCompartidas[srcIndiceShMem].procAtt[j].pid != -1) { 

                    //Debo pasar la direccion de memoria relativa a absoluta
                    iPosicionPCBProcComparte = iFnBuscaPosicionProc(
                        memoriasCompartidas[srcIndiceShMem].procAtt[j].pid);

                    pcAuxDest = (unsigned char*)
                        pstuPCB [iPosicionPCBProcComparte].uiDirBase +
                        (unsigned int) (memoriasCompartidas[srcIndiceShMem].
                                                        procAtt[j].ptrShAddr);

                    //Calculo cuantos bytes de la memoria compartida estan
                    //dentro del segmento del proceso 'destino' 
                    if((unsigned int)
                       (pcAuxDest+memoriasCompartidas[srcIndiceShMem].tamanio)<=
                       (pstuPCB[iPosicionPCBProcComparte].uiDirBase +
                        pstuPCB[iPosicionPCBProcComparte].uiLimite) ) {
                            uiBytesSHMDest =
                                memoriasCompartidas[srcIndiceShMem].tamanio;
                    } else {
                            uiBytesSHMDest =
                                pstuPCB[iPosicionPCBProcComparte].uiDirBase +
                                pstuPCB[iPosicionPCBProcComparte].uiLimite -
                                (unsigned int) pcAuxDest;
                    }
                                    
                    //Copio la memoria compartida, la cantidad de bytes sera la
                    //menor entre uiBytesSHMSrc y uiBytesSHMDest
                    if(uiBytesSHMSrc < uiBytesSHMDest) {
                        ucpFnCopiarMemoria(pcAuxDest, pcAuxSrc, uiBytesSHMSrc);
                    } else {
                        ucpFnCopiarMemoria(pcAuxDest, pcAuxSrc, uiBytesSHMDest);
                    }
                }
            }
        }
    }
}

