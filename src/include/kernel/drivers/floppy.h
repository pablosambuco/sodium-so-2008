/** \file floppy.h
 *  \brief Agrupa funciones y constantes para el controlador de la disquetera.
 */

#ifndef FLOPPY_H
#define FLOPPY_H

#include <kernel/drivers/dma.h>

#define Reg_Estado_FDC	0x3F4
#define Reg_Datos_FDC		0x3F5
#define Reg_Sal_Dig 		0x3F2
#define Reg_Ent_Dig 		0x3F7

#define Sectores_Cilindro 36 /*!< Cantidad de sectores por cilindro*/
#define Sectores_Pista		18 /*!< Cantidad de pistas*/


typedef struct {u8 RST0; u8 Valido;} ST0_t;
typedef struct {u8 RST1; u8 Valido;} ST1_t;
typedef struct {u8 RST2; u8 Valido;} ST2_t;

/**
	\brief Estructura con la informacion CHS del disquette
*/
typedef struct
{
	u8 Cilindro; /*!< Cilindros*/
	u8 Cabeza;/*!< Cabezas*/
	u8 Sector;/*!< Sectores*/
}
CHS;

/**
	\brief Codigos de lecturas
*/
typedef enum 
{
	Termin_Normal, Term_Anormal, Com_No_Finalizado, Com_Invalido
}Estado;



u8	 leerSector					(u32 Buffer, u16 NBloque);
u8	 escribirSector			(u32 Buffer, u16 NBloque);
u8 	 operExitosa				();
void prenderMotor				();
void apagarMotor				();
u8	 envComRecalibrado	();
u8	 envComDesplCabeza	(u8 Cilindro, u8 Cabeza);
u8	 envComLectura			(u8 Cilindro, u8 Cabeza, u8 Sector);
u8	 envComEscritura		(u8 Cilindro, u8 Cabeza, u8 Sector);
void enviarDatosOper		(u8 Cilindro, u8 Cabeza, u8 Sector);
u8	 leerEstadoFDC			();
void leerST0						();
u8	 leerST3						();
u8	 vFnImprimirST0				();
void vFnImprimirST1				();
void vFnImprimirST2				();
void vFnImprimirST3				();
void vFnImprimirEstadoFDC	();
u8 	 listoPES						();
u8 	 esperandoByteProc	();
u8 	 bytePProcesador		();
u8 	 FDCOcupado					(); //FDCOcupado no funciona como se espera, usar esperandoByteProc() 
u8 	 unidadOcupada			(); //y bytePProcesador() en vez de este.
void obtResOper					();
void invalidarEstados		();
CHS  LBAaCHS						(u16 NBloque);


#endif
