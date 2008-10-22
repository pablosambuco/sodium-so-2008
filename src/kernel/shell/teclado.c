#include <video.h>
#include <kernel/definiciones.h>
#include <kernel/system.h>
#include <kernel/system_asm.h>
#include <kernel/libk/string.h>
#include <fs/ramfs.h>
#include <shell/teclado.h>
#include <shell/shell.h>

// 21/10/2008 - Agregados para CTRL+C
#include <kernel/gdt.h>
#include <kernel/pcb.h>
#include <kernel/signal.h>
#include <kernel/syscall.h>
// Fin Agregados 21/10/2008

int bActivarTeclado = 0;

stuDefinicionTeclado stuMapaTeclado;

int shift = 0;
int ctrl  = 0;
int alt   = 0;
int altgr = 0;
int caps  = 0;
int num   = 0;

int iCantidadCaracteres = 0;
int iSemaforoBufferTeclado = 0;
int iLlenarIndice = 0;


/**
\fn void vFnManejadorTecladoShell (unsigned char ucScanCode)
\brief Se ejecutará desde el manejador del teclado de la IDT.
Como se ejecuta en forma asincrónica, se usa un flag semáforo para evitar (en realidad minimizar, a menos que usemos cli y sti)los efectos de las condiciones de carrera.
\param ucScanCode Scancode recibido desde el teclado
\date 21/10/2008
*/
void vFnManejadorTecladoShell (unsigned char ucScanCode)
{
    stuKeyCode keycode;
    unsigned char ucTeclaLevantada;
    static unsigned char staucScanCodeExtendido = 0;

    if (!bActivarTeclado)
        return; //Teclado desactivado

//    vFnImprimir("[%x]",ucScanCode);

    //Tratamiento de scancodes extendidos (de dos bytes: e0 xx)
    if( ucScanCode == 0xE0 ) {
        staucScanCodeExtendido = 1;
        return;
    }

    ucTeclaLevantada = (ucScanCode & 0x80)>>7;
    ucScanCode &= 0x7F;

    if( ucScanCode >= LARGO_MAPA_TECLADO)
        return; //Evitamos direccionar fuera de los keymaps

    //Se convierte el scancode a keycode
    if( staucScanCodeExtendido ) {
        vFnScanCodeExtAKeyCode( &keycode, ucScanCode );
        staucScanCodeExtendido = 0;
    } else {
        vFnScanCodeAKeyCode( &keycode, ucScanCode );
    }

    //Se determina si el keycode corresponde a una tecla especial, aquellas que
    //modifican los flags: shift, ctrl, alt, altgr, caps, num
    if( iFnTeclaEspecial(keycode, ucTeclaLevantada) ) 
        return;

    //Se ignoran los scancodes por tecla_levantada
    if( ucTeclaLevantada )
        return;
   
    if (keycode.ascii) 
    {
        while (iSemaforoBufferTeclado != 0);
        iSemaforoBufferTeclado = 1;

        if( iFnCombinacionTeclasEspecial(keycode) ) {
            //Si llega, por ejemplo, CTRL+C, no lo colocamos en el buffer, ya
            //que la funcion anterior se encargo de manejarlo
            iSemaforoBufferTeclado = 0;
            return;
        }

        stBufferTeclado[iLlenarIndice++] = keycode.ascii;
        if (iLlenarIndice > (TAMANIO_BUFFER_TECLADO - 1))
            iLlenarIndice = 0;
        iSemaforoBufferTeclado = 0;
    }

}


/**
\fn void vFnScanCodeAKeyCode( stuKeyCode* pKeyCode, unsigned char ucScanCode)
\brief Traduce los scancodes del teclados a los keycodes de Sodium. Solo para scancodes simples (1 byte)
\param pKeyCode (Salida) Keycode correspondiente de Sodium
\param ucScanCode Scancode recibido desde el teclado
\date 14/07/2008
*/
void vFnScanCodeAKeyCode( stuKeyCode* pKeyCode, unsigned char ucScanCode)
{
    if(shift)
        *pKeyCode = stuMapaTeclado.stashMatrizShifted[ucScanCode];
    else if(altgr)
        *pKeyCode = stuMapaTeclado.stashMatrizAltGred[ucScanCode];
    else
        *pKeyCode = stuMapaTeclado.stashMatrizNormal[ucScanCode];

    //Se tratan especialmente las teclas del KEYPAD
    if( pKeyCode->tipo == KT_PAD ) {
        vFnKeyCodeNumPad( pKeyCode );
        return;
    }

    //Si son letras, CAPS invierte el estado del SHIFT
    if( (pKeyCode->tipo == KT_LETTER) && caps) {
        if( shift )
            *pKeyCode = stuMapaTeclado.stashMatrizNormal[ucScanCode];
        else
            *pKeyCode = stuMapaTeclado.stashMatrizShifted[ucScanCode];
    }
}


/**
\fn void vFnKeyCodeNumPad( stuKeyCode* pKeyCode )
\brief Traduce los keycodes de las teclas del pad numerico segun el estado de la tecla NUMLOCK
\param pKeyCode Keycode a traducir / Keycode traducido
\date 14/07/2008
*/
void vFnKeyCodeNumPad( stuKeyCode* pKeyCode )
{
    static unsigned short stus_keypad_con_num[20] = {
        K(KT_PAD,'0'),          K(KT_PAD,'1'),          K(KT_PAD,'2'),
        K(KT_PAD,'3'),          K(KT_PAD,'4'),          K(KT_PAD,'5'),
        K(KT_PAD,'6'),          K(KT_PAD,'7'),          K(KT_PAD,'8'),
        K(KT_PAD,'9'),          K(KT_PAD,'+'),           K(KT_PAD,'-'),
        K(KT_PAD,'*'),          K(KT_PAD,'/'),           K(KT_PAD,TECLA_ENTER),
        K(KT_PAD,'.'),          K(KT_PAD,'.'),          K(KT_PAD,241),
        K(KT_PAD,'('),          K(KT_PAD,')')  };
    static unsigned short stus_keypad_sin_num[20] = {
        K(KT_PAD,TECLA_INS),    K(KT_PAD,TECLA_END),    K(KT_PAD,TECLA_ABA),
        K(KT_PAD,TECLA_PGDN),   K(KT_PAD,TECLA_IZQ),    0,
        K(KT_PAD,TECLA_DER),    K(KT_PAD,TECLA_HOME),   K(KT_PAD,TECLA_ARR),
        K(KT_PAD,TECLA_PGUP),   K(KT_PAD,'+'),          K(KT_PAD,'-'),
        K(KT_PAD,'*'),          K(KT_PAD,'/'),          K(KT_PAD,TECLA_ENTER),
        K(KT_PAD,'.'),          K(KT_PAD,'.'),          K(KT_PAD,241),
        K(KT_PAD,'('),          K(KT_PAD,')')  };
    unsigned char ucPosicion;

    ucPosicion = pKeyCode->ascii;

    //Fuera de rango
    if( ucPosicion >= 20 ) {
        pKeyCode->ascii = 0;
        pKeyCode->tipo  = 0;
        return;
    }

    if( num ) {
        pKeyCode->ascii = KVAL( stus_keypad_con_num[ucPosicion] );
        pKeyCode->tipo  = KTYP( stus_keypad_con_num[ucPosicion] );
    } else {
        pKeyCode->ascii = KVAL( stus_keypad_sin_num[ucPosicion] );
        pKeyCode->tipo  = KTYP( stus_keypad_sin_num[ucPosicion] );
    }
}


/**
\fn void vFnScanCodeExtAKeyCode( stuKeyCode* pKeyCode, unsigned char ucScanCode)
\brief Traduce los scancodes del teclados a los keycodes de Sodium. Solo para scancodes extendidos (2 bytes, de la forma 0xE0 0xXX )
\param pKeyCode (Salida) Keycode correspondiente de Sodium
\param ucScanCode Scancode recibido desde el teclado
\date 14/07/2008
*/
void vFnScanCodeExtAKeyCode( stuKeyCode* pKeyCode, unsigned char ucScanCode)
{
    static unsigned short stus_e0_keys[128] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, K(KT_PAD,TECLA_ENTER), K(KT_SHIFT,TECLA_CONTROL)/*DERECHO*/, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, K(KT_PAD,'/'), 0, 0/*PRINTSCREEN*/,
        K(KT_SHIFT,TECLA_ALTGR), 0, 0, 0, 0, K(KT_FN,TECLA_F13), K(KT_FN,TECLA_F14), 0/*AYUDA*/,
        0 /*E0_DO??*/, K(KT_FN,TECLA_F17), 0, 0, 0, 0, 0/*BREAK*/, K(KT_SPEC,TECLA_HOME),
        K(KT_CUR,TECLA_ARR), K(KT_FN,TECLA_PGUP), 0, K(KT_CUR,TECLA_IZQ), 0/*E0_OK??*/, K(KT_CUR,TECLA_DER), 0/*E0_KPMINPLUS??*/, K(KT_SPEC,TECLA_END),
        K(KT_CUR,TECLA_ABA), K(KT_FN,TECLA_PGDN), K(KT_FN,TECLA_INS), K(KT_FN,TECLA_DEL), 0, 0, 0, 0,
        0, 0, 0, 0/*E0_MSLW??*/, 0/*E0_MSRW??*/, 0/*E0_MSTM??*/, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0/*MACRO*/,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    pKeyCode->ascii = KVAL( stus_e0_keys[ucScanCode] );
    pKeyCode->tipo  = KTYP( stus_e0_keys[ucScanCode] );
}


/**
\fn int iFnTeclaEspecial (stuKeyCode keycode, unsigned char ucTeclaLevantada)
\brief Determina si el keycode corresponde a una tecla especial (ctrl, shift, alt, altgr), modifica los flags globales, y retorna si el keycode era de una tecla especial
\param keycode Keycode a evaluar
\param ucTeclaLevantada Flag que indica si se recibio el keycode por oprimir o por soltar una tecla
\date 14/07/2008
*/
int iFnTeclaEspecial (stuKeyCode keycode, unsigned char ucTeclaLevantada)
{
    static int stabTeclaEspecialLevantada = 1;
    int valor;

    //vFnImprimir("%d-%d,%d s=%d c=%d a=%d g=%d\n", keycode.tipo,keycode.ascii,ucTeclaLevantada,shift,ctrl,alt,altgr);

    //Si lo que se recibio fue una tecla_levantada, aqui solo importa si se
    //trata de las teclas ctrl, shift, alt, altgr
    if( ucTeclaLevantada && ( keycode.tipo != KT_SHIFT) )
        return 0;

    switch (keycode.tipo) {
        case KT_SPEC:
            if (stabTeclaEspecialLevantada) {
                switch (keycode.ascii){
                    case CAPS_LOCK:
                        caps = !caps;
                        break;
                    case NUM_LOCK:
                        num = !num;
                        break;
                    default:
                        return 0;
                }
            }
            stabTeclaEspecialLevantada = !stabTeclaEspecialLevantada;
            return 1;
            break;
        case KT_SHIFT:
            //Si se recibio una tecla ctrl, shift, alt o altgr 
            //se decide si se presiono o levanto la tecla
            if(ucTeclaLevantada == 0)
               valor = 1;
            else
               valor = 0;
            
            //Ahora se ve de que tecla se trata
            switch (keycode.ascii) {
                case SHIFT_DERECHO:
                case SHIFT_IZQUIERDO:
                    shift = valor;
                    return 1;
                case TECLA_CONTROL:
                    ctrl = valor;
                    return 1;
                case TECLA_ALTGR:
                    altgr = valor;
                    return 1;
                case TECLA_ALT:
                    alt = valor;
                    return 1;
                default:
                    return 0;
            }
            break;
    }

    return 0;
}


/**
\brief Detecta si se presiono una combinacion especial de teclas y actua en consecuencia
\param keycode Keycode recibido
\return Retorna 1 si se dio una combinacion especial, 0 si no
\date 22/10/2008
\note Combinaciones tratadas: CTRL+c
*/
int iFnCombinacionTeclasEspecial(stuKeyCode keycode) {
    unsigned long ulPosFg;

    /* Si la combinacion de teclas presionadas es CTRL+c, enviamos SIGINT al
     * proceso que corre Foreground
     */
    if( ctrl && keycode.ascii == 'c') {
        //vFnImprimir("CTRL+C");
        ulPosFg = iFnBuscaPosicionProc(ulPidProcesoForeground);
        /* Si Shell no esta esperando a ningun proceso, no tiene sentido
         * despertarlo
         */
        if ( pstuPCB[1].iEstado == PROC_ESPERANDO ) {
            /* Ya que puede que el proceso actual no ejecute mas (si es el que
             * tiene que recibir la senial, limpiamos el 'semaforo' de teclado
             */
            iSemaforoBufferTeclado = 0;

            //Enviamos SIGINT al Proceso Foreground
            lFnSysKill(ulPidProcesoForeground, SIGINT);
        }
        return 1;
    }

    return 0;
}

/**
\fn void * pvFnAbrirArchivoKeymap ()
\brief Busca el archivo KEYMAPS.BIN en el filesystem y retorna un puntero a su ubicacion
\return Retorna un puntero a la ubicacion del archivo KEYMAPS.BIN
\date 14/07/2008
*/
void * pvFnAbrirArchivoKeymap ()
{
	stEntradaLS *pstEntLs;
    stDirectorio *pstDirBusqueda;

    if (iFnObtenerDirectorio("/mnt/usr", &pstDirBusqueda) < 0) {
        vFnImprimir("\nSodium Dice: Error! Directorio /mnt/usr no existe");
        return NULL;
    }
    if( iFnBuscarArchivo(pstDirBusqueda, "KEYMAPS.BIN", &pstEntLs) < 0) {
        vFnImprimir("\nSodium Dice: Error! El archivo KEYMAPS.BIN no existe");
        return NULL;
    }

    return pstEntLs;
}


/**
\fn int iFnCambiaTecladoI (unsigned int uiNumeroDefinicion)
\brief Carga en memoria la definicion de teclado indicada, copiandola del archivo KEYMAPS.BIN
\param uiNumeroDefinicion Numero de definicion dentro del archivo (comienza desde 1)
\date 14/07/2008
*/
int iFnCambiaTecladoI (unsigned int uiNumeroDefinicion)
{
    stEntradaLS *pstEntLs;
    stuDefinicionTeclado *pstuPosMemoriaKeymap;

    if (uiNumeroDefinicion > CANTIDAD_KEYMAPS || uiNumeroDefinicion < 1) {
        vFnImprimir("\nSodium Dice: Error! La definicion de teclado no existe");
        return 1;
    }

    pstEntLs = (stEntradaLS*) pvFnAbrirArchivoKeymap();
    pstuPosMemoriaKeymap = (stuDefinicionTeclado*)(DIR_LINEAL(pstEntLs->wSeg, pstEntLs->wOff)) + (uiNumeroDefinicion-1);

    ucpFnCopiarMemoria( (unsigned char*)&stuMapaTeclado,
                        (unsigned char*)pstuPosMemoriaKeymap,
                        sizeof(stuDefinicionTeclado));

    vFnImprimir("\nSe cargo la distribucion de teclado \"%s\"",
                    (pstuPosMemoriaKeymap)->sNombre );
 
    return 0;
}

/**
\fn int iFnCambiaTecladoS (char * sCodigoDefinicion)
\brief Carga en memoria la definicion de teclado indicada, copiandola del archivo KEYMAPS.BIN
\param sCodigoDefinicion Nombre de la distribucion de teclado, por ejemplo "us", "es", etc
\date 15/07/2008
*/
int iFnCambiaTecladoS (char * sCodigoDefinicion)
{
    stEntradaLS *pstEntLs;
    stuDefinicionTeclado * pstuPosMemoriaKeymap;
    int i,comparacion;

    pstEntLs = (stEntradaLS*) pvFnAbrirArchivoKeymap();
    pstuPosMemoriaKeymap = (stuDefinicionTeclado*)(DIR_LINEAL(pstEntLs->wSeg, pstEntLs->wOff));

    for(i=0; i<CANTIDAD_KEYMAPS; i++) {
       comparacion=iFnCompararCadenas(sCodigoDefinicion,(pstuPosMemoriaKeymap)->sCodigo);
       if(comparacion == 1)
          return iFnCambiaTecladoI(i+1);
       pstuPosMemoriaKeymap++;
    }
    return 1;
}
/**
\fn void vFnListarKeymaps ()
\brief Imprime por pantalla una lista de las definiciones de teclado presentes en el archivoo KEYMAPS.BIN, indicando su numero y nombre
\date 14/07/2008
*/
void vFnListarKeymaps ()
{
    stEntradaLS *pstEntLs;
    stuDefinicionTeclado * pstuPosMemoriaKeymap;
    int i;

    pstEntLs = (stEntradaLS*) pvFnAbrirArchivoKeymap();
    pstuPosMemoriaKeymap = (stuDefinicionTeclado*)(DIR_LINEAL(pstEntLs->wSeg, pstEntLs->wOff));

    for(i=0; i<CANTIDAD_KEYMAPS; i++){
        vFnImprimir("\n  %d  - %s: %s",i+1,(pstuPosMemoriaKeymap)->sCodigo,(pstuPosMemoriaKeymap)->sNombre);
        pstuPosMemoriaKeymap++;
    }
}


/**
\fn char cFnGetChar ()
\brief Saca un caracter del buffer de teclado
\return Caracter del buffer de teclado
\date 14/07/2008
*/
char cFnGetChar ()
{
    char cCaracter;
    static int staiVaciarIndice = 0;
    int iFlags;

    // indica si las interrupciones estaban habilitadas al entrar a la funcion
    int bInterrupcionesDeshabilitadas = 0;

    __asm__ ("pushf\n pop %%eax": "=a" (iFlags):);

    // si estaban habilitadas, aqui se deshabilitan
    if (!(iFlags & 0x200))
    {
        __asm__ ("sti"::);
        bInterrupcionesDeshabilitadas = 1;
    }

    //Nos quedamos en espera activa mientras no hayan caracteres nuevos en el
    //buffer
    //imprimir("\ngetchar: Antes de espera activa");
    while (staiVaciarIndice == iLlenarIndice);

    //vFnImprimir("\ngetchar: Luego de espera activa. staiVaciarIndice=%d, iLlenarIndice=%d",staiVaciarIndice, iLlenarIndice);
    while (iSemaforoBufferTeclado != 0);
    iSemaforoBufferTeclado = 1;

    cCaracter = stBufferTeclado[staiVaciarIndice++];
    //imprimir("\ngetchar: cCaracter: %c, staiVaciarIndice=%d, iLlenarIndice=%d",cCaracter, staiVaciarIndice, iLlenarIndice);

    if (staiVaciarIndice > (TAMANIO_BUFFER_TECLADO - 1))
    {
        staiVaciarIndice = 0;
        //imprimir("\ngetchar: Llegamos al final del buffer.   staiVaciarIndice=%d, iLlenarIndice=%d", staiVaciarIndice, iLlenarIndice);
    }
    
    //imprimir("\ngetchar: Fin, retornamos Caracter: %c",cCaracter);
    iSemaforoBufferTeclado = 0;

    if (bInterrupcionesDeshabilitadas)
        __asm__ ("cli"::);

    return (cCaracter);
}


/**
\fn char cFnPausa ()
\brief Imprime la sentencia de cFnPausa y espera la presión de cualquier tecla
\return Caracter correspondiente a la tecla presionada
\date 06/10/2004
*/
char cFnPausa ()
{
    char cCaracter;

    vFnImprimir ("\nPresione cualquier tecla para continuar...");
    cCaracter = cFnGetChar ();
    return (cCaracter);
}

 
