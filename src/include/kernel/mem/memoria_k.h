/*!
 * \file memoria_k.H
 * \brief Biblioteca de funciones y definiciones para memoria dinamica paginada
 */
#ifndef _MEMORIA_K_H_
#define _MEMORIA_K_H_

#include <kernel/definiciones.h>

#define MEM_ALTA    0x00000000  //Primer bit
#define MEM_BAJA    0x00000001

#define MEM_KERNEL  0x00000000  //Segundo bit
#define MEM_USUARIO 0x00000002

#define MEM_DEFAULT (MEM_ALTA | MEM_KERNEL)

#define GET_MEM_ALTA_BAJA(X)        ( (X) & 0x00000001 )    //Primer Bit
#define GET_MEM_KERNEL_USUARIO(X)   ( (X) & 0x00000002 )    //Segundo Bit

/*!
 * \brief Tipo de datos para apuntar a un nodo libre
 */
typedef struct nodo     
{
    dword nTamanio;
    struct nodo *pNodoSig;
} t_nodo;

/*!
 * \brief Tipo de datos para apuntar a un nodo ocupado
 */
typedef struct nodoOcupado 
{
    dword nTamanio;
    unsigned int uiOpciones;
} t_nodoOcupado;


extern t_nodo InicioMemoria; /*!< Variable global utilizada para indicar el comienzo 
                                  de la memoria a asignar*/
/*!
 * \brief Funcion para la alocacion dinamica de memoria
 */
void *pvFnKMalloc(dword nTamanio, unsigned int uiOpciones);
/*!
 * \brief Funcion para la liberacion de memoria dinamica
 */
void vFnKFree(void * pvBloqueMemoria); 

void vFnIniciarKMem();
void vFnListarKMem();

t_nodo InicioMemoriaKernel; /*!< Esta estructura contiene un puntero a la direccion de 
memoria desde donde sera posible asignar memoria baja para el kernel.*/

t_nodo InicioMemoriaAlta; /*!< Esta estructura contiene un puntero a la direccion de 
memoria desde donde sera posible asignar memoria alta para el kernel, a otros procesos.*/

#endif //_MEMORIA_K_H_


