/*!
 * \file memoria_k.H
 * \brief Biblioteca de funciones y definiciones para memoria dinamica segmentada
 */
#ifndef _MEMORIA_S_H
#define _MEMORIA_S_H

#include <video.h>

//t_nodo InicioSegmentosLibres;


































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

