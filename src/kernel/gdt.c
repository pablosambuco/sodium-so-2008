#include <kernel/gdt.h>
#include <kernel/pcb.h>
#include <video.h>
#include <kernel/registros.h>
#include <kernel/init.h>
#include <kernel/idle.h>
#include <kernel/sched.h>
#include <kernel/syscall.h>
#include <kernel/libk/string.h>
#include <kernel/shm.h>

extern dword pdwGDT;

unsigned int uiUltimoPid = 0;
unsigned long ulProcActual = 0;
unsigned long ulUltimoProcesoEnFPU = 0; //PID del ultimo proceso que uso el FPU

//stuPCB pstuPCB[CANTMAXPROCS];
stuTSS stuTSSTablaTareas[CANTMAXPROCS];

#define TOTAL_ENTRADAS_GDT (sizeof(stuEstructuraGdt) / sizeof(stuGDTDescriptor))

unsigned char iMapaGDT[ TOTAL_ENTRADAS_GDT / 8 ]; 

#define SEGMENT_SIZE	0x20000 // 128 Kb
#define TOTAL_SEGMENTOS	(16 * 0x100000 / SEGMENT_SIZE) // mapeo 16 Mbs

unsigned char iMapaSegmentos[ TOTAL_SEGMENTOS / 8 ];		

#define _SET_BIT( bitmap, pos, set ) 					\
	do{								\
		if( set ){						\
			bitmap[ (pos-(pos%8)) / 8 ] |= (1 << (7-((pos)%8))); \
		} else	{						\
			bitmap[ (pos-(pos%8)) / 8 ] &= ~(1 << (7-((pos)%8))); \
		}							\
	}while( 0 )

#define _GET_BIT( bitmap, pos ) 					\
			(bitmap[ (pos) / 8 ] & (1 << (7-((pos)%8)))) 	\

#define mapa_gdt_set( pos, set ) _SET_BIT( iMapaGDT, pos, set )
#define mapa_gdt_get( pos ) _GET_BIT( iMapaGDT, pos )

#define mapa_segmentos_set( pos, set ) _SET_BIT( iMapaSegmentos, pos, set )
#define mapa_segmentos_get( pos ) _GET_BIT( iMapaSegmentos, pos )

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
void
vFnGdtInicializar (dword pdwGDT)
{
  int iN;
  pstuTablaGdt = (stuEstructuraGdt *) pdwGDT;

  for (iN = 0; iN < CANTMAXPROCS; iN++)
    {
      pstuPCB[iN].iEstado = PROC_NO_DEFINIDO;
    }

  for( iN = 0; iN < sizeof( iMapaGDT ) / sizeof( iMapaGDT[0]); iN++ )
	  iMapaGDT[iN] = 0; // bloque de entradas de la GDT no utilizadas

  mapa_gdt_set( 0, 1 ); /* descriptor nulo */
  mapa_gdt_set( 1, 1 ); /* descriptor codigo del kernel */
  mapa_gdt_set( 2, 1 ); /* descriptor datos+stack del kernel */

  for( iN = 0; iN < sizeof( iMapaSegmentos ) / sizeof(iMapaSegmentos[0]); iN++ )
	  iMapaSegmentos[iN] = 0; // bloque de segmentos no utilizados

  for( iN = 0; iN < 32; iN++) // 16; iN++ ) 
	  mapa_segmentos_set( iN, 1 );	// los primeros 32 segmentos (4Mb) los dejamos para el kernel
}

/******************************************************************************
Función: iFnBuscaPosicionProc
Descripción:  Devuelve la posición en el vector de procesos al pid ingresado
              como parámetro.

Recibe:   *Integer con el nro de proceso
Devuelve: *Integer con la posición en el vector de procesos o -1 si no existe o 
	fue eliminado.

Fecha última modificación: 11/11/2007
*******************************************************************************/
int
iFnBuscaPosicionProc (unsigned long ulPid)
{
  int iN = 0, iPosicion = -1;
  while (iN < CANTMAXPROCS && iPosicion == -1)
    {
      if ((pstuPCB[iN].ulId == ulPid) && (pstuPCB[iN].iEstado != PROC_NO_DEFINIDO) && (pstuPCB[iN].iEstado != PROC_ELIMINADO))
	iPosicion = iN;
      iN++;
    }
  return iPosicion;
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
unsigned int
uiFnSetearBaseLimiteDescriptor( int uiPosicion,
				unsigned int uiBase, 
				unsigned int uiLimite )
{
	pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].usBaseBajo = uiBase & 0xFFFF;
	pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].ucBaseMedio = (uiBase >> 16) & 0xFF;
	pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].usBaseAlto = uiBase >> 24;
	pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].usLimiteBajo = uiLimite & 0xFFFF;
	pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].bitLimiteAlto = (uiLimite >> 16) & 0x0F;
	
	return uiPosicion;
}

/***************************************************************************
 Funcion: uiFnAgregarDescriptorGDT
 Descripcion: Funcion que agrega un descriptor en la GDT
 Parametros: unsigned int base  (dirección inicial del segmento en memoria),
          unsigned int limit (longitud del segmento (con o sin granularidad)
          unsigned int opt   (tipo de descriptor, acceso, crecimiento,
                          granularidad, etc)
 Valor devuelto: Offset desde la base de la GDT
 Ultima modificacion: 09/04/2006
***************************************************************************/

unsigned int
uiFnAgregarDescriptorGDT (unsigned int uiBase, unsigned int uiLimite,
			  unsigned int uiOpt, int uiPosicion)
{
  uiFnSetearBaseLimiteDescriptor( uiPosicion, uiBase, uiLimite );

  pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].ucAcesso = (uiOpt + D_PRESENT) >> 8;
  pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].bitGranularidad = ((uiOpt & 0xff) >> 4);

  return (uiPosicion);
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
unsigned int
uiFnAgregarDescriptorGDT2(int uiPosicion,
			  unsigned int uiBase, 
			  unsigned int uiLimite,
			  unsigned char type,
			  unsigned char bit_s,
			  unsigned char dpl,
			  unsigned char bit_p,
			  unsigned char avl,
			  unsigned char bit_d_b,
			  unsigned char bit_g )
{
  uiFnSetearBaseLimiteDescriptor( uiPosicion, uiBase, uiLimite );

  pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].ucAcesso = (type & 0x0f) | ((bit_s & 1) << 4) | ((dpl & 1) << 5) | ((bit_p & 1) << 7);
  pstuTablaGdt->stuGdtDescriptorDescs[uiPosicion].bitGranularidad = (avl & 1) | ((bit_d_b & 1) << 1) | ((bit_g & 1) << 1);

  return (uiPosicion);
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnBuscarPCBLibre()
{
  int iPosicion = 0;
 
  while (iPosicion < CANTMAXPROCS
	 && pstuPCB[iPosicion].iEstado != PROC_ELIMINADO
	 && pstuPCB[iPosicion].iEstado != PROC_NO_DEFINIDO )
    {
      iPosicion++;
    }
 
  if( iPosicion < CANTMAXPROCS ) 
	  return iPosicion;

  return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
unsigned int uiFnBuscarEntradaGDTLibre()
{
  unsigned int uiPosicion = 0;
 
  while (uiPosicion < TOTAL_ENTRADAS_GDT && mapa_gdt_get( uiPosicion ) )
    {
      uiPosicion++;
    }
  
  if (uiPosicion < TOTAL_ENTRADAS_GDT ){
	  mapa_gdt_set( uiPosicion, 1 ); // marcamos como usada
	  return uiPosicion;
  }

  return 0;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
unsigned int uiFnBuscarSegmentoLibre()
{
  unsigned int uiPosicion = 0;
 
  while (uiPosicion < TOTAL_SEGMENTOS && mapa_segmentos_get( uiPosicion ) )
    {
      uiPosicion++;
    }
  
  if (uiPosicion < TOTAL_SEGMENTOS ){
	  mapa_segmentos_set( uiPosicion, 1 ); // marcamos como usada
	  return uiPosicion;
  }

  return 0;
}

/***************************************************************************
 Funcion: iFnCrearTSS
 Descripcion: Funcion que crea una TSS para la nueva tarea y llama a
           addTssToGdt para insertar en la GDT un descriptor que la
           identifique
 Parametros: void *eip, puntero al código de la función que queremos ejecutar
 Valor devuelto: posicion en la tabla de procesos o -1 si no se pudo crear
 Ultima modificacion: 09/04/2006
***************************************************************************/

int
iFnCrearTSS (void *pEIP,
	     int iPosicion,
	     unsigned int uiCS,
	     unsigned int uiDS,
	     unsigned int uiSS)
{
	int iN = 0;

    stuTSSTablaTareas[iPosicion].trapbit = 0;
    stuTSSTablaTareas[iPosicion].uIOMapeoBase = sizeof (stuTSS);
    stuTSSTablaTareas[iPosicion].cs = uiCS;
    stuTSSTablaTareas[iPosicion].fs = uiDS;
    stuTSSTablaTareas[iPosicion].gs = uiDS;
    stuTSSTablaTareas[iPosicion].ds = uiDS;
    stuTSSTablaTareas[iPosicion].es = uiDS;
    stuTSSTablaTareas[iPosicion].ss = uiSS;
    stuTSSTablaTareas[iPosicion].ss0 = uiSS;
    stuTSSTablaTareas[iPosicion].ss1 = uiSS;
    stuTSSTablaTareas[iPosicion].ss2 = uiSS;
    
    stuTSSTablaTareas[iPosicion].esp0 =
      (unsigned int) &stuTSSTablaTareas[iPosicion].
      espacio0[TSS_TAMANIO_STACK_R0 - 8];
    
    stuTSSTablaTareas[iPosicion].esp1 =
      (unsigned int) &stuTSSTablaTareas[iPosicion].
      espacio1[TSS_TAMANIO_STACK_R1 - 8];
    
    stuTSSTablaTareas[iPosicion].esp2 =
      (unsigned int) &stuTSSTablaTareas[iPosicion].
      espacio2[TSS_TAMANIO_STACK_R2 - 8];
    
    stuTSSTablaTareas[iPosicion].esp =
      (unsigned int) &stuTSSTablaTareas[iPosicion].
      espacio0[TSS_TAMANIO_STACK_R0 - 8];
    
    stuTSSTablaTareas[iPosicion].ebp =
      (unsigned int) &stuTSSTablaTareas[iPosicion].
      espacio0[TSS_TAMANIO_STACK_R0 - 8];

    //Rellenamos los stacks de los distintos rings de ejecucion con patrones faciles de distinguir. Facilita mucho
    //el debugueo del contenido del stack en tiempo de ejecucion con los comandos "stack" y "dump"
    for (iN = 0; iN < TSS_TAMANIO_STACK_R0; iN++)
      {
        stuTSSTablaTareas[iPosicion].espacio0[iN] = 0x11L;
      }
    for (iN = 0; iN < TSS_TAMANIO_STACK_R1; iN++)
      {
        stuTSSTablaTareas[iPosicion].espacio1[iN] = 0x22L;
      }
    for (iN = 0; iN < TSS_TAMANIO_STACK_R2; iN++)
      {
        stuTSSTablaTareas[iPosicion].espacio2[iN] = 0x33L;
      }

    stuTSSTablaTareas[iPosicion].uiEBandera = 0x202L;	//0x3202L; para tareas ring3

    stuTSSTablaTareas[iPosicion].eip = (unsigned int) pEIP;
    stuTSSTablaTareas[iPosicion].ldt = 0;
    stuTSSTablaTareas[iPosicion].eax = 0;
    stuTSSTablaTareas[iPosicion].ebx = 0;
    stuTSSTablaTareas[iPosicion].ecx = 0;
    stuTSSTablaTareas[iPosicion].edx = 0;

	/* Valores para FPU (17-07-08) */
	//Redondeo al mas cercano, Doble precision, Interrupcion por Precision enmascarada (bit PM),
    //el resto de las interrupciones sin enmascarar y Stack vacio. 
	stuTSSTablaTareas[iPosicion].fpu.control = 0x0360; 
	stuTSSTablaTareas[iPosicion].fpu.status = 0x0000;
	stuTSSTablaTareas[iPosicion].fpu.tag = 0xFFFF;
	/*	stuTSSTablaTareas[iPosicion].fpu.ip = 0;
	stuTSSTablaTareas[iPosicion].fpu.cs = 0;
	stuTSSTablaTareas[iPosicion].fpu.dp = 0;
	stuTSSTablaTareas[iPosicion].fpu.ds = 0;*/
	/* FIN Valores para FPU */

	return (iPosicion);
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int
iFnCrearTSS2 (void *pEIP,
	      void *pESP,
	     int iPosicion,
	     unsigned int uiCS,
	     unsigned int uiDS,
	     unsigned int uiSS)
{
	stuTSSTablaTareas[iPosicion].trapbit = 0;
	stuTSSTablaTareas[iPosicion].uIOMapeoBase = sizeof (stuTSS);
	stuTSSTablaTareas[iPosicion].cs = uiCS;
	stuTSSTablaTareas[iPosicion].fs = uiDS;
	stuTSSTablaTareas[iPosicion].gs = uiDS;
	stuTSSTablaTareas[iPosicion].ds = uiDS;
	stuTSSTablaTareas[iPosicion].es = uiDS;
	stuTSSTablaTareas[iPosicion].ss = uiSS;
	stuTSSTablaTareas[iPosicion].ss0 = 0x00; // estos selectores nunca se usan, porque
	stuTSSTablaTareas[iPosicion].ss1 = 0x00; // al estar ya en ring0, una interrupción no
	stuTSSTablaTareas[iPosicion].ss2 = 0x00; // nos cambia de nivel de proteccion
	
	stuTSSTablaTareas[iPosicion].esp0 = (unsigned int) 0;
	stuTSSTablaTareas[iPosicion].esp1 = (unsigned int) 0;
	stuTSSTablaTareas[iPosicion].esp2 = (unsigned int) 0;
	
	stuTSSTablaTareas[iPosicion].esp = (unsigned int) pESP;
	stuTSSTablaTareas[iPosicion].ebp = (unsigned int) pESP;
	
	stuTSSTablaTareas[iPosicion].uiEBandera = 0x202L;	//0x3202L; para tareas ring3
	
	stuTSSTablaTareas[iPosicion].eip = (unsigned int) pEIP;
	stuTSSTablaTareas[iPosicion].ldt = 0;
	stuTSSTablaTareas[iPosicion].eax = 0;
	stuTSSTablaTareas[iPosicion].ebx = 0;
	stuTSSTablaTareas[iPosicion].ecx = 0;
	stuTSSTablaTareas[iPosicion].edx = 0;

	/* Valores para FPU (17-07-08) */
	//Redondeo al mas cercano, Doble precision, Interrupcion por Precision enmascarada (bit PM),
    //el resto de las interrupciones sin enmascarar y Stack vacio. 
	stuTSSTablaTareas[iPosicion].fpu.control = 0x0360; 
	stuTSSTablaTareas[iPosicion].fpu.status = 0x0000;
	stuTSSTablaTareas[iPosicion].fpu.tag = 0xFFFF;
	/*	stuTSSTablaTareas[iPosicion].fpu.ip = 0;
	stuTSSTablaTareas[iPosicion].fpu.cs = 0;
	stuTSSTablaTareas[iPosicion].fpu.dp = 0;
	stuTSSTablaTareas[iPosicion].fpu.ds = 0;*/
	/* FIN Valores para FPU */

	return (iPosicion);
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnCrearPCB( int iPosicion, 
		 void *pEIP, 
		 char *stNombre, 
		 unsigned int uiIndiceGDT_CS,
		 unsigned int uiIndiceGDT_DS,
		 unsigned int uiIndiceGDT_TSS,
	      	 unsigned int uiPosTSS,
	      	 unsigned int uiDirBase,
	         unsigned int uiLimite )
{
	int iN;
	int iJ;
	tms tmsDefault;
	itimerval itimervalDefault;
	
	tmsDefault.tms_utime = 0;
	tmsDefault.tms_stime = 0;
	tmsDefault.tms_cutime = 0;
	tmsDefault.tms_cstime = 0;
	itimervalDefault.it_interval.tv_sec = 0;
	itimervalDefault.it_interval.tv_usec = 0;
	itimervalDefault.it_value.tv_sec = 0;
	itimervalDefault.it_value.tv_usec = 0;

	pstuPCB[iPosicion].uiIndiceGDT_CS = uiIndiceGDT_CS;
	pstuPCB[iPosicion].uiIndiceGDT_DS = uiIndiceGDT_DS;
	pstuPCB[iPosicion].uiIndiceGDT_TSS = uiIndiceGDT_TSS;

	pstuPCB[iPosicion].vFnFuncion 	= pEIP;
	pstuPCB[iPosicion].iEstado 	= PROC_LISTO;	//Lista para ejecucion
	pstuPCB[iPosicion].ulLugarTSS 	= uiPosTSS;
	pstuPCB[iPosicion].ulId 	= uiUltimoPid++;
	pstuPCB[iPosicion].ulParentId 	= pstuPCB[ulProcActual].ulId;
	pstuPCB[iPosicion].uiDirBase 	= uiDirBase;
	pstuPCB[iPosicion].uiLimite 	= uiLimite;
	pstuPCB[iPosicion].stuTmsTiemposProceso	= tmsDefault;
	pstuPCB[iPosicion].timers[0]=itimervalDefault;
	pstuPCB[iPosicion].timers[1]=itimervalDefault;
	pstuPCB[iPosicion].timers[2]=itimervalDefault;
	pstuPCB[iPosicion].lNanosleep 	= 0;
	pstuPCB[iPosicion].puRestoDelNanosleep 	= NULL;
	pstuPCB[iPosicion].uiTamProc = 0;
	pstuPCB[iPosicion].lPidTracer 	= PROC_WOTRACER;

	for (iN = 0; iN < 12; iN++)
	  {
	    pstuPCB[iPosicion].stNombre[iN] = stNombre[iN];
	  }

	pstuPCB[iPosicion].stNombre[iN] = '\0';
	
	/*agregado por el grupo*/
	pstuPCB[iPosicion].iPrioridad = 0;
	
	pstuPCB[iPosicion].uiLimite 	= uiLimite;

	//inicializo el vector de memorias comartidas
	for (iJ = 0; iJ < MAXSHMEMPORPROCESO; iJ ++)
	{
		pstuPCB[iPosicion].memoriasAtachadas[iJ].utilizada = FALSE;
	}
	
	return iPosicion;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnNuevaTarea( void *pEIP, char *stNombre )
{
	unsigned short int bInterrupcionesHabilitadas = 0;
	unsigned long int *puliParametroStack;
	unsigned int uiIndiceGDT;
	int iPosicion = iFnBuscarPCBLibre(),
	    iPosTSS = 0;
	int iFlags;

	__asm__ ("pushf\n pop %%eax": "=a" (iFlags):);
	// si estaban habilitadas, aqui se deshabilitan
	if (iFlags & 0x200){
	    __asm__ ("cli"::);
	    bInterrupcionesHabilitadas = 1;
	}

	iPosTSS = iFnCrearTSS( pEIP, 
			       iPosicion,
			       wFnGetCS(),
			       wFnGetDS(),
			       wFnGetSS() );

	uiIndiceGDT = uiFnBuscarEntradaGDTLibre();

	uiFnAgregarDescriptorGDT ((unsigned int)
	  			  &stuTSSTablaTareas[iPosTSS], 103,
	  			  (D_TSS + D_BIG), uiIndiceGDT);
	
	iFnCrearPCB( iPosicion, /* PCB asignada */
		     pEIP, /* funcion a ejecutar */
		     stNombre, /* nombre del proceso */
		     1 /* posicion del CS del kernel */, 
		     2 /* posicion del DS del kernel */, 
		     uiIndiceGDT, /* indice de la TSS para este proceso en la GDT */
		     iPosicion, /* posicion de la TSS en la tabla de TSS (igual a la de la tabla de PCBs) */
		     0x000000, /* base del segmento (usa el del kernel por lo tanto, es cero) */
		     0xffffffff /* limite fisico (4gb) */ );

	//ESCRIBIMOS EN EL STACK SPACE (RING0) DEL PROCESO CREADO EL CONTENIDO DE LA VARIABLE PID QUE ÉSTE TOMA COMO PARÁMETRO
	
	puliParametroStack =
	  (unsigned long int *) &(stuTSSTablaTareas[iPosicion].
	  			espacio0[TSS_TAMANIO_STACK_R0 - 4]);
	*puliParametroStack = (unsigned long int) pstuPCB[iPosicion].ulId;

	if (bInterrupcionesHabilitadas)
	  __asm__ ("sti"::);

	return iPosicion;
}

/**
 * @brief Copia 'size' bytes de 'orig' a 'dest'
 *
 * XXX Optimizar reescribiendo en inline assembler
 */
unsigned char* ucpFnMemCopy( unsigned char *ucpDestino, unsigned char *ucpOrigen, unsigned int uiTamanio )
{
	unsigned char *ucpDestinoOriginal = ucpDestino;

	while( uiTamanio-- )
		*ucpDestino++ = *ucpOrigen++;

	return ucpDestinoOriginal;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnInstanciarInit()
{
	unsigned short int bInterrupcionesHabilitadas = 0;
	unsigned int uiIndiceGDT_CS,
		     uiIndiceGDT_DS,
		     uiIndiceGDT_TSS,
		     uiBaseSegmento;
	int iPosicion = iFnBuscarPCBLibre();
	int iFlags;

	__asm__ ("pushf\n pop %%eax": "=a" (iFlags):);
	// si estaban habilitadas, aqui se deshabilitan
	if (iFlags & 0x200){
	    __asm__ ("cli"::);
	    bInterrupcionesHabilitadas = 1;
	}

	uiBaseSegmento = uiFnBuscarSegmentoLibre() * SEGMENT_SIZE;
	uiIndiceGDT_CS = uiFnBuscarEntradaGDTLibre();
	ucpFnCopiarMemoria( (unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[uiIndiceGDT_CS], 
		(unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[wFnGetCS() / 8],
		sizeof( stuGDTDescriptor ) );
	// divido por 4k porque me estoy copiando el selector de datos del kernel, que tiene
	// granularidad en 4k
	uiFnSetearBaseLimiteDescriptor( uiIndiceGDT_CS, uiBaseSegmento, SEGMENT_SIZE / 4096 ); 

	uiIndiceGDT_DS = uiFnBuscarEntradaGDTLibre();
	ucpFnCopiarMemoria( (unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[uiIndiceGDT_DS], 
		(unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[wFnGetDS() / 8],
		sizeof( stuGDTDescriptor ) );
	uiFnSetearBaseLimiteDescriptor( uiIndiceGDT_DS, uiBaseSegmento, SEGMENT_SIZE / 4096 );

	vFnImprimir( "\n Copiando el codigo del proceso \"init\" de 0x%x a 0x%x, ocupa %d bytes", 
			__init_begin(), uiBaseSegmento, __init_size() );
	ucpFnCopiarMemoria( (unsigned char*)uiBaseSegmento, (unsigned char*)__init_begin(), SEGMENT_SIZE );

	if( iPosicion != iFnCrearTSS2( 0x0, /* arranca en la posicion 0 del nuevo binario */
				(void*)(SEGMENT_SIZE - 0x10),
			       iPosicion,
			      uiIndiceGDT_CS * 8,
			      uiIndiceGDT_DS * 8,
			      uiIndiceGDT_DS * 8 ) ){
		// EXPLOTA!! XXX
	}

	uiIndiceGDT_TSS = uiFnBuscarEntradaGDTLibre();

	uiFnAgregarDescriptorGDT ((unsigned int)
	  			  &stuTSSTablaTareas[iPosicion], 103,
	  			  (D_TSS + D_BIG), uiIndiceGDT_TSS);
	
	iFnCrearPCB( iPosicion, /* PCB de init */
		     0x0, /* direccion de arranque de init */
		     "PR_init()...", /* nombre */
		     uiIndiceGDT_CS,
		     uiIndiceGDT_DS, 
		     uiIndiceGDT_TSS, 
		     iPosicion, /* posicion de la TSS dentro de la tabla de TSSs */
		     uiBaseSegmento, /* direccion base del segmento asignado a este proc */
		     SEGMENT_SIZE /* limite del segmento */ );

	if (bInterrupcionesHabilitadas)
	  __asm__ ("sti"::);

	return iPosicion;
}
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnInstanciarIdle()
{
	unsigned short int bInterrupcionesHabilitadas = 0;
	unsigned int uiIndiceGDT_CS,
		     uiIndiceGDT_DS,
		     uiIndiceGDT_TSS,
		     uiBaseSegmento;
	int iFlags;
	iTareaNula = iFnBuscarPCBLibre(); //siempre se carga en el primero,
								//el idle es el primer proceso usuario insanciado
	//vFnImprimir("\npid: %d\n",iTareaNula);
	

	__asm__ ("pushf\n pop %%eax": "=a" (iFlags):);
	// si estaban habilitadas, aqui se deshabilitan
	if (iFlags & 0x200){
	    __asm__ ("cli"::);
	    bInterrupcionesHabilitadas = 1;
	}

	uiBaseSegmento = uiFnBuscarSegmentoLibre() * SEGMENT_SIZE;
	uiIndiceGDT_CS = uiFnBuscarEntradaGDTLibre();
	ucpFnCopiarMemoria( (unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[uiIndiceGDT_CS], 
		(unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[wFnGetCS() / 8],
		sizeof( stuGDTDescriptor ) );
	// divido por 4k porque me estoy copiando el selector de datos del kernel, que tiene
	// granularidad en 4k
	uiFnSetearBaseLimiteDescriptor( uiIndiceGDT_CS, uiBaseSegmento, SEGMENT_SIZE / 4096 ); 

	uiIndiceGDT_DS = uiFnBuscarEntradaGDTLibre();
	ucpFnCopiarMemoria( (unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[uiIndiceGDT_DS], 
		(unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[wFnGetDS() / 8],
		sizeof( stuGDTDescriptor ) );
	uiFnSetearBaseLimiteDescriptor( uiIndiceGDT_DS, uiBaseSegmento, SEGMENT_SIZE / 4096 );

	/*vFnImprimir( "\n Copiando el codigo del proceso \"idle\" de 0x%x a 0x%x, ocupa %d bytes", 
			__idle_begin(), uiBaseSegmento, __idle_size() );*/
	ucpFnCopiarMemoria( (unsigned char*)uiBaseSegmento, (unsigned char*)__idle_begin(), SEGMENT_SIZE );

	if( iTareaNula != iFnCrearTSS2( 0x0, /* arranca en la posicion 0 del nuevo binario */
				(void*)(SEGMENT_SIZE - 0x10),
			       iTareaNula,
			      uiIndiceGDT_CS * 8,
			      uiIndiceGDT_DS * 8,
			      uiIndiceGDT_DS * 8 ) ){
		// EXPLOTA!! XXX
	}

	uiIndiceGDT_TSS = uiFnBuscarEntradaGDTLibre();

	uiFnAgregarDescriptorGDT ((unsigned int)
	  			  &stuTSSTablaTareas[iTareaNula], 103,
	  			  (D_TSS + D_BIG), uiIndiceGDT_TSS);
	
	iFnCrearPCB( iTareaNula, /* PCB de idle */
		     0x0, /* direccion de arranque de idle */
		     "PR_idle()...", /* nombre */
		     uiIndiceGDT_CS,
		     uiIndiceGDT_DS, 
		     uiIndiceGDT_TSS, 
		     iTareaNula, /* posicion de la TSS dentro de la tabla de TSSs */
		     uiBaseSegmento, /* direccion base del segmento asignado a este proc */
		     SEGMENT_SIZE /* limite del segmento */ );

	if (bInterrupcionesHabilitadas)
	  __asm__ ("sti"::);

	return iTareaNula;
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnDuplicarProceso( unsigned int uiProcPadre ){
	unsigned int uiIndiceGDT_CS,
		     uiIndiceGDT_DS,
		     uiIndiceGDT_TSS,
		     uiStackUsuario,
		     uiComienzoStackKernel,
		     uiBaseSegmento;
	int iPosicion = 0;

	// no deshabilitamos las interrupciones porque esta funcion se
	// invoca desde el syscall fork(), que a su vez es llamado por
	// el handler de la int 0x80, por lo cual al momento de comenzar
	// la atencion de la interrupcion, se deshabilitan las demas

	iPosicion = iFnBuscarPCBLibre();
	uiBaseSegmento = uiFnBuscarSegmentoLibre() * SEGMENT_SIZE;

	if( !iPosicion || !uiBaseSegmento )
		return -EAGAIN;

	uiIndiceGDT_CS = uiFnBuscarEntradaGDTLibre();
	uiIndiceGDT_DS = uiFnBuscarEntradaGDTLibre();
	uiIndiceGDT_TSS = uiFnBuscarEntradaGDTLibre();

	if( !uiIndiceGDT_CS || !uiIndiceGDT_DS || !uiIndiceGDT_TSS ) 
	// XXX habria que desmarcar el segmento o las entradas en la GDT que SI se pudieron asignar
		return -ENOMEM;
#if 1	
	vFnLog( "\n fork(): iFnBuscarPCBLibre: %d", iPosicion );
	vFnLog( "\n fork(): uiFnBuscarSegmentoLibre: %d, 0x%x", uiBaseSegmento / SEGMENT_SIZE, uiBaseSegmento );
	vFnLog( "\n fork(): uiFnBuscarEntradaGDTLibre(CS): %d, 0x%x", uiIndiceGDT_CS, uiIndiceGDT_CS * 8 );
	vFnLog( "\n fork(): uiFnBuscarEntradaGDTLibre(DS): %d, 0x%x", uiIndiceGDT_DS, uiIndiceGDT_DS * 8 );
	vFnLog( "\n fork(): uiFnBuscarEntradaGDTLibre(TSS): %d, 0x%x", uiIndiceGDT_TSS, uiIndiceGDT_TSS * 8 );
#endif

	ucpFnCopiarMemoria( (unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[uiIndiceGDT_CS], 
		(unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[stuTSSTablaTareas[uiProcPadre].cs / 8],
		sizeof( stuGDTDescriptor ) );
	// divido por 4k porque me estoy copiando el selector de datos del kernel, que tiene
	// granularidad en 4k
	uiFnSetearBaseLimiteDescriptor( uiIndiceGDT_CS, uiBaseSegmento, SEGMENT_SIZE / 4096 ); 

	ucpFnCopiarMemoria( (unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[uiIndiceGDT_DS], 
		(unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[stuTSSTablaTareas[uiProcPadre].ds / 8],
		sizeof( stuGDTDescriptor ) );
	uiFnSetearBaseLimiteDescriptor( uiIndiceGDT_DS, uiBaseSegmento, SEGMENT_SIZE / 4096 );

	/* copio todo el segmento del proc actual al nuevo */
	ucpFnCopiarMemoria( (unsigned char*)uiBaseSegmento, 
		(unsigned char*)pstuPCB[uiProcPadre].uiDirBase, 
		SEGMENT_SIZE );

	/* armamos la TSS del proceso hijo */
	stuTSSTablaTareas[iPosicion].trapbit = 0;
	stuTSSTablaTareas[iPosicion].uIOMapeoBase = sizeof (stuTSS);
	stuTSSTablaTareas[iPosicion].cs = uiIndiceGDT_CS * 8; /* le asignamos los selectores de segmento que */
	stuTSSTablaTareas[iPosicion].ds = uiIndiceGDT_DS * 8; /* encontramos con uiFnBuscarEntradaGDTLibre() */
	stuTSSTablaTareas[iPosicion].ss = uiIndiceGDT_DS * 8;
	stuTSSTablaTareas[iPosicion].es = uiIndiceGDT_DS * 8;
	stuTSSTablaTareas[iPosicion].fs = uiIndiceGDT_DS * 8;
	stuTSSTablaTareas[iPosicion].gs = uiIndiceGDT_DS * 8;
	stuTSSTablaTareas[iPosicion].ss0 = 0x00;
	stuTSSTablaTareas[iPosicion].ss1 = 0x00;
	stuTSSTablaTareas[iPosicion].ss2 = 0x00;
	
	stuTSSTablaTareas[iPosicion].esp0 = (unsigned int) 0;
	stuTSSTablaTareas[iPosicion].esp1 = (unsigned int) 0;
	stuTSSTablaTareas[iPosicion].esp2 = (unsigned int) 0;
	
	/* ahora comenzamos a copiar los valores de los registros, sacandolos de lo
	 * que se fue apilando al momento de comenzar el manejo de la syscall */
	/* el comienzo del stack del kernel se calcula en **system_asm.asm**,
	 * cualquier cambio ahi (que implique desplazar el stack), debe contemplarse
	 * aqui ya que este calculo resultaria invalido */
	uiComienzoStackKernel = 0x200000 - 0x08 - 0x100 
					          * ((pstuPCB[uiProcPadre].uiIndiceGDT_TSS * 8)-0x20);

	stuTSSTablaTareas[iPosicion].esp = *(unsigned int*)(uiComienzoStackKernel-0x08);
	stuTSSTablaTareas[iPosicion].ebp = *(unsigned int*)(uiComienzoStackKernel-0x0c);

	stuTSSTablaTareas[iPosicion].edx = *(unsigned int*)(uiComienzoStackKernel-0x10);
	stuTSSTablaTareas[iPosicion].ecx = *(unsigned int*)(uiComienzoStackKernel-0x14);
	stuTSSTablaTareas[iPosicion].ebx = *(unsigned int*)(uiComienzoStackKernel-0x18);
	stuTSSTablaTareas[iPosicion].eax = 0x00; // al hijo se le devuelve cero!!

	/* calculamos la posicion del fin stack del usuario, tomando la base del segmento 
	 * del proceso (de su PCB) y el valor de ESP (apilado en el stack del kernel) */
	uiStackUsuario = pstuPCB[uiProcPadre].uiDirBase 
				      + stuTSSTablaTareas[iPosicion].esp;
	
	stuTSSTablaTareas[iPosicion].edi = *(unsigned int*)(uiStackUsuario+0x04);
	stuTSSTablaTareas[iPosicion].esi = *(unsigned int*)(uiStackUsuario+0x08);
	stuTSSTablaTareas[iPosicion].eip = *(unsigned int*)(uiStackUsuario+0x1c);
	stuTSSTablaTareas[iPosicion].uiEBandera = *(unsigned int*)(uiStackUsuario+0x24);

	stuTSSTablaTareas[iPosicion].ldt = stuTSSTablaTareas[uiProcPadre].ldt;
	
	/* Si el ultimo proceso en usar la FPU es el padre del nuevo proceso,
	 * su TSS estara desactualizada
	 * y los valores habra que sacarlos directamente de la FPU */
	if(ulUltimoProcesoEnFPU == pstuPCB[uiProcPadre].ulId) {
	  	//Se guarda el estado actual de la FPU, en la TSS del proceso hijo
	  	asm volatile(
			  "fnsave %0\n" //guarda en la TSS del hijo y reinicializa el FPU
			  "fwait	\n" 
			  "frstor %0\n" //vuelve a cargar el FPU con sus valores anteriores
			  : "=m" (stuTSSTablaTareas[iPosicion].fpu));
	} else {
		//La TSS del padre esta actualizada
		//asi que tomamos los valores de la misma
		ucpFnCopiarMemoria( (unsigned char*)&stuTSSTablaTareas[iPosicion].fpu,
							(unsigned char*)&stuTSSTablaTareas[uiProcPadre].fpu,
							sizeof(stuFpu));
	}
	
	uiFnAgregarDescriptorGDT ((unsigned int)
	  			  &stuTSSTablaTareas[iPosicion], 103,
	  			  (D_TSS + D_BIG), uiIndiceGDT_TSS);
	
	// el proceso seleccionado tiene un hijo mas!
	++pstuPCB[uiProcPadre].lNHijos;	

#if 0
	vFnImprimir( "\n fork(): EIP padre (de la TSS): %x", stuTSSTablaTareas[uiProcPadre].eip);
	vFnImprimir( "\n fork(): EIP padre (del stack): %x", stuTSSTablaTareas[iPosicion].eip);
	vFnImprimir( "\n fork(): CS padre (del stack): %x", *(unsigned int*)(uiStackUsuario+0x20));
	vFnImprimir( "\n fork(): EFLAGS padre (del stack): %x", *(unsigned int*)(uiStackUsuario+0x24));

	vFnImprimir( "\n fork(): TSS padre: %x", pstuPCB[uiProcPadre].uiIndiceGDT * 8);
	vFnImprimir( "\n fork(): comienzo stack kernel para esta instancia: %x", uiComienzoStackKernel );
	vFnImprimir( "\n fork(): fin stack usuario para esta instancia: %x", uiStackUsuario );
	vFnImprimir( "\n fork(): ESP original: %x", stuTSSTablaTareas[iPosicion].esp );
	vFnImprimir( "\n fork(): EBP original: %x", stuTSSTablaTareas[iPosicion].ebp );
 
	vFnImprimir( "\n fork(): EDX original: %x", stuTSSTablaTareas[iPosicion].edx );
	vFnImprimir( "\n fork(): ECX original: %x", stuTSSTablaTareas[iPosicion].ecx );
	vFnImprimir( "\n fork(): EBX original: %x", stuTSSTablaTareas[iPosicion].ebx );
	vFnImprimir( "\n fork(): EAX (modificado): %x", stuTSSTablaTareas[iPosicion].eax );
#endif
	
	/* ajustamos el stack en el hijo para que quede como estaba en el padre antes
	 * de ejecutar el syscall fork() (que provoca que se utilizen 0x28 bytes en el
	 * stack entre lo que pushea el micro y lo que pusheamos nosotros en system_asm.asm)*/
	stuTSSTablaTareas[iPosicion].esp += 0x28;  

	iFnCrearPCB( iPosicion, /* PCB asignado al nuevo proceso */
		     0, /* no nos interesa guardar ningun puntero en la PCB */
		     pstuPCB[uiProcPadre].stNombre, /* copiamos el nombre del proceso padre */
		     uiIndiceGDT_CS, /* posicion del descriptor del segmento de codigo para este proceso dentro de la GDT */
		     uiIndiceGDT_DS, /* posicion del descriptor del segmento de datos para este proceso dentro de la GDT */
		     uiIndiceGDT_TSS, /* posicion del descriptor de la TSS dentro de la GDT */
		     iPosicion, /* indice dentro de la tabla de TSS */
		     uiBaseSegmento, /* base del segmento asignado*/
		     SEGMENT_SIZE /* limite del segmento */ );

	return iPosicion;
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnReemplazarProceso( unsigned int uiProceso, unsigned char *ucpCodigo, unsigned int uiTamanio ){
	
	unsigned int uiESPOriginal,
		     uiStackUsuario,
		     uiComienzoStackKernel;

	// no deshabilitamos las interrupciones porque esta funcion se
	// invoca desde el syscall execve(), que a su vez es llamado por
	// el handler de la int 0x80, por lo cual al momento de comenzar
	// la atencion de la interrupcion, se deshabilitan las demas
	
/* no buscamos un segmento libre, ni buscamos entradas disponibles en la GDT
 * porque lo unico que tenemos q realizar es reemplazar el codigo del proceso
 * actual, y resetear el estado, para arrancar este nuevo programa desde "cero" */

	/* copio todo el segmento del proc actual al nuevo */
	ucpFnCopiarMemoria( (unsigned char*)pstuPCB[uiProceso].uiDirBase, ucpCodigo, uiTamanio );

/* la TSS no se altera, ya que no tiene sentido alterar la TSS del proceso actual, ya que de momento q se haga context-switch a otro proceso, lo que este ejecutando en ese momento es lo que va a quedar realmente. todo lo que pongamos en la TSS aqui se pierde...
 * la estrategia consiste en reemplazar los valores en el stack, tanto la parte q esta en el kernel como la del usuario, de manera que al volver del syscall, no
vuelva a la proxima instruccion despues de la invocacion a la int0x80, sino al arranque del proceso nuevo (o sea, tampoco se toca la PCB) */
	
/* ahora comenzamos a copiar los valores de los registros, sacandolos de lo
 * que se fue apilando al momento de comenzar el manejo de la syscall */
/* el comienzo del stack del kernel se calcula en **system_asm.asm**,
 * cualquier cambio ahi (que implique desplazar el stack), debe contemplarse
 * aqui ya que este calculo resultaria invalido */
	uiComienzoStackKernel = 0x200000 - 0x08 - 0x100 
				* ((pstuPCB[uiProceso].uiIndiceGDT_TSS * 8)-0x20);
	
	/* extraemos del stack del kernel, el ESP original: */
	uiESPOriginal = *(unsigned int*)(uiComienzoStackKernel-0x08);

/* calculamos la posicion del fin stack del usuario, tomando la base del segmento 
* del proceso (de su PCB) y el valor de ESP (apilado en el stack del kernel) */
	uiStackUsuario = pstuPCB[uiProceso].uiDirBase + uiESPOriginal;
	
//*(unsigned int*)(uiComienzoStackKernel-0x08) = SEGMENT_SIZE - 0x10; /* ESP */
//*(unsigned int*)(uiComienzoStackKernel-0x0c) = SEGMENT_SIZE - 0x10; /* EBP */

	*(unsigned int*)(uiComienzoStackKernel-0x10) = 0x00; /* EDX */
	*(unsigned int*)(uiComienzoStackKernel-0x14) = 0x00; /* ECX */
	*(unsigned int*)(uiComienzoStackKernel-0x18) = 0x00; /* EBX */
	*(unsigned int*)(uiComienzoStackKernel-0x18) = 0x00; /* EAX */ 

	*(unsigned int*)(uiStackUsuario+0x04) = 0x00; /* EDI */ 
	*(unsigned int*)(uiStackUsuario+0x08) = 0x00; /* ESI */
	*(unsigned int*)(uiStackUsuario+0x1c) = 0x00; /* EIP */
	*(unsigned int*)(uiStackUsuario+0x24) = 0x202L; /* EFLAGS */

	return (int) uiProceso;
}

/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnEliminarProceso( unsigned int uiProceso ){
	/* marcamos el segmento que ocupaba como disponible: */
	vFnImprimir( "\n waitpid(): Liberando el segmento %d, base %x",
		     pstuPCB[ uiProceso ].uiDirBase / SEGMENT_SIZE,
		     pstuPCB[ uiProceso ].uiDirBase );
	mapa_segmentos_set( pstuPCB[ uiProceso ].uiDirBase / SEGMENT_SIZE, 0 );

	/* liberamos las 3 entradas de la GDT reservadas para el descriptor
	 * de segmento de codigo, datos y TSS: */
	vFnImprimir( "\n waitpid(): Liberando entradas %d, %d y %d de la GDT",
		     pstuPCB[ uiProceso ].uiIndiceGDT_CS,
		     pstuPCB[ uiProceso ].uiIndiceGDT_DS,
		     pstuPCB[ uiProceso ].uiIndiceGDT_TSS );
	mapa_gdt_set( pstuPCB[ uiProceso ].uiIndiceGDT_CS, 0 );
	mapa_gdt_set( pstuPCB[ uiProceso ].uiIndiceGDT_DS, 0 );
	mapa_gdt_set( pstuPCB[ uiProceso ].uiIndiceGDT_TSS, 0 );

	vFnImprimir( "\n waitpid(): Eliminando PCB %d (PID %d)",
		     uiProceso,
		     pstuPCB[ uiProceso ].ulId );

/* liberamos la PCB que utilizaba (y por consiguiente, la entrada en la tablas
 * de TSS */
	pstuPCB[ uiProceso ].iEstado = PROC_ELIMINADO;

	return uiProceso;
}

/*agregado por el grupo
 *La función iFnClonarProceso, crea un nuevo proceso, pero compartiendo
 *el segmento de datos y codigo del segmento que lo llamó
 * */
/******************************************************************************
Funcion:
Descripcion:
Recibe:   
Devuelve: 
*******************************************************************************/
int iFnClonarProceso()
{
	unsigned int uiIndiceGDT_TSS;
	unsigned int uiIndiceGDT_DS;
	unsigned int uiBaseSegmento;
	int iPosicion;
	unsigned int uiComienzoStackKernel, uiStackUsuario;



if(!uiIndiceGDT_TSS)
{	vFnImprimir("ENOMEM\n");
	return -ENOMEM;
}

iPosicion = iFnBuscarPCBLibre();
uiBaseSegmento = uiFnBuscarSegmentoLibre() * SEGMENT_SIZE;

uiIndiceGDT_DS = uiFnBuscarEntradaGDTLibre();
uiIndiceGDT_TSS = uiFnBuscarEntradaGDTLibre();
//vFnImprimir("Indice GDT: %d\n",uiIndiceGDT_TSS);

ucpFnMemCopy( (unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[uiIndiceGDT_DS], 
		(unsigned char*) &pstuTablaGdt->stuGdtDescriptorDescs[stuTSSTablaTareas[ulProcActual].ds / 8],
		sizeof( stuGDTDescriptor ) );
	uiFnSetearBaseLimiteDescriptor( uiIndiceGDT_DS, uiBaseSegmento, SEGMENT_SIZE / 4096 );
//vFnImprimir("Copio ds\n");

	ucpFnMemCopy( (unsigned char*)uiBaseSegmento, 
		(unsigned char*)pstuPCB[ulProcActual].uiDirBase, 
		SEGMENT_SIZE );
//vFnImprimir("Copio segmento\n");

	stuTSSTablaTareas[iPosicion].trapbit = 0;
	stuTSSTablaTareas[iPosicion].uIOMapeoBase = sizeof(stuTSS);
	stuTSSTablaTareas[iPosicion].cs = stuTSSTablaTareas[ulProcActual].cs;
	stuTSSTablaTareas[iPosicion].ds = uiIndiceGDT_DS * 8;
	stuTSSTablaTareas[iPosicion].ss = uiIndiceGDT_DS * 8;
	stuTSSTablaTareas[iPosicion].es = uiIndiceGDT_DS * 8;
	stuTSSTablaTareas[iPosicion].fs = uiIndiceGDT_DS * 8;
	stuTSSTablaTareas[iPosicion].gs = uiIndiceGDT_DS * 8;
	stuTSSTablaTareas[iPosicion].ss0 = 0;
	stuTSSTablaTareas[iPosicion].ss1 = 0;
	stuTSSTablaTareas[iPosicion].ss2 = 0;
	stuTSSTablaTareas[iPosicion].esp0 = 0;
	stuTSSTablaTareas[iPosicion].esp1 = 0;
	stuTSSTablaTareas[iPosicion].esp2 = 0;
	//vFnImprimir("ds padre %x ds hijo %x\n",stuTSSTablaTareas[ulProcActual].ds,stuTSSTablaTareas[iPosicion].ds);
	vFnImprimir("\nVerificacion de Clone\n CS padre: %x CS hijo: %x\n",stuTSSTablaTareas[ulProcActual].cs,stuTSSTablaTareas[iPosicion].cs );

	uiComienzoStackKernel = 0x200000 - 0x08 - 0x100 
					          * ((pstuPCB[ulProcActual].uiIndiceGDT_TSS * 8)-0x20);

	stuTSSTablaTareas[iPosicion].esp = *(unsigned int*)(uiComienzoStackKernel-0x08);
	stuTSSTablaTareas[iPosicion].ebp = *(unsigned int*)(uiComienzoStackKernel-0x0c);

	stuTSSTablaTareas[iPosicion].edx = *(unsigned int*)(uiComienzoStackKernel-0x10);
	stuTSSTablaTareas[iPosicion].ecx = *(unsigned int*)(uiComienzoStackKernel-0x14);
	stuTSSTablaTareas[iPosicion].ebx = *(unsigned int*)(uiComienzoStackKernel-0x18);
	stuTSSTablaTareas[iPosicion].eax = 0x00;


	uiStackUsuario = pstuPCB[ulProcActual].uiDirBase 
				      + stuTSSTablaTareas[iPosicion].esp;
	
	stuTSSTablaTareas[iPosicion].edi = *(unsigned int*)(uiStackUsuario+0x04);
	stuTSSTablaTareas[iPosicion].esi = *(unsigned int*)(uiStackUsuario+0x08);
	stuTSSTablaTareas[iPosicion].eip = *(unsigned int*)(uiStackUsuario+0x1c);
	stuTSSTablaTareas[iPosicion].uiEBandera = *(unsigned int*)(uiStackUsuario+0x24);

	stuTSSTablaTareas[iPosicion].ldt = 0; //stuTSSTablaTareas[ulProcActual].ldt;

	
	/* Si el ultimo proceso en usar la FPU es el padre del nuevo proceso,
	 * su TSS estara desactualizada
	 * y los valores habra que sacarlos directamente el FPU */
	if(ulUltimoProcesoEnFPU == pstuPCB[ulProcActual].ulId) {
	  //Se guarda el estado actual de la FPU, en la TSS del proceso hijo
	  asm volatile(
			  "fnsave %0\n" //guarda en la TSS del hijo y reinicializa el FPU
			  "fwait	\n" 
			  "frstor %0\n" //vuelve a cargar el FPU con sus valores anteriores
			  : "=m" (stuTSSTablaTareas[iPosicion].fpu));
	} else {
		//La TSS del padre esta actualizada
		//asi que tomamos los valores de la misma
		ucpFnCopiarMemoria( (unsigned char*) &stuTSSTablaTareas[iPosicion].fpu,
							(unsigned char*) &stuTSSTablaTareas[ulProcActual].fpu,
							sizeof(stuFpu));
	}
	
	uiFnAgregarDescriptorGDT ((unsigned int)
	  			  &stuTSSTablaTareas[iPosicion], 103,
	  			  (D_TSS + D_BIG), uiIndiceGDT_TSS);
	
	++pstuPCB[ulProcActual].lNHijos;	

stuTSSTablaTareas[iPosicion].esp += 0x28;
			
		iFnCrearPCB(iPosicion, 
				0,
				pstuPCB[ulProcActual].stNombre,
				pstuPCB[ulProcActual].uiIndiceGDT_CS,
				pstuPCB[ulProcActual].uiIndiceGDT_DS,
				uiIndiceGDT_TSS,
				iPosicion,
				uiBaseSegmento,
				SEGMENT_SIZE);

return 0;

}

