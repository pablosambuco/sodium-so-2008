#include <kernel/drivers/dma.h>
#include <video.h>
#include <kernel/definiciones.h>
#include <kernel/system.h>
#include <kernel/puertos.h>


u8 Reg_Dir	[4] = {0x00, 0x02, 0x04, 0x06};
u8 Reg_Cont	[4] = {0x01, 0x03, 0x05, 0x07};
u8 Reg_Pag	[4] = {0x87, 0x83, 0x81, 0x82};

u8 InhCanal[4] = {0x04, 0x05, 0x06, 0x07};
u8 HabCanal[4] = {0x00, 0x01, 0x02, 0x03};

u8 Peticion	[4] = {0x10, 0x20, 0x40, 0x80};
u8 TC				[4] = {0x01, 0x02, 0x04, 0x08};


/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void inicDMA()
{
	setMascaraDMA(0xFF);
	outport(Reg_Comando_Esc, 0);
	outport(Reset_FF, 0);
	vFnImprimir("DMA Iniciado\n");
//	while(1);
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void setCanalDMA(u8 Canal, u32 Direccion, u8 Modo, u16 Cuenta)
{
	inhibirCanal	(Canal);
	setModoDMA		(Modo);
	setDirDMA			(Canal, Direccion);
	setContDMA		(Canal, Cuenta);
}	

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void setMascaraDMA(u8 Mascara)
{
	outport(RW_Reg_Mascara, Mascara);
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void setDirDMA(u8 Canal, u32 Direccion)
{
	outport(Reg_Dir[Canal], PByte(Direccion));
	outport(Reg_Dir[Canal], SByte(Direccion));
	outport(Reg_Pag[Canal], TByte(Direccion));
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void setContDMA(u8 Canal, u16 Cuenta)
{
	outport(Reg_Cont[Canal], PByte(Cuenta));
	outport(Reg_Cont[Canal], SByte(Cuenta));
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void setModoDMA(u8 Modo)
{
	outport(Reg_Modo, Modo);
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void habilitarCanal(u8 Canal)
{
	outport(Mascara_Canal, HabCanal[Canal]);
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void inhibirCanal(u8 Canal)
{
	outport(Mascara_Canal, InhCanal[Canal]);
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
u8 peticionDMA(u8 Canal)
{
	u8 estado;
	
	estado = inb(Reg_Estado_DMA);
	
	if((estado & Peticion[Canal]) == Peticion[Canal])	
		return 1;
	else
		return 0; 
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
u8 TCDMA(u8 Canal)
{
	u8 estado;
	
	estado = inb(Reg_Estado_DMA);
	
	if((estado & TC[Canal]) == TC[Canal])	
		return 1;
	else
		return 0; 
} 
