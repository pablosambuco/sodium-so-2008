/*!
 * \file memoria_s.h
 * \brief Biblioteca de funciones y definiciones para memoria dinamica segmentada
 */
#ifndef _MEMORIA_S_H
#define _MEMORIA_S_H

#include <video.h>
#include <kernel/mem/memoria_k.h> //Para la estructura t_nodo... TODO Mover a un lugar comun

/**
 * @brief Puntero global a la lista (ordenada ascendente) de bloques de memoria libre
 */
t_nodo stuListaSegmentosLibres;

/**
 * @brief Variable global con la cantidad de bytes libres para procesos
 */
unsigned int uiTotalMemoriaLibre; //TODO - Mover a un lugar comun


void vFnInicializarMemoriaSegmentada();
void * pvFnBuscarNodoAnteriorMemoriaLibre(unsigned int);
void * pvFnReservarSegmento(unsigned int);
void vFnLiberarSegmento( void *, unsigned int);
void vFnInsertarBloqueLibreEnListaOrd( t_nodo * );
void vFnListarBloquesLibres();









/*!
 * \brief Prototipo de la Funcion "malloc".
 */
int iFnMalloc(int);


/*!
 * \brief Prototipo de la Funcion "free".
 */
int iFnFree(int);


/*!
 * \brief Prototipo de la Funcion "particionar_memoria".
 */
void vFnParticionarMemoria();

/*!
 * \brief Prototipo de la Funcion "mostrar_mem_seg".
 */
void vFnMostrarMemoriaSegmentada();

/*!
 * \brief Prototipo de la Funcion "segmento_max".
 */
int iFnSegmentoMaximo();

#endif

