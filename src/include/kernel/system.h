/**
	\file kernel/system.h
	\brief Contiene constantes y funciones del sistema
*/
#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#define INT_TIMER 32
#define INT_KEYBOARD 33
#define INT_SYSCALL 0x80

// Constantes para el control del timer del sistema
#define TIMER_DIVISOR_FREQ_DEFAULT	0X0FFF
#define TIMER_FREQ_REAL			1193181l
#define TIMER_TICKS_UPDATE_DEFAULT	10000
#define TIMER_MAXERROR_DEFAULT		432
#define TIMER_MILISEGUNDOS_POR_TICK_DEFAULT	(1000 * TIMER_DIVISOR_FREQ_DEFAULT) / TIMER_FREQ_REAL

unsigned long int uliClockTick; /*!< Define y almacena el tick de reloj (se incrementa en uno cada int de timer)*/


typedef struct
{
    unsigned int :16;
    unsigned int TSSSegmentSelector :16;
    unsigned int :8 ;
    unsigned int Type :5;  //Deberia ser 0x05
    unsigned int DPL :2;
    unsigned int P :1;
    unsigned int :16;
} __attribute__((packed))  t_TaskGate;

typedef struct
{
    unsigned int Offset1 :16;
    unsigned int SegmentSelector :16;
    unsigned int :5 ;
    unsigned int Type :8;  //Deberia ser 0x70
    unsigned int DPL :2;
    unsigned int P :1;
    unsigned int Offset2 :16;
} __attribute__((packed)) t_InterruptGate;

typedef struct
{
    unsigned int Offset1 :16;
    unsigned int SegmentSelector :16;
    unsigned int :5 ;
    unsigned int Type :8;  //Deberia ser 0x78
    unsigned int DPL :2;
    unsigned int P :1;
    unsigned int Offset2 :16;
} __attribute__((packed)) t_TrapGate;

typedef union
{
    t_TaskGate TaskGate;
    t_InterruptGate InterruptGate;
    t_TrapGate TrapGate;
}stuIDTDescriptor;

typedef struct
{
    stuIDTDescriptor IDTDescriptor[256];
} stuIDT;


typedef struct
{
    dword dwLimite :16;
    dword dwBase :32;
} t_IDTRegister;

stuIDT *pstuIDT;  /*!<Puntero a la IDT */
unsigned char ucFnObtenerScanCode(); /*!< Maneja el protocolo de comunicacion con el teclado, para obtener el scancode del último evento*/
void vFnIniciarTeclado();
void vFnIniciarExcepciones();/*!<carga las excepciones de modo protegido*/
void vFnIniciarIDT();  /*!<Remapea el PIC, inicializa la IDT y habilita las inter.*/
void vFnHandlerGenerico();     /*!<Handler generico para las interrupciones*/
void vFnHandlerTimer();        /*!<Handler para manejar el timer */
void vFnPlanificador();        /*!<Selecciona el siguiente proceso a ejecutar y le cede la CPU*/
void vFnHandlerTeclado();     /*!<Handler para manejar el teclado*/

long vFnHandlerSyscall( long eax,
			long ebx,
			long ecx,
			long edx );     /*!<Handler para entrar a cualquier syscall*/

void vFnLTR(unsigned int uiSelector);
unsigned int uiFnSTR(void);



#endif //_SYSTEM_H_


