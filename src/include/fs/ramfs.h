/** \file ramfs.h
 *   \brief Agrupa funciones y constantes para el disco virtual.
 */

#ifndef __RAMFS_H__
#define __RAMFS_H__

#include <kernel/definiciones.h>

#define CANTMAXDISCOSRAM 3 /*!< Discos en RAM */
#define CANTMAXDIRENT	 20 /*!< Cantidad de directorios */
#define DISCO_USR 	 0 /*!< numero de disco de usuario */
#define DISCO_LOG 	 1 /*!< numero de disco de log */
#define DISCO_LOTES 	 2 /*!< numero de disco de lotes */

/** 
 * \brief Dado un segmento y offset calcula la dirección lineal que representa.
 * \note Ya que estamos trabajando en modo protegido, es más útil que usar la
 * dirección de memoria partida en segmento y offset, utilizada en el listado
 * de archivos a cargar por el loader del sodium. 
 * \param seg segmento.
 * \param offs offset lineal.
 * \returns Direccion lineal de la posicion pasada.
 */
#define DIR_LINEAL(seg,offs) ( (seg) * 16 + (offs) )

/**
	\brief Entrada del archivo.
*/
typedef struct {
	char strNombreArchivo[11]; /*!< Nombre del archivo de 8+3 caracteres */
	word wSeg; /*!< Segmento donde se ubica */
	word wOff; /*!< offset dentro del segmento donde se encuentra */
	dword dwTamanio; /*!< tamanio del archivo */
} NOALIGN stEntradaLS;

/**
	\brief Entrada del archivo.
*/
typedef struct {
	stEntradaLS stEnt[CANTMAXDIRENT]; /*!< Entradas de directorios */
} NOALIGN stDirectorio;

/**
	\brief Informacion pertinente a los discos
*/
typedef struct {
	char strEtiqueta[20]; /*!< Label del disco */
	char strPMontaje[44]; /*!< Punto de montaje del disco */
	dword dwDireccionInicial; /*!< Direccion de memoria inicial del disco */
	dword dwTamanio; /*!< Tamanio del disco */
	stDirectorio *pstDirRaiz; /*!< Puntero al directorio raiz del disco */
	dword dwEntradasRaiz; /*!< Cantidad de entradas raiz */
	u8    bActivo; /*!< indicador de disco activo */
} stDiscoRAM;

int iFnRamFsInit(void);

int iFnNormalizarNomArch(const char *strNomArch83, char *strNomArch);

int iFnBuscarArchivo(stDirectorio * pstDir, char *strNombreArchivo, stEntradaLS ** ppstEntradaLSArchEnc);

int iFnObtenerDirectorio(char *strPMontaje, stDirectorio ** ppstDir);

int iFnObtenerDisco(char *strPMontaje, stDiscoRAM **ppstDisco);

int iFnRamFsLs(char *strParams);


#endif
