/*!
 *  \file paginas.h
 *  \brief Estructuras y definiciones para el mantenimiento de las paginas en memoria
 */

#ifndef paginas_h_
#define paginas_h_

/*!
 * \brief Estructura que contiene datos de las tablas de paginas
 * \todo expandir comentarios
 */
struct stuTablaPagina{
		unsigned int uiFrame;
		unsigned int bUsado;
		};

/*!
 * \brief Estructura atributos para armar un mapa de bits
 * \todo expandir comentarios
 */
struct stuBitMap	{
		unsigned 	bit0: 1,
				bit1: 1,
				bit2: 1,
				bit3: 1,
				bit4: 1,
				bit5: 1,
				bit6: 1,
				bit7: 1;
		}__attribute__((packed));


int iTamanioMemoria; /*!< Tamanio de la memoria*/
int iLimCantPaginasProceso; /*!< Cantidad de paginas maxima por proceso*/


struct stuTablaPagina* pstuTablaPaginaFnAsignarPaginas (unsigned int);

void vFnIniciarMapaBits(int , int);

/*!
 * \brief esta funcion asigna los huecos al proceso
 */
int iFnBuscarVacio();

/*!
 * \brief esta funcion verifica que halla la suf. cant de frames libres para poder alocar un proceso
 */
int iFnVerificarSuficientes(int); 

void vFnLimpiarFrame(int);

/*!
 * \brief funcion que la llama eliminar tarea
 */
int iFnLimpiarMemoria(int, struct stuTablaPagina *); 

void vFnImprimirTablaPaginas(int, struct stuTablaPagina *);

void vFnImprimirMapaBits();

int iFnDarDireccion( struct stuTablaPagina *, int, int, int); 

#endif
