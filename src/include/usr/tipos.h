/**
 * \file usr/tipos.h
 * \brief Definiciones de tipos.
 *
 * Esta biblioteca contiene las definiciones de tipo de datos necesarias
 * para trabajar con bytes, words y double words (referenciar los mismos
 * datos de assembler desde C)
 *
 * Estas definiciones se utilizan tanto en kernel como en lado usuario
 */

#ifndef _TIPOS_H_
#define _TIPOS_H_

typedef unsigned int dword;
typedef unsigned short word;
typedef unsigned char byte;

typedef unsigned char 	u8;
typedef signed	 char 	s8;
typedef unsigned short	u16;
typedef signed	 short	s16; 
typedef unsigned int		u32; 
typedef signed	 int		s32; 

//Registros de 80 bits, utilizados por la FPU
typedef struct _u80 { unsigned int word[20]; }	u80; 

#define PByte(Direccion)	((u8)(Direccion))
#define SByte(Direccion)	((u8)(Direccion >> 8 ))
#define TByte(Direccion)	((u8)(Direccion >> 16))
#define CByte(Direccion)	((u8)(Direccion >> 24))

#endif
