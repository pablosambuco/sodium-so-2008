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


void vFnInicializarMemoriaSegmentada();
void * pvFnReservarSegmento(unsigned int);
void vFnLiberarSegmento( void *, unsigned int);
void vFnListarBloquesLibres();

#endif

