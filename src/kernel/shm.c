#include <kernel/semaforo.h>
#include <kernel/shm.h>
#include <kernel/definiciones.h>
#include <kernel/pcb.h>
#include <kernel/gdt.h>
#include <kernel/system.h>
#include <video.h>


extern stuPCB pstuPCB[CANTMAXPROCS];
extern unsigned long ulProcActual;

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
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
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnShmAt(int shmid, void * shmAddr)
{

    int espacioLibrePCB=0;
    int espacioLibreShMem=0;
    int iPosicionPCBProcComparte;
    void * addrAbsoluta;
    int i;

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
                    (char*)pstuPCB[ulProcActual].uiDirBase +
                        (unsigned int)shmAddr,
                    (char*)pstuPCB[iPosicionPCBProcComparte].uiDirBase +
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


/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnShmDt(int shmid){

    int i;
    bool desocuparShm = TRUE;
    for(i=0;i<CANTMAXPROCSHM;i++)
    {
        if(memoriasCompartidas[shmid].procAtt[i].pid == pstuPCB[ulProcActual].ulId)
        {
            memoriasCompartidas[shmid].procAtt[i].pid = -1;
        } 
        else if(memoriasCompartidas[shmid].procAtt[i].pid != -1)
        {
            desocuparShm = FALSE;
        }
    }
    
    if(desocuparShm == TRUE)
    {
        memoriasCompartidas[shmid].declarada = FALSE;
        memoriasCompartidas[shmid].key = 0;
        memoriasCompartidas[shmid].tamanio = 0;
    }

    return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
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
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
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
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnCopiarVariablesCompartidas()
{
    int srcIndiceShMem; 
    int srcIndiceEnAttach;
    int iPosicionPCBProcComparte;
        
    int i;
    int j;
    int k;
    //byte aux;
    char *auxSrc;
    char *auxDest;
    
    //Me fijo lasvariables comprtidas que iene el proceso que se esta por ir
    for (i=0; i<MAXSHMEMPORPROCESO; i++) {
        // Si tengo una variable compartida, la copio a los demas procesos
        if (pstuPCB [ulProcActual].memoriasAtachadas[i].utilizada == TRUE) {
            srcIndiceShMem =
                pstuPCB [ulProcActual].memoriasAtachadas[i].posicionEnShMem;
            srcIndiceEnAttach =
                pstuPCB [ulProcActual].memoriasAtachadas[i].posicionEnAttach;

            //Debo pasar la direccion de memoria relativa a absoluta
            auxSrc = (char*) pstuPCB [ulProcActual].uiDirBase + (unsigned int) (memoriasCompartidas[srcIndiceShMem].procAtt[srcIndiceEnAttach].ptrShAddr);

            // Copio mi variable en todos los demas que tienen el mismo key
            for (j=0; j<CANTMAXPROCSHM; j++) {
                // Si el pid no es -1 entonces es porque esa posicion esta usada
                // por otro proceso
                if (memoriasCompartidas[srcIndiceShMem].procAtt[j].pid != -1) { 
                    //TODO 

                    //Debo pasar la direccion de memoria relativa a absoluta
                    iPosicionPCBProcComparte = iFnBuscaPosicionProc(
                        memoriasCompartidas[srcIndiceShMem].procAtt[j].pid);
                    auxDest = (char*) pstuPCB [iPosicionPCBProcComparte].uiDirBase + (unsigned int) (memoriasCompartidas[srcIndiceShMem].procAtt[j].ptrShAddr);

                                    
                    ucpFnCopiarMemoria(
                        auxDest,
                        auxSrc,
                        memoriasCompartidas[srcIndiceShMem].tamanio);
                }
            }
        }
    }
}

