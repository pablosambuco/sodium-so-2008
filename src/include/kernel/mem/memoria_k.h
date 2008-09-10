/*!
 * \file memoria_k.H
 * \brief Biblioteca de funciones y definiciones para memoria dinamica paginada
 */
#ifndef _MEMORIA_K_H_
#define _MEMORIA_K_H_

#include <kernel/definiciones.h>

#define MEM_BAJA 1
#define MEM_ALTA 2

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
} t_nodoOcupado;


extern t_nodo InicioMemoria; /*!< Variable global utilizada para indicar el comienzo 
                                  de la memoria a asignar*/
/*!
 * \brief Funcion para la alocacion dinamica de memoria
 */
void *pvFnKMalloc(dword tamanio, int opciones);   
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


