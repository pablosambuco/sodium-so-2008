/*! \file ramfs.c
 *    \brief algoritmos de acceso a la unidad virtual en RAM
 *   
 *	Contiene las estructuras de datos y algoritmos para acceso al disco virtual
 */

#include <fs/ramfs.h>
#include <kernel/libk/libk.h>
#include <kernel/libk/string.h>
#include <kernel/mem/memoria_k.h>
#include <kernel/definiciones.h>
#include <kernel/syscall.h> //para c�digos de error.
#include <shell/teclado.h>
#include <shell/shell.h>
#include <shell/sys_video.h>
#include <video.h>


extern stuVentana pstuVentana[HWND_VENTANA_MAX]; /*!< Extern a estructura que guarda el formato de las ventanas activas */

stDiscoRAM stDiscosRAM[CANTMAXDISCOSRAM]; /*!< Vector de descriptores de discos RAM */

/**
 * \brief Funci�n que inicializa los discos RAM.
 * 
 * Inicializa tres discos RAM, cargando valores apropiados en el vector de 
 * estructuras stDiscosRAM[].
 *  - El DISCO_USR se define en memoria baja, reclamando parte de la memoria que
 *  queda disponible entre el kernel y los 640kb.
 *  - El DISCO_LOG se define en memoria alta, tomando relativamente gran
 *  capacidad (3MB), que ser� destinada a guardar los logs generados durante
 *  las pruebas.
 *  - El DISCO_LOTES es m�s peque�o y se utilizar� temporalmente para editar
 *  los lotes de pruebas. Tambi�n se define en memoria alta.
 *
 * \returns Entero que indica EXITO si la operaci�n se realiz� correctamente o
 * 	un c�digo de error.
 * \sa ucpFnCopiarMemoria()
 */

int iFnRamFsInit(void)
{
	stDiscoRAM *disco_usr;
	stDiscoRAM *disco_log;
	stDiscoRAM *disco_lotes;

	disco_usr = &stDiscosRAM[DISCO_USR];
	disco_log = &stDiscosRAM[DISCO_LOG];
	disco_lotes = &stDiscosRAM[DISCO_LOTES];

	ucpFnCopiarMemoria((unsigned char *) disco_usr->strEtiqueta,
			   (unsigned char *) "USR", 4);
	ucpFnCopiarMemoria((unsigned char *) disco_log->strEtiqueta,
			   (unsigned char *) "LOGS", 5);
	ucpFnCopiarMemoria((unsigned char *) disco_lotes->strEtiqueta,
			   (unsigned char *) "LOTES", 6);

	ucpFnCopiarMemoria((unsigned char *) &(disco_usr->strPMontaje),
			   (unsigned char *) "/mnt/usr", 9);
	ucpFnCopiarMemoria((unsigned char *) &(disco_log->strPMontaje),
			   (unsigned char *) "/mnt/log", 9);
	ucpFnCopiarMemoria((unsigned char *) &(disco_lotes->strPMontaje),
			   (unsigned char *) "/mnt/lotes", 10);

// El disco usuario se ubica en memoria baja porque es el que tiene que 
// apropiarse de la info que subimos del disquete en modo real.
// Los dem�s discos no tienen esta restricci�n.
	disco_usr->dwTamanio = 100 * 1024;	//intento reservar 100kb

	if ((disco_usr->dwDireccionInicial =
	     (dword) pvFnKMalloc(disco_usr->dwTamanio, MEM_BAJA)) == 0) {
		vFnImprimir
		    ("\nNo se pudo reservar suficiente memoria para el disco %s",
		     disco_usr->strEtiqueta);
		return -ENOMEM;
	}
	disco_usr->pstDirRaiz = (void *) 0x80000;//disco_usr->dwDireccionInicial;
	disco_usr->dwEntradasRaiz = 20;
	disco_usr->bActivo = 1;

	disco_log->dwTamanio = 2 * 1024 * 1024;	//intento reservar 2MB

	if ((disco_log->dwDireccionInicial =
	     (dword) pvFnKMalloc(disco_log->dwTamanio, MEM_ALTA)) == 0) {
		vFnImprimir
		    ("\nNo se pudo reservar suficiente memoria para el disco %s",
		     disco_log->strEtiqueta);
		return -ENOMEM;
	}
	disco_log->pstDirRaiz = (void *) disco_log->dwDireccionInicial;
	disco_log->dwEntradasRaiz = 20;
	disco_log->bActivo = 1;

	disco_lotes->dwTamanio = 20 * 1024;	//intento reservar 20kb

	if ((disco_lotes->dwDireccionInicial =
	     (dword) pvFnKMalloc(disco_lotes->dwTamanio, MEM_ALTA)) == 0) {
		vFnImprimir
		    ("\nNo se pudo reservar suficiente memoria para el disco %s",
		     disco_lotes->strEtiqueta);
		return -ENOMEM;
	}
	disco_lotes->pstDirRaiz = (void *) disco_lotes->dwDireccionInicial;
	disco_lotes->dwEntradasRaiz = 20;
	disco_lotes->bActivo = 1;

	return EXITO;

}

/**
 * \brief Formatea un String para normalizar el nombre
 *
 * Funci�n que lee un string de nombre de archivo del tipo "P1______BIN" y 
 * lo convierte a "P1.BIN\\0".
 *
 * \note Esta funci�n asume que el string 8.3 no tiene \\0 de finalizaci�n, 
 * por lo que ni siquiera lo buscamos. Asumimos que estamos manejando entradas
 * del listado.bin que se define para que el loader cargue dichos archivos en 
 * memoria durante la inicializaci�n del sodium.
 * \note IMPORTANTE: El string que representar� al nombre normalizado S� terminar�
 * con \\0
 *
 * \param strNomArch83
 * 		Puntero a string que contiene el nombre de archivo sin punto.
 * \param strNomArch
 * 		Puntero a string que contendr� el nombre de archivo con punto.
 * 		
 * \returns 	Cantidad de caracteres copiados al string destino, incluyendo
 * 		 el \\0. 
 */
int iFnNormalizarNomArch(const char *strNomArch83, char *strNomArch)
{
	int iN = 0, iJ = 0;
	for (iJ = 0; iJ < 8; iJ++) {
		if (strNomArch83[iJ] != ' ') {
			strNomArch[iN++] = strNomArch83[iJ];
		}
	}
	strNomArch[iN] = '.';
	iN++;
	for (iJ = 8; iJ < 11; iJ++) {
		if (strNomArch83[iJ] != ' ') {
			strNomArch[iN++] = strNomArch83[iJ];
		}
	}
	strNomArch[iN] = '\0';
	return iN;
}


/** 
 * \brief Devuelve direccion lineal de memoria de archivo buscado
 * 
 * Funci�n que de acuerdo al directorio de b�squeda y al nombre del archivo
 * buscado, devuelve la direcci�n lineal donde se encuentra en memoria.
 * Si se desea m�s informaci�n del mismo (el tama�o, por ejemplo), se puede
 * obtener pasando por par�metro la direcci�n de una variable de tipo 
 * "puntero de stEntradaLS". Luego podr� usar la nueva direcci�n, que apuntar�
 * a la entrada correspondiente del directorio ra�z, para acceder a la 
 * informaci�n adicional.
 * 
 * \param pstDir  
 * 	Puntero a directorio en donde se realizar� la b�squeda.
 * \param strNombreArchivo
 * 	Puntero a string que contiene el nombre del archivo a buscar.
 * \param ppstEntradaLSArchEnc
 * 	Puntero a una variable puntero a entrada de listado que terminar� apuntando
 * 	a la entrada del directorio ra�z que corresponde al archivo encontrado, o 
 * 	apuntar� a NULL si no se encontr�.
 * 
 * \returns 0 Para operaci�n exitosa o C�digo de Error.
 */

int iFnBuscarArchivo(stDirectorio * pstDir,
		      char *strNombreArchivo, stEntradaLS ** ppstEntradaLSArchEnc)
{
	char strNomTemp[13];

	int iN;
	for (iN = 0; iN < CANTMAXDIRENT; iN++) {
		iFnNormalizarNomArch(pstDir->stEnt[iN].strNombreArchivo,
				     strNomTemp);
		if (iFnCompararCadenas(strNomTemp, strNombreArchivo) == 1) {
			*ppstEntradaLSArchEnc = (stEntradaLS *) & pstDir->stEnt[iN];
			return EXITO;
		}
	}
	*ppstEntradaLSArchEnc = NULL;
	return -ENOENT;
}

/** 
 * \brief Funci�n que de acuerdo al punto de montaje solicitado, devuelve un puntero
 * a dicho directorio.
 *
 * \param strPMontaje
 * 	Puntero a string que indica el punto de montaje.
 * \param ppstDir
 * 	Puntero a una variable de tipo puntero de estructura de directorio que 
 * 	representar� al punto de montaje solicitado.
 *
 * \returns 0 Para operaci�n exitosa o C�digo de Error.
 */
int iFnObtenerDirectorio(char *strPMontaje, stDirectorio ** ppstDir)
{
	stDiscoRAM *pstDisco;
	int iValRetorno;

	if ((iValRetorno = iFnObtenerDisco(strPMontaje, &pstDisco)) < 0) {
		*ppstDir = NULL;
		return iValRetorno;
	}
	*ppstDir = (stDirectorio *) pstDisco->pstDirRaiz;
	return EXITO;
}


/** 
 * \brief Funci�n que de acuerdo al punto de montaje solicitado, devuelve un puntero
 * al disco correspondiente.
 * 
 * \param strPMontaje
 * 	Puntero a string que indica el punto de montaje.
 * \param ppstDisco
 * 	Puntero una variable puntero del tipo estructura de disco que terminar�
 * 	apuntando a la entrada correspondiente al disco que haya sido montado
 * 	en ese punto de montaje, o NULL si el punto de montaje no corresponde
 * 	a ning�n disco montado.
 *
 * \returns C�digo de Error en caso de fallo.
 */

int iFnObtenerDisco(char *strPMontaje, stDiscoRAM **ppstDisco)
{
	int iN;
	for (iN = 0; iN < CANTMAXDISCOSRAM; iN++) {
		if (iFnCompararCadenas
		    (stDiscosRAM[iN].strPMontaje, strPMontaje) == 1) {
			if (stDiscosRAM[iN].bActivo == 1) {
				*ppstDisco = &stDiscosRAM[iN];
				return EXITO;
			} else {
				*ppstDisco = NULL;
				return -ENOMNT;
			}
		}
	}
	*ppstDisco = NULL;
	return -ENOENT;

}

/**
 * \brief Imprime listado archivos del dir principal
 *
 * Funci�n que imprime en pantalla el listado de archivos que se encuentran
 * en el directorio principal del disco ram "USR", que es el que se llena con
 * los archivos cargados en memoria cuando todav�a estamos trabajando en modo
 * real.
 *
 * \param strParams 
 * 	No utilizado todav�a. Par�metro donde se podr�a pasar wildcards o 
 * 	el nombre del directorio a listar.
 * \returns
 * 	Entero indicando 0 en �xito o -1 en caso de error.
 * \sa iFnRamFsInit()
 */

int iFnRamFsLs(char *strParams)
{
	int 	iN = 0,
		iValRetorno;

	char strNombreArchTmp[13];

	stDirectorio 	*pstDir 	= NULL;
	stDiscoRAM 	*pstDisco 	= NULL;
	stEntradaLS 	*pstEntradaLS 	= NULL;

	if ((iValRetorno = iFnObtenerDisco("/mnt/usr", &pstDisco)) < 0) {
		vFnImprimir("\nNo se pudo obtener el disco");
		return iValRetorno;
	}

	vFnImprimir("\nEtiqueta del volumen: %s, montado en %s",
		    pstDisco->strEtiqueta, pstDisco->strPMontaje);

	pstDir = pstDisco->pstDirRaiz;

	vFnImprimir
	    ("\nArchivo    \tTamanio (Bytes)\tDir.\t(Lineal)\t(Seg:Offset)\n");


	while ((pstDir->stEnt[iN].strNombreArchivo[0]) != ' ') {
		pstEntradaLS = &pstDir->stEnt[iN];

/* Aunque esto ensucia, es un hack necesario para mantener alineadas las 
 * columnas del listado, porque todav�a no tenemos implementado el %(NUM)s 
 * en el string de formato de la vFnImprimir
 */
		ucpFnCopiarMemoria((unsigned char *) strNombreArchTmp, (unsigned char *) "           ", 12);
		iValRetorno = iFnNormalizarNomArch (pstEntradaLS->strNombreArchivo,
				      		    strNombreArchTmp);
		strNombreArchTmp[iValRetorno]=' ';strNombreArchTmp[12]='\0';
/* end_of_hack :P */

		vFnImprimir ("%s\t%d\t\t\t%x\t\t%x:%x\n",
			     strNombreArchTmp,
			     pstEntradaLS->dwTamanio,
			     DIR_LINEAL (pstEntradaLS->wSeg, 
					 pstEntradaLS->wOff),
			     pstEntradaLS->wSeg, 
			     pstEntradaLS->wOff
			    );

		if (((iN + 1) % (pstuVentana[HWND_COMANDO].iAlto -
				 1)) == 0 && iN > 0)
			cFnPausa();
		iN++;
	}
	vFnImprimir("\nEOD");
	return EXITO;
}
