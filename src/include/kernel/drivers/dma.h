/** \file dma.h
 *   \brief Agrupa funciones y constantes para el controlador DMA.
 *	\TODO hacer andar el DMA
 *	\TODO terminar documentacion de todos los campos definidos
 */

#ifndef DMA_H
#define DMA_H

#include <kernel/definiciones.h>
 
#define Reg_Dir_C0				0x00
#define Reg_Dir_C1				0x02
#define Reg_Dir_C2				0x04
#define Reg_Dir_C3				0x06
#define Reg_Cont_C0				0x01
#define Reg_Cont_C1				0x03
#define Reg_Cont_C2				0x05
#define Reg_Cont_C3				0x07
#define Reg_Estado_DMA		0x08
#define Reg_Comando_Esc		0x08
#define Reg_Comando_Lec		0x0A
#define Reg_Peticion			0x09
#define Reg_Modo					0x0B
#define Clr_Cont_Reg_Modo 0x0E /*!< Modo del DMA: Lectura (inport)*/
#define Reset_FF					0x0C 
#define Master_Clear			0x0D 
#define Clr_Reg_Mascara		0x0E 
#define RW_Reg_Mascara		0x0F 
#define Mascara_Canal			0x0A /*!< Mascara de Escritura del canal*/
#define Reg_Temporal			0x0D /*!< Mascara para Lectura */


#define Pag_C0	0x87 /*!< registro de pagina 0 del DMA*/
#define Pag_C1	0x83 /*!< registro de pagina 1 del DMA*/
#define Pag_C2	0x81 /*!< registro de pagina 2 del DMA*/
#define Pag_C3	0x82 /*!< registro de pagina 3 del DMA*/


//Modos DMA (Reg. Modo)
#define MDMADef		0x00 /*!< Demanda | Inc. Dir. | No Auto inic. | Tr. Verificacion | Canal 0 */

#define Tr_Simple	0x40 //01xxxxxxb
#define Tr_Bloque	0x80 //10xxxxxxb
#define Cascada		0xC0 //11xxxxxxb

#define Dec_Dir		0x20 //xx1xxxxxb

#define Auto_Inic 0x10 //xxx1xxxxb

#define Tr_Esc		0x04 //xxxx01xxb
#define Tr_Lec		0x08 //xxxx10xxb

#define Canal1		0x01 //xxxxxx01b
#define Canal2		0x02 //xxxxxx10b
#define Canal3		0x03 //xxxxxx11b

//Estado DMA
#define PeticionC0	0x10
#define PeticionC1	0x20
#define PeticionC2	0x40
#define PeticionC3	0x80

#define TCC0				0x01
#define TCC1				0x02
#define TCC2				0x04
#define TCC3				0x08



//Mascara DMA
#define InhibirC0	0x04
#define InhibirC1	0x05
#define InhibirC2	0x06
#define InhibirC3	0x07

#define HabilitarC0 0x00
#define HabilitarC1 0x01
#define HabilitarC2 0x02
#define HabilitarC3 0x03


#define PByte(Direccion)	((u8)(Direccion))
#define SByte(Direccion)	((u8)(Direccion >> 8 ))
#define TByte(Direccion)	((u8)(Direccion >> 16))
#define CByte(Direccion)	((u8)(Direccion >> 24))



void inicDMA();
void setCanalDMA(u8 Canal, u32 Direccion, u8 Modo, u16 Cuenta);
void setMascaraDMA(u8 Mascara);
void setDirDMA(u8 Canal, u32 Direccion);
void setContDMA(u8 Canal, u16 Cuenta);
void setModoDMA(u8 Modo);
void habilitarCanal(u8 Canal);
void inhibirCanal(u8 Canal);
u8 peticionDMA(u8 Canal);
u8 TCDMA(u8 Canal);

#endif
