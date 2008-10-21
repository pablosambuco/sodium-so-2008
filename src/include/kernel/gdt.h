/**
\file kernel/gdt.h
\brief Funciones de biblioteca, constantes y variables para el manejo de la GDT
*/
#ifndef GDT_H
#define GDT_H

#include <kernel/definiciones.h>
#include <kernel/pcb.h>

#define DELAY __asm("nop\n"); /*!< Delay de un ciclo de instruccion */

#define D_LDT   0x200   /*!< Segmento LDT */
#define D_TASK  0x500   /*!< Task gate (Para funciones de mayor privilegio) */
#define D_TSS   0x900   /*!< TSS  (Selector de segmento de tarea ) */
#define D_CALL  0x0C00  /*!< call gate para 386 */
#define D_INT   0x0E00  /*!< interrupt gate para 386 */
#define D_TRAP  0x0F00  /*!< trap gate para 386 */
#define D_DATA  0x1000  /*!< Segmento de Datos */
#define D_CODE  0x1800  /*!< Segmento de Codigo */

#define D_DPL3		0x6000	/*!< DPL3 o mascara para DPL (Nivel de privilegio de descriptor) */
#define D_DPL2		0x4000	/*!< DPL2 idem  */
#define D_DPL1		0x2000	/*!< DPL1 idem  */
#define D_PRESENT	0x8000	/*!< Presente (indica si esta cargado en memoria) */
#define D_NOT_PRESENT	0x8000	/*! No Presente (lo recíproco) */
#define D_BIG		0x40    /*! Modo por defecto para 32 bits   */
#define D_4KB		0x80    /*! Granularidad 4Kb */

/*! PARA SEGMENTOS DE DATOS */
#define D_WRITE		0x200    /*! Escritura habilitada */

/*! PARA SEGMENTOS DE CODIGO */
#define D_READ		0x200    /*! Lectura habilitada */

/**
	\note Se usa el atributo NOALIGN para que el compilador no deje espacios intermedios 
	entre los campos de la estructura, o asigne 4 bytes donde sólo le pedimos 1 
*/
typedef struct {
    unsigned short usLimiteBajo;    /*!< limite bajo 0..15   */
    unsigned short usBaseBajo;      /*!< base bajo 0..15     */
    unsigned char ucBaseMedio;      /*!< base media 16..23   */
    /**
    \note (es 1 byte que contiene Type (bits 8-11) indica si el descriptor es de codigo, de datos o de sistema, Bit 'S' que indica con cero si el descriptor es de sistema o con uno, si es de codigo o de datos. DPL (Define nivel de previlegio del segmento. y el bit 'P' indica si el segmento está en memoria principal o no 
    */
    unsigned char ucAcesso;         /*! byte de acceso*/
    unsigned int bitLimiteAlto:4;   /*!< limite alto 16..19   */

    /**
    \note granularidad que contiene: AVL 2 bits el numero 20 diponibles y reservado para software de sistema, el bit 21 deberia ser siempre cero. D/B especifica distintas opciones segun sea el segmento de codigo,de stack o de datos. G Banderita de granularidad de 1 bit que determina si el limite del segmento se especifica de a un Byte o de a 4 KByte.
    */
    unsigned int bitGranularidad:4;  
    unsigned char usBaseAlto;       /*!< base alto24..31    */

} NOALIGN stuGDTDescriptor;        


/**
 \brief Estructura que mantiene la GDT
*/
typedef struct {
    stuGDTDescriptor stuGdtDescriptorDescs[8192]; /*!< Vector descriptores GDT*/
} stuEstructuraGdt;

stuEstructuraGdt *pstuTablaGdt; /*!< puntero a la GDT */


/** 
*  \brief Definicion y estructura de registros del FPU. Contiene los registros del FPU en el orden en que se usan en "fsave" y "frstor"
*/
typedef struct {
  unsigned int control;	//Control word
  unsigned int status;	//Status word
  unsigned int tag;		//Tag word
  unsigned int ip;		//Instruction offset
  unsigned int cs;		//Instruction selector
  unsigned int dp;		//Data offset
  unsigned int ds;		//Data selector
  u80 st[8]; 			//Pila:	8 registros de 80 bits 
} NOALIGN stuFpu;


/** 
*  \brief Definicion y estructura de la TSS.
*  
*  TSS para 386.
*  Contiene los registros del procesador que se almacenaran con los context switch
*/
typedef struct {          
  unsigned int uiEnlace;  /*!< Puntero a la TSS anterior, en caso de anidamientos */
  unsigned int esp0;      
  unsigned int ss0;
  unsigned int esp1;
  unsigned int ss1;
  unsigned int esp2;
  unsigned int ss2;
  unsigned int cr3;
  unsigned int eip;
  unsigned int uiEBandera;
  unsigned int eax;
  unsigned int ecx;
  unsigned int edx;
  unsigned int ebx;
  unsigned int esp;
  unsigned int ebp;
  unsigned int esi;
  unsigned int edi;
  unsigned int es;
  unsigned int cs;
  unsigned int ss;
  unsigned int ds;
  unsigned int fs;
  unsigned int gs;
  unsigned int ldt;
  unsigned int trapbit;
  unsigned int uIOMapeoBase;
  char         espacio0[TSS_TAMANIO_STACK_R0];
  char         espacio1[TSS_TAMANIO_STACK_R1];
  char         espacio2[TSS_TAMANIO_STACK_R2];
  stuFpu fpu; //estructura de registros del FPU
}stuTSS;


typedef struct {          
    void * pvPuntoCarga;
    unsigned int uiTamanioTexto;
    unsigned int uiTamanioDatosInicializados;
    unsigned int uiTamanioStack;
    char stNombre [13];
} stuInfoEjecutable;



#define TOTAL_ENTRADAS_GDT (sizeof(stuEstructuraGdt) / sizeof(stuGDTDescriptor))



stuPCB pstuPCB[CANTMAXPROCS]; /*!<Vector de procesos del sistema (deberia ser una cola, pero por ahora no) */
stuTSS stuTSSTablaTareas[CANTMAXPROCS]; /*!<Vector de TSS del sistema (deberia ser una cola, pero por ahora no) */


void vFnGdtInicializar(dword);

int iFnBuscaPosicionProc(unsigned long);
unsigned int uiFnSetearBaseLimiteDescriptor(int,unsigned int,unsigned int);

unsigned int iFnAgregarDescriptorGDT(unsigned int,unsigned int,unsigned int,int);

int iFnBuscarPCBLibre();
unsigned int uiFnBuscarEntradaGDTLibre();

int iFnCrearTSSTareaEspecial (void*,int,unsigned int,unsigned int,unsigned int);
int iFnCrearTSS (void*,void*,int,unsigned int,unsigned int,unsigned int);

int iFnCrearPCB(int,void*,char*,unsigned int,unsigned int,unsigned int,unsigned int,unsigned int,unsigned int,unsigned int,unsigned int, unsigned int);

int iFnNuevaTareaEspecial(void *, char*);
int iFnInstanciarIdle();
int iFnInstanciarInit();

int iFnLeerCabeceraEjecutable(char* stArchivo, stuInfoEjecutable* pstuInfo);

int iFnCrearProceso(char *);
int iFnRedimensionarProceso(unsigned long ulPid, unsigned long ulBrk);

int iFnDuplicarProceso(unsigned int uiProcPadre); /*!< devuelve el indice de PCB/TSS del proceso nuevo */
int iFnEliminarProceso(unsigned int uiProcesp); /*!< elimina todos los recursos usados por el proceso */
int iFnReemplazarProceso(unsigned long, char *);
int iFnClonarProceso();

#endif //GDT_H
/** \note 
 * los selectores de segmento se mantienen, lo que hay que hacer es meterle 
 * los valores que queremos reemplazar directamente en el stack, en las 
 * posiciones donde fueron resguardados, de manera tal que cuando el proceso 
 * vaya volviendo a ejecucion "normal" (dentro de su espacio de direcciones),
 * restaure los registros a los valores que querramos 
 * \note primero nos paramos en el inicio del stack temporal dentro del kernel para 
 * este proceso 
 */

