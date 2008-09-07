#include <kernel/drivers/floppy.h>

#include <video.h>
#include <kernel/definiciones.h>
#include <kernel/mem/memoria_s.h>
#include <kernel/mem/memoria_k.h>

#include <kernel/system.h>
#include <kernel/puertos.h>

u8 SelCabeza[2] = {0x00, 0x04};

Estado estado;

u8	STP; //Registro Principal de Estado
ST0_t	ST0; //Registro de Estado de Interrupciones
ST1_t	ST1; //Registro Fase Resultados
ST2_t	ST2; //Registro Fase Resultados
u8	ST3; //Registro de Unidad

u8 Int; 


/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
u8 leerSector(u32 Buffer, u16 NBloque)
{
	CHS chs;

	while(!esperandoByteProc());
	
	prenderMotor();
	
	vFnImprimir("Motor Prendido\n");
		
	setCanalDMA(2, Buffer, MDMADef | Tr_Esc | Canal2, 0x1FF);	
	habilitarCanal(2);
	
	chs = LBAaCHS(NBloque);
	
	Int = 0;
		
	while(!envComDesplCabeza(chs.Cilindro, chs.Cabeza));

	vFnImprimir("Comando Desplazar Cabeza Enviado\n");
	
	
	while (!Int);	//Esta espera activa debera reemplazarse por un bloqueo del proceso.

	Int = 0;
	
	while(!envComLectura(chs.Cilindro, chs.Cabeza, chs.Sector));
	
	vFnImprimir("Comando Lectura Enviado\n");
	
	while(!Int); //Esta espera activa debera reemplazarse por un bloqueo del proceso.
	
	while(!TCDMA(2)); 
	vFnImprimir("Se Alcanzo el Fin de Cuenta\n");
	
	apagarMotor();
	
	if(operExitosa()) return 1;

	while(!esperandoByteProc());		
	leerEstadoFDC();
	vFnImprimirEstadoFDC();	
	
	while(!ST0.Valido);
	vFnImprimirST0();
		
	while(!ST1.Valido);
	vFnImprimirST1();

	while(!ST2.Valido);
	vFnImprimirST2();	
	
	while(!leerST3());
	vFnImprimirST3();
	
	return 0;
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
u8 escribirSector(u32 Buffer, u16 NBloque)
{
	CHS chs;
	
	while(!esperandoByteProc());
	
	prenderMotor();
		
	//while(1);
	
	setCanalDMA(2, Buffer, MDMADef | Tr_Lec | Canal2, 0x1FF);	
	habilitarCanal(2);
	
	vFnImprimir("Canal DMA Seteado\n");
	
	chs = LBAaCHS(NBloque);
	
	Int = 0;
		
	while(!envComDesplCabeza(chs.Cilindro, chs.Cabeza))
		{
		vFnImprimir("Enviando comando desplazar cabeza...\n");
		}

	vFnImprimir("Comando Desplazar Cabeza Enviado\n");
	
	while (!Int);	

	vFnImprimir("ISR Disquetera ya respondio a interrupcion\n");
	
	Int = 0;
	
	while(!envComEscritura(chs.Cilindro, chs.Cabeza, chs.Sector));
	
	vFnImprimir("Comando escritura Enviado\n");

	while (!Int);	
	
	while(!TCDMA(2)); 
	vFnImprimir("Se Alcanzo el Fin de Cuenta\n");
	
	apagarMotor();
	
	if(operExitosa()) return 1;
		
	while(!esperandoByteProc());		
	leerEstadoFDC();
	vFnImprimirEstadoFDC();	
	
	while(!ST0.Valido);
	vFnImprimirST0();
		
	while(!ST1.Valido);
	vFnImprimirST1();

	while(!ST2.Valido);
	vFnImprimirST2();	
	
	while(!leerST3());
	vFnImprimirST3();
	
	return 0;	
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
CHS LBAaCHS(u16 NBloque)
{
	CHS chs;
	
	chs.Cilindro 	= NBloque / Sectores_Cilindro;
	chs.Cabeza 		= (NBloque % Sectores_Cilindro) / Sectores_Pista;
	chs.Sector 		= ((NBloque % Sectores_Cilindro) % Sectores_Pista) + 1;
	
	return chs;
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void prenderMotor()
{	
	vFnImprimir("Se va a Prender el Motor\n");
	//for(i = 0; i < 10000; i++);

//***************	Aca Pincha!!!!!!!!!!!! *******************/
	outport(0x3F2/*Reg_Sal_Dig*/, 0x1C); //00011100b Prender el Motor 
	vFnImprimir("Motor Prendido\n");
	//while(1);
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void apagarMotor()
{
	outport(Reg_Sal_Dig, 0x0C); //00001100b Apagar el Motor
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
u8 envComRecalibrado()
{
	if(!esperandoByteProc()) return 0;

	ST0.Valido = 0;
	outport(Reg_Datos_FDC, 0x07); //Recalibrar
	while(!esperandoByteProc());
	outport(Reg_Datos_FDC, 0x00); //Recalibrar: Cabeza 0 | Unidad A
	
	return 1;
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
u8 envComDesplCabeza(u8 Cilindro, u8 Cabeza)
{
	if(!esperandoByteProc()) return 0;

	ST0.Valido = 0;
	outport(Reg_Datos_FDC, 0x0F);
	while(!esperandoByteProc());
	outport(Reg_Datos_FDC, SelCabeza[Cabeza]);
	while(!esperandoByteProc());
	outport(Reg_Datos_FDC, Cilindro);
	
	return 1;
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
u8 envComLectura(u8 Cilindro, u8 Cabeza, u8 Sector)
{
	if(!esperandoByteProc()) return 0;

	invalidarEstados();
	outport(Reg_Datos_FDC, 0xE6); //11100110b MultiTrack | MFM 	
	enviarDatosOper(Cilindro, Cabeza, Sector);
	
	return 1;
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
u8 envComEscritura(u8 Cilindro, u8 Cabeza, u8 Sector)
{
	if(!esperandoByteProc()) return 0;
	
	invalidarEstados();
	outport(Reg_Datos_FDC, 0xC5); //11000101b MultiTrack | MFM 	
	enviarDatosOper(Cilindro, Cabeza, Sector);
	
	return 1;
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void enviarDatosOper(u8 Cilindro, u8 Cabeza, u8 Sector)
{
	while(!esperandoByteProc());
	outport(Reg_Datos_FDC, SelCabeza[Cabeza]); //Unidad A | Cabeza 0 (Revisar si Cabeza es la cant. de cab.)
	while(!esperandoByteProc());
	outport(Reg_Datos_FDC, Cilindro); //Nro Cil.
	while(!esperandoByteProc());
	outport(Reg_Datos_FDC, Cabeza); //Nro Cabeza
	while(!esperandoByteProc());
	outport(Reg_Datos_FDC, Sector); //Nro Sector
	while(!esperandoByteProc());
	outport(Reg_Datos_FDC, 0x02); //Tamanio Sector (log2 nro bytes) - 7 -> (log2 512) - 7 = 2
	while(!esperandoByteProc());
	outport(Reg_Datos_FDC, 0x12); //Ultimo Sector Pista
	while(!esperandoByteProc());
	outport(Reg_Datos_FDC, 0x50); //Tam. GAP 3
	while(!esperandoByteProc());
	outport(Reg_Datos_FDC, 0x00); //Tam. Datos
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void invalidarEstados()
{
	ST0.Valido = 0;
	ST1.Valido = 0;
	ST2.Valido = 0;
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
u8 listoPES()
{
	u8 valor;
	
	valor = inb(Reg_Estado_FDC);
	
	if((valor & 0x80) != 0) //Bit 7 a 1
		return 1;
	else
	{	
		vFnImprimirST0();
		vFnImprimirST1();
		vFnImprimirST2();
		vFnImprimirST3();
		
		return 0;
	}	
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
u8 leerEstadoFDC()
{
	return (STP = inb(Reg_Estado_FDC));
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnImprimirEstadoFDC()	
{
	if((STP & 0x80) != 0) //Bit 7 a 1
	{	
		vFnImprimir("FDC listo para E/S\n");
		
		if((STP & 0x40) != 0) //Bit 6 a 1
			vFnImprimir("Byte para la CPU\n");
		else
			vFnImprimir("Esperando Byte de la CPU\n");	
	}
	else
		vFnImprimir("FDC No esta Listo para E/S\n");
	
	if((STP & 0x10) != 0) //Bit 4 a 1	
		vFnImprimir("Procesando Comando\n");
	else
		vFnImprimir("FDC Libre\n");
		
	if((STP & 0x01) != 0) //Bit 0 a 1
		vFnImprimir("Diskettera Ocupada\n");
	else
		vFnImprimir("Diskettera Lista\n");	
}


void leerST0()
{
	outport(Reg_Datos_FDC, 0x08); //Leer Estado Int
	while(!bytePProcesador());
	ST0.RST0 = inb(Reg_Datos_FDC);
	ST0.Valido = 1;
	while(!bytePProcesador());
	inb(Reg_Datos_FDC); //Leer la posicion del Cabezal (Obligatorio)
}	
	
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
u8 vFnImprimirST0()
{	
	if(!ST0.Valido) return 0;

	if((ST0.RST0 >> 6) == 0)
		vFnImprimir("Terminacion Normal\n");
		
	if((ST0.RST0 >> 6) == 1)
		vFnImprimir("Comando No Finalizado\n");
	
	if((ST0.RST0 >> 6) == 2)
		vFnImprimir("Comando Invalido\n");
		
	if((ST0.RST0 >> 6) == 3)
		vFnImprimir("Terminacion Anormal\n");
	
	
	if((ST0.RST0 & 0x20) != 0) //Bit 5 a 1
		vFnImprimir("Fin de Seek\n");
//	else
//		vFnImprimir("Cilindro 0 No Alcanzado\n");
					
	if((ST0.RST0 & 0x10) != 0) //Bit 4 a 1
		vFnImprimir("Error en el Equipo(Puede faltar otro recalibrado)\n");
		
	if((ST0.RST0 & 0x08) != 0) //Bit 3 a 1
		vFnImprimir("La Disketera No esta Lista\n");	
	
	if((ST0.RST0 & 0x04) != 0) //Bit 2 a 1
		vFnImprimir("Direccion Cabeza 1\n");	
	else
		vFnImprimir("Direccion Cabeza 0\n");	
				
	return 1;		
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
u8 operExitosa() //Termino bien y no hay error de CRC.
{	
	while(!ST0.Valido || !ST1.Valido || !ST2.Valido);

	if((ST0.RST0 >> 6) == 0 && (ST1.RST1 & 0x20) == 0) //Terminacion Normal y no hay error de CRC
		return 1;
		
	return 0;	
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnImprimirST1()
{
	if((ST1.RST1 & 0x80) != 0) //Bit 7 a 1
		vFnImprimir("Fin del Cilindro\n");
		
	if((ST1.RST1 & 0x20) != 0) //Bit 5 a 1	
		vFnImprimir("Error de CRC\n");
		
	if((ST1.RST1 & 0x10) != 0) //Bit 4 a 1
		vFnImprimir("Tiempo Maximo de Transferencia Excedido\n");
		
	if((ST1.RST1 & 0x04) != 0) //Bit 2 a 1
		vFnImprimir("No Hay Datos\n");
		
	if((ST1.RST1 & 0x02) != 0) //Bit 1 a 1
		vFnImprimir("Protegido Contra Escritura\n");
		
	if((ST1.RST1 & 0x01) != 0) //Bit 0 a 1
		vFnImprimir("Marca de Direccion Perdida\n");
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnImprimirST2()
{
	if((ST2.RST2 & 0x20) != 0) //Bit 5 a 1
		vFnImprimir("Error de CRC en Campo de Datos\n");
		
	if((ST2.RST2 & 0x10) != 0) //Bit 4 a 1
		vFnImprimir("Cilindro Erroneo\n");
		
	if((ST2.RST2 & 0x08) != 0) //Bit 3 a 1
		vFnImprimir("Scan Hit Igual\n");
		
	if((ST2.RST2 & 0x04) != 0) //Bit 2 a 1
		vFnImprimir("Scan No Hit\n");
		
	if((ST2.RST2 & 0x02) != 0) //Bit 1 a 1
		vFnImprimir("Cilindro Defectuoso\n");
		
	if((ST2.RST2 & 0x01) != 0) //Bit 0 a 1
		vFnImprimir("Marca de Direccion Perdida\n");
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
u8 leerST3()
{
	if(!esperandoByteProc()) return 0;

	outport(Reg_Datos_FDC, 0x04); //00000100b  (ID Comando)
	while(!esperandoByteProc());
	outport(Reg_Datos_FDC, 0x00); //Unidad A | Cabeza 0 (Revisar si Cabeza es la cant. de cab.)
	while(!bytePProcesador());
	ST3 = inb(Reg_Datos_FDC);
	
	return 1;
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void vFnImprimirST3()
{
	if((ST3 & 0x80) != 0) //Bit 7 a 1
		vFnImprimir("Falla en la Diskettera\n");
		
	if((ST3 & 0x40) != 0) //Bit 6 a 1
		vFnImprimir("Proteccion Contra Escritura\n");
	
	if((ST3 & 0x20) != 0) //Bit 5 a 1
		vFnImprimir("Diskettera Lista\n");
	else
		vFnImprimir("La Diskettera No esta Lista\n");
		
	if((ST3 & 0x10) != 0) //Bit 4 a 1
		vFnImprimir("Posicionado en Cilindro 0\n");
	
	if((ST3 & 0x04) != 0) //Bit 2 a 1
		vFnImprimir("Posicion de la Cabeza en 1\n");
	else
		vFnImprimir("Posicion de la Cabeza en 0\n");	
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
u8 esperandoByteProc()
{
	if((leerEstadoFDC() & 0xC0) == 0x80) // Bit 7 en 1 y 6 en 0
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
u8 bytePProcesador()
{
	if((leerEstadoFDC() & 0xC0) == 0xC0) // Bit 7 en 1 y 6 en 1
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
u8 FDCOcupado() // Desde que inicia el comando hasta que termina el resultado
{
	if((leerEstadoFDC() & 0x10) != 0) //Bit 4 a 1
	{	
		vFnImprimir("FDC Ocupado\n");
		return 1;
	}	
	else 
	{
		vFnImprimir("FDC Listo p/ Comando\n");
		return 0;
	}		
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
u8 unidadOcupada() // Seek o Recalibrado
{
	if((leerEstadoFDC() & 0x01) != 0) // Bit 1 a 1
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
void obtResOper()
{
	u8 i;
	
	while(!bytePProcesador());
	ST0.RST0 		= inb(Reg_Datos_FDC);
	ST0.Valido 	= 1;
	while(!bytePProcesador());
	ST1.RST1 		= inb(Reg_Datos_FDC);
	ST1.Valido 	= 1;
	while(!bytePProcesador());
	ST2.RST2 		= inb(Reg_Datos_FDC);
	ST2.Valido 	= 1;
	
	for(i = 0; i < 4; i++)
	{
		while(!bytePProcesador());
		inb(Reg_Datos_FDC);
	}	
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
//Handler Interrupcion
void ISRDisketeraC()
{
	vFnImprimir("Interrupcion de la Disketera\n");
	
	if(!bytePProcesador()) //Comando Seek / Recalibrado
		leerST0();
	else
		obtResOper();
		
	Int = 1;	
}
