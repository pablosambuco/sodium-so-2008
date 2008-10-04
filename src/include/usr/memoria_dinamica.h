/*!
 * \file memoria_s.h
 * \brief Biblioteca de funciones y definiciones para memoria dinamica segmentada
 */
#ifndef __MEMORIA_DINAMICA_H
#define __MEMORIA_DINAMICA_H


#define HEAP_TAMANIO_INICAL 4000
#define HEAP_INCREMENTO 4000


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
//    unsigned int uiOpciones;
} t_nodoOcupado;



void * malloc( unsigned int );
void * realloc( void *, unsigned int );
void free( void * );
void vFnInsertarBloqueLibreEnListaOrd( t_nodo * );
void * pvFnBuscarNodoAnteriorMemoriaLibre( unsigned int );
int iFnAgrandarHeap( int, char**, char** );
void vFnMostrarMemLibreHeap();

#endif
