/**
 * \file lotes.h
 * \brief Definiciones de los lotes creados en el sistema
 */

#ifndef	__LOTES_H__
#define __LOTES_H__
#include <kernel/definiciones.h>
#define CANTMAXTAREAS	20

/**
 * \brief Estados posibles de la tarea 
 */
typedef enum {
	Terminada, Lista, Ejecutando, Esperando
} enuEstadoTarea;

/**
 * \brief Informacion de las tareas para mostrar en el log
 */
typedef struct {
	dword		dwTiempoProgramado; //hacer clockticks + t
	unsigned int	uiIdProceso;
	unsigned int	uiTiempoCPU;
	unsigned int	uiTiempoHDD;
	unsigned int	uiTiempoFDD;
	unsigned int	uiTiempoIO;
	unsigned int	uiTiempoEspera;
	unsigned int	uiTiempo;
	enuEstadoTarea	enuEstado;

} stTareaDef;

/**
 * \brief Informacion referente a los lotes
 */
typedef struct {
	unsigned int 	uiIdLote;
	char 		strNombreLote[20];
	stTareaDef 	stTareas[CANTMAXTAREAS];
	
} stLoteDef;

int iFnCargarLote(char* strNombreArchivo);

int iFnEliminarLote(char* strNombreLote);


#endif
