#include <kernel/definiciones.h>
#include <kernel/system.h>
#include <kernel/system_asm.h>
#include <kernel/puertos.h>
#include <kernel/syscall.h>
#include <kernel/pcb.h>
#include <kernel/gdt.h>
#include <kernel/libk/string.h>
#include <kernel/libk/libk.h>

#include <kernel/mem/memoria_s.h>
#include <kernel/mem/memoria_k.h>
#include <kernel/mem/paginas.h>

#include <shell/shell.h>
#include <shell/teclado.h>
#include <shell/sys_video.h>
#include <video.h>

#include <shell/cmd_log.h>
#include <shell/cmd_lotes.h>
#include <shell/cmd_planif.h>
#include <shell/cmd_regs.h>
#include <fs/ramfs.h>


int iAutoCompletarCancelado = 0;
int iInicializarVariablesEnv = 1;

extern int iCantidadCaracteres;
extern int bActivarTeclado;
extern stuTSS stuTSSTablaTareas[CANTMAXPROCS];
extern stuPCB pstuPCB[CANTMAXPROCS];
extern int iFnSistema();
extern int iFnProceso1();

extern dword pdwGDT;
extern stuEstructuraGdt *pstuTablaGdt;
extern int uiUltimoPid;

extern void vFnBorrarCaracter(int hwnd);
extern void vFnCambiarVisibilidadVentanaProceso();
extern unsigned int uiTamanioMemoriaBaja;
extern unsigned int uiTamanioMemoriaBios;
extern unsigned int uiUnidadBoot;
extern unsigned int uiTamanioBSS;
extern unsigned int uiTamanioKernel;
extern unsigned int uiModoMemoria;
extern stuVentana pstuVentana[HWND_VENTANA_MAX];
extern int iTamanioPagina;

extern semaforo semaforosEnElSistema[CANTMAXSEM];
extern shMem memoriasCompartidas[CANTMAXSHM];
void vFnInicializarEnv();

int cCaracterDevuelto = 13;	//devuelve el caracter tocado luego del autocompletar

/**
\fn void vFnLoopPrincipalShell()
\brief Función principal del shell. Detecta cCaracteres no tomados del buffer del teclado y los guarda 
en el buffer del intérprete de comandos. Maneja el enter y el backspace.
*/
void vFnLoopPrincipalShell()
{
	unsigned char cCaracter;
	int continua = 0;
	//me indica si despues de un autocompletar cancelado, debo saltearme
	//la peticion de nuevo caracter (evita tener que tocar dos veces una tecla
	//luego de cancelar el autocompletar)

	if (iInicializarVariablesEnv == 1) {
		vFnInicializarEnv();
		iInicializarVariablesEnv = 0;
	}
	bTerminalActiva = TERM_SODIUM;

	while (1) {
		if (continua == 0) {
			cCaracter = cFnGetChar();
		} else {
			continua = 0;
		}

		//Si presiono F1 o F2, cambio de terminal
		switch (cCaracter) {
            case TECLA_F1:
                if (bTerminalActiva != TERM_SODIUM)
                    vFnCambiarTerminal();
                continue;
            case TECLA_F2:
                if (bTerminalActiva != TERM_LOG)
                    vFnCambiarTerminal();
                continue;
		}

		//Si estoy en la terminal de log...
		if (bTerminalActiva == TERM_LOG) {
			vFnTermLog(cCaracter);
			continue;
		}

		//Si estoy en la "terminal" de Shell...
		switch (cCaracter) {
            case TECLA_ENTER:
                //Llamo al intérprete de comandos.
                vFnShell();

                //Espero a ver si se toca una tecla despues de autocompletar
                if (cCaracterDevuelto != 13) {
                    cCaracter = cCaracterDevuelto;
                    //Reinicio el caracter global, para no tener conflicto
                    cCaracterDevuelto = 13;

                    continua = 1;
                }
                continue;
            case TECLA_BACKSPACE:
				if (iCantidadCaracteres > 0) {
					vFnImprimir("%c", cCaracter);
					stBufferShell [iCantidadCaracteres--] = '\0';
				}
                continue;
            default:
                //Si todavia no se llenó el buffer...
                if (iCantidadCaracteres < TAMANIO_BUFFER_SHELL)
                {
                    //Si es un caracter imprimible...
                    if (cCaracter < 200) {
                        //Se imprime y se coloca en el buffer del shell
                        vFnImprimir ("%c", cCaracter);
                        stBufferShell [iCantidadCaracteres++] = cCaracter;
                    }
                }
		}
	}
}


/**
\fn int iFnEsCmdShell(char *stComando)
\brief Busca en el buffer de cCaracteres la existencia del comando pasado como parámetro

\param stComando Puntero a cCaracter a una cadena de texto que contiene el nombre del comando buscado.
\return Integer que indica la posicion inicial en el buffer donde se encontro el comando o -1 si no lo encontró.
\date 09/04/2006
*/
int iFnEsCmdShell(char *stComando)
{
	int iN = 0, iJ = 0, iK = 0, iPosEncontrado = -1, iNroFaltantes;

	if (iCantidadCaracteres > 0) {
		while (((stBufferShell[iN] == ' ') || (stBufferShell[iN] == '\t')) && iN < TAMANIO_BUFFER_SHELL)	//busco el primer cCaracter
			//no-blanco,
		{		//típicamente es el primer cCaracter, pero
			iN++;	//lo buscamos igual en caso que hayan espacios o
		}		//tabulaciones antes del comando a interpretar

		if (iN < TAMANIO_BUFFER_SHELL)	//si iN==TAMANIO_BUFFER_SHELL, no hay ningun comando para interpretar.
			//Se llegó al final del buffer y todos los cCaracteres eran
			//blancos
		{
			iPosEncontrado = iN;
			while ((stComando[iJ] != ' ')
			       && (stComando[iJ] != '\0')
			       && (stBufferShell[iN] != ' ')
			       && (stBufferShell[iN] != '\t')
			       && (stBufferShell[iN] != '\0')
			       && (iPosEncontrado != -1)) {
				if (stComando[iJ] != stBufferShell[iN])
					iPosEncontrado = -1;
				iN++;
				iJ++;
			}
		}
//si no hubo errores de concordancia pero tampoco se llego a evaluar toda la
//longitud del comando pasado como parametro, completaremos el buffer con el.
		if (iPosEncontrado != -1 && stComando[iJ] != '\0') {
			//encuentro la cantidad de cCaracteres que faltan agregar
			for (iNroFaltantes = 0;
			     stComando[iJ] != '\0'; iJ++, iNroFaltantes++);

			//incremento la cantidad de cCaracteres declarados en el buffer a la nueva
			//cantidad.
			iCantidadCaracteres += iNroFaltantes;

			// desplazo los parametros hacia la derecha nrofaltantes posiciones.
			for (iK = iCantidadCaracteres;
			     (iK - iNroFaltantes) >= iN; iK--)
				stBufferShell[iK] =
				    stBufferShell[iK - iNroFaltantes];

			//vuelvo a iJ a su valor original para rellenar cn el resto del comando...
			iJ -= iNroFaltantes;

			for (; iNroFaltantes > 0;
			     iNroFaltantes--, iN++, iJ++) {
				stBufferShell[iN] = stComando[iJ];
			}
			for (iN = 0;
			     iN <
			     (iCantidadCaracteres +
			      iFnLongitudCadena(STRPROMPTLINE)); iN++)
				vFnBorrarCaracter(0);

			vFnImprimirPrompt();
			vFnImprimir("%s", stBufferShell);
			cCaracterDevuelto = cFnGetChar();
			if (cCaracterDevuelto != 13) {
				iAutoCompletarCancelado = 1;
				return (-1);
			}
		}
	}

	return iPosEncontrado;

}



/**
  \fn int iFnGetArg(int iPos, int iNumero, char *stResultado, int iAnchoMaximo)
  \brief Entrega el parámetro requerido de la línea de comando.
  \param iPos La primer posición en el buffer desde donde comenzar a buscar.
  \param iNumero El numero de parámetro que se solicita del buffer de entrada.
  \param stResultado Un puntero a cadena conde copiará el parámetro recuperado
  \param iAnchoMaximo Un entero que le indica la longitud máxima de la cadena que recibir el parámetro
  \return Entero que indica:
                1 si encontró el parámetro
                2 si lo encontró pero no pudo recuperarlo en su totalidad
                0 si no lo encontró.
  \date 09/04/2006
*/
int iFnGetArg(int iPos, int iNumero, char *stResultado, int iAnchoMaximo)
{
	int iN = iPos, iJ = 0, iK = 0;

	while (stBufferShell[iN] != '\0' && iJ < iNumero) {
		if ((stBufferShell[iN] == ' ')
		    && (stBufferShell[iN + 1] != ' '))
			iJ++;

		iN++;
	}

	if (iJ == iNumero)	//si se encontró el parametro buscado lo copio
	{
		do {
			stResultado[iK] = stBufferShell[iK + iN];
			iK++;
		}
		while (iK < (iAnchoMaximo - 1)
		       && stBufferShell[iK + iN] != ' '
		       && (iK + iN) < iCantidadCaracteres);

		stResultado[iK] = '\0';

		//vFnImprimir("\ngetarg: nro %d, $%s$",iNumero,chrResultado);
		if (iK < (iAnchoMaximo - 1)) {
			return (1);
		} else {
			vFnImprimir("\ngetArg():El tamanio es invalido.");
			return (2);
		}
	} else {
		vFnImprimir("\ngetArg():parametro no encontrado.");
		return (0);
	}
}

/**
\fn void vFnImprimirContexto()
\brief Imprime la estetica de la ventana como los separadores y la informacion de atajos rapidos.
*/
void vFnImprimirContexto()
{
	vFnImprimirSeparadores();
	vFnImprimirMenu();
}

/**
\fn void vFnImprimirSeparadores()
\brief Imprime la estetica de la ventana.
*/
void vFnImprimirSeparadores()
{
	int iN;

	vFnImprimirVentana(HWND_SEP_CSW, "\n");
	vFnImprimirVentana(HWND_SEP_PROCESOS, "\n");
	vFnImprimirVentana(HWND_SEP_AYUDA, "\n");
	for (iN = 0; iN < 79; iN++) {
		vFnImprimirVentana(HWND_SEP_CSW, "%c", 0XC4);
		vFnImprimirVentana(HWND_SEP_PROCESOS, "%c", 0xCD);
		vFnImprimirVentana(HWND_SEP_AYUDA, "%c", 0xCD);
		vFnImprimirVentana(HWND_LOG_AYUDA, "%c", 0xCD);
	}
}

/**
\fn void vFnShell()
\briaef Interpreta el buffer de cCaracteres y ejecuta los comandos ingresados. 
\date 09/04/2006
*/
void vFnShell()
{
	int iComandoPos;
	int iComandoIncorrecto = 0;

	stBufferShell[iCantidadCaracteres] = '\0';

	/////////////////////////////////////////////////////////////////////////////

	if ((iFnEsCmdShell("ayuda")) >= 0 || (iFnEsCmdShell("help")) >= 0) {
		vFnMenuAyuda();
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("bitmap")) >= 0) {
		if (uiModoMemoria == MODOPAGINADO) {
			vFnImprimir("\nMAPA DE PAGINAS:");
			vFnMenuBitmap();
		} else
			iComandoIncorrecto = 1;
	}
	/////////////////////////////////////////////////////////////////////////////
  else if (iFnEsCmdShell ("verSem") >= 0)
    {
	vFnMenuVerSemaforos();
    }
  else if (iFnEsCmdShell ("verShm") >= 0)
    {
	vFnMenuVerShm();
    }
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("check")) >= 0) {
		if (uiModoMemoria == MODOPAGINADO)
			vFnMenuCheck(iComandoPos);
		else
			iComandoIncorrecto = 1;
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("sysgetpid")) >= 0) {
		long ret = lFnSysGetPid();
		vFnImprimir("\nel syscall getpid() me devolvio %d", ret);
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("sysgetppid")) >= 0) {
		long ret = lFnSysGetPPid();
		vFnImprimir("\nel syscall getppid() me devolvio %d", ret);
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("syswaitpid")) >= 0) {
		char strPID[16];
		int ret = 0, status = 0, pid = 0;
		if ((iFnGetArg
		     (iComandoPos, 1, strPID, sizeof(strPID)) == 1)) {
			pid = iFnCtoi(strPID);
			ret = lFnSysWaitPid(pid, &status, 0);
			vFnImprimir
			    ("\nel syscall waitpid() me devolvio %d, y estado quedo en %d",
			     ret, status);
		}
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("syskill")) >= 0) {
		char strPID[16], strSIG[16];
		int iPID, iSIG;

		if ((iFnGetArg
		     (iComandoPos, 1, strPID, sizeof(strPID)) == 1)) {
			if ((iFnGetArg
			     (iComandoPos, 2, strSIG,
			      sizeof(strSIG)) == 1)) {
				iPID = iFnCtoi(strPID);
				iSIG = iFnCtoi(strSIG);
				vFnImprimir
				    ("\n Enviando la sig %d al proceso %d... ",
				     iSIG, iPID);
				if (lFnSysKill(iPID, iSIG))
					vFnImprimir
					    ("ERROR (PID o SIG invalido)");
				else
					vFnImprimir("OK");
			} else {
				vFnImprimir
				    ("\n Error, falta el numero de sig (ver ayuda)");
			}
		} else {
			vFnImprimir("\n uso: syskill PID SIG"
				    "\n SIG: "
				    "\n  SIGFPE  %d"
				    "\n  SIGSEGV %d"
				    "\n  SIGINT  %d"
				    "\n  SIGSTOP %d"
				    "\n  SIGCONT %d"
				    "\n  SIGKILL %d"
				    "\n  SIGTERM %d",
				   SIGFPE, SIGSEGV, SIGINT, SIGSTOP, SIGCONT, SIGKILL, SIGTERM);
		}
	}
	/////////////////////////////////////////////////////////////////////////////

	else if ((iComandoPos = iFnEsCmdShell("planif")) >= 0) {
		vFnMenuPlanif(iComandoPos);
	}
	/////////////////////////////////////////////////////////////////////////////

	else if ((iComandoPos = iFnEsCmdShell("lote")) >= 0) {
		vFnMenuLotes(iComandoPos);
	}
	/////////////////////////////////////////////////////////////////////////////

	else if ((iComandoPos = iFnEsCmdShell("log")) >= 0) {
		vFnMenuLog(iComandoPos);
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("init")) >= 0) {
		vFnImprimir("\ninstanciando init");
		vFnMenuInstanciarInit();
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("pcb")) >= 0) {
		vFnMenuPCB(iComandoPos);
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("idle")) >= 0) {
		vFnImprimir("\nllamando a la nula");
		lFnSysIdle();
	}	
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("reboot")) >= 0) {
		vFnImprimir("\nReiniciando el sistema...");
		lFnSysReboot(0);

	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("cambiateclado")) >= 0) {
		vFnMenuCambiaTeclado(iComandoPos);
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("cls")) >= 0) {
		vFnMenuCls();
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("desc")) >= 0) {
		vFnMenuDesc(iComandoPos);
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("dump")) >= 0) {
		vFnMenuDump(iComandoPos);
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("ejecutar")) >= 0) {
		vFnMenuEjecutar(iComandoPos);
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("stack")) >= 0) {
		vFnMenuStack(iComandoPos);
	}
	/////////////////////////////////////////////////////////////////////////////

	else if ((iComandoPos = iFnEsCmdShell("Execve")) >= 0) {
		vFnImprimir("\nTODO Execve");
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("exec")) >= 0) {
		if (uiModoMemoria == MODOPAGINADO)
			vFnMenuExec(iComandoPos);
		else
			vFnMenuExecSeg(iComandoPos);

	}
	/////////////////////////////////////////////////////////////////////////////
	else if (iFnEsCmdShell("gdt") >= 0) {
		vFnMenuGdt();
	}
	/////////////////////////////////////////////////////////////////////////////
	else if (iFnEsCmdShell("idt") >= 0) {
		vFnMenuIdt();
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("kill")) >= 0) {
		if (uiModoMemoria == MODOPAGINADO)
			vFnMenuKill(iComandoPos);
		else		//segmentacion
			vFnMenuKillSeg(iComandoPos);
	}
	/////////////////////////////////////////////////////////////////////////////
	else if (iFnEsCmdShell("ls") >= 0) {
		vFnMenuLs(iComandoPos);
	}
	/////////////////////////////////////////////////////////////////////////////
	else if (iFnEsCmdShell("mem") >= 0) {
		vFnMenuMem();
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("pag")) >= 0) {
		if (uiModoMemoria == MODOPAGINADO) {
			vFnImprimir("\nMAPA DE PAGINAS:");
			vFnMenuPag(iComandoPos);
		} else {
			iComandoIncorrecto = 1;
		}
	}
	/////////////////////////////////////////////////////////////////////////////
	else if (iFnEsCmdShell("yield") >= 0) {
		vFnImprimir("\nEl shell cede la ejecucion\n");
		lFnSysSchedYield();
	}
	/////////////////////////////////////////////////////////////////////////////
	else if (iFnEsCmdShell("interval") >= 0) {
		vFnImprimir("parece ser el intervalo");
		vFnImprimir("Time slice: %l",lFnSysSchedRrGetInterval( 0,0));
	}		
	/////////////////////////////////////////////////////////////////////////////
	else if ((iFnEsCmdShell("ps")) >= 0) {
		vFnMenuPs();
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iFnEsCmdShell("segs")) >= 0) {
		if (uiModoMemoria == MODOSEGMENTADO)
			vFnMenuSegs();
		else
			iComandoIncorrecto = 1;
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("set")) >= 0) {
		vFnMenuSet(iComandoPos);
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("sumafpu")) >= 0) {
		vFnMenuSumaFpu(iComandoPos);
	}
	/////////////////////////////////////////////////////////////////////////////
	else if ((iComandoPos = iFnEsCmdShell("tss")) >= 0) {
		vFnMenuTSS(iComandoPos);
	}
	/////////////////////////////////////////////////////////////////////////////
	else if (iFnEsCmdShell("ver") >= 0) {
		vFnMenuVer();
	}
	/////////////////////////////////////////////////////////////////////////////
	else if (iFnEsCmdShell("winm") >= 0) {

		vFnCambiarVisibilidadVentanaProceso();
		vFnImprimirContexto();
	} 
	/////////////////////////////////////////////////////////////////////////////
	else {
		iComandoIncorrecto = 1;
	}
	/////////////////////////////////////////////////////////////////////////////
	if (iAutoCompletarCancelado == 0) {
		//WV: Si el comando ingresado es erroneo y se ingresaron caracteres.
		if ((iComandoIncorrecto == 1) &&
			  (iCantidadCaracteres > 0)) {
			vFnImprimir("\nComando no reconocido");
		}
		iCantidadCaracteres = 0;
		stBufferShell[0] = '\0';
		vFnImprimirPrompt();
	} else {
		iAutoCompletarCancelado = 0;	//reseteo la bandera
	}

}				//Fin vFnShell()


/*****************************************************************************/
/*---------COMIENZO DE LA CODIFICACION DE LOS comandos DEL SHELL ------------*/
/*****************************************************************************/



/**
\fn void vFnImprimirMenu()
\brief Arma la barra inferior donde se comentan los comandos más importantes.
\date 09/04/2006
*/
void vFnImprimirMenu()
{
	if (uiModoMemoria == MODOPAGINADO) {
		vFnImprimirVentana(2,
				   "\n| ayuda | ps  |  init | mem |  pag  | cls  | F2: Log |");
	} else {
		vFnImprimirVentana(2,
				   "\n| ayuda | ps  |  init | mem | segs  | cls  | F2: Log |");
	}
	//WV: Imprime la ayuda en la ventana del log.
	vFnImprimirVentana(HWND_LOG_AYUDA,
			   "\n| ayuda LOG | ^:Arriba | v:Abajo | B:Borrar | F1: Shell |");
}

/**
\fn void vFnMenuAyuda()
\brief Muestra la ayuda en pantalla, para más ayuda escriba ayuda.
\date 09/04/2006
*/
void vFnMenuAyuda()
{
	vFnImprimir("\nSistemas Operativos - UNLaM");
	vFnImprimir("\n-------------------------------------------------------------------------------");
	vFnImprimir("\nCOMANDOS GENERALES");

    vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>cls");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nLimpia la pantalla");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>desc [indice]");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nDireccion donde se encuentra el descriptor indicado");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>dump [desde] [len]");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nRealiza un vuelco de memoria en pantalla. \nIndicar Direccion inicial y cantidad de words");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>exec [opcion] [tam] [prioridad]");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nCrea un proceso. Indicar su tamanio en bytes");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>Execve [nombre_archivo] [parametros] [tam_bss] [prioridad]");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nCrea un proceso a partir de un programa de prueba compilado en \nforma independiente. Indicar tamanio de BSS en bytes");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	SHELL_INTERACTIVO;		//cada quince lineas una pausa
	vFnMenuCls();

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>gdt");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nMuestra el contenido de las primeras 20 posiciones en la GDT");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>idt");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nMuestra el contenido de la IDT");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>kill [pid]");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nRemueve el proceso de la memoria. Indicar Pid");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>mem");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nMuestra la cantidad de Memoria base y total del sistema");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>ps");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nLista los Procesos en memoria y su estado");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>stack [pid] [ring 0-2]");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nMuestra el contenido del stack del proceso y ring de ejecucion especificado.");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>tss [pid]");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nMuestra el contenido de la TSS y PCB del proceso indicado");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	SHELL_INTERACTIVO;
	vFnMenuCls();

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>syskill [pid] [sig]");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir
	    ("\nEnvia una senial a un proceso determinado.Seniales soportadas:\n SIGSTOP, SIGCONT, SIGKILL, SIGTERM, SIGALARM");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>ver");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nVersion del sistema operativo");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>winm");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nOculta o restaura la ventana de procesos de usuario");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>ls");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nMuestra los archivos presentes en el directorio actual");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>reboot");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nReinicia el sistema");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>set [variable]=[valor]");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nle asigna a [variable] el [valor] indicado. Teclee \"set\" para mas ayuda");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	SHELL_INTERACTIVO;
	vFnMenuCls();

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>planif [subcomando] [parametros]");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nPermite la modificacion del algoritmo de planificacion.\nDependiendo del subcomando seleccionado, se obtienen diferentes resultados.\nTipee [planif ayuda] o [planif] para mas informacion");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>lote [subcomando] [parametros]");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nPermite la creacion de conjuntos de procesos que se dispararan\nde forma simultanea.\nDependiendo del subcomando seleccionado, se obtienen diferentes resultados.\nTipee [lote ayuda] o [lote] para mas informacion");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>sumafpu numero1 numero");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nSuma dos numeros flotantes y muestra el resultado por pantalla");

	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nCmd>cambiateclado [NUMERO_DISTRIBUCION | CODIGO_DISTRIBUCION]");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("\nCambia la configuracion del teclado. Si se ejecuta sin parametros muestra las distribuciones disponibles");
	
	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	SHELL_INTERACTIVO;
	vFnMenuCls();

	if (uiModoMemoria == MODOPAGINADO) {
		vFnSysSetearColor(HWND_COMANDO, BLANCO);
		vFnImprimir("\nEn MODO PAGINADO\n");
		vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
		vFnImprimir("\nCmd>pag [pid]");
		vFnSysSetearColor(HWND_COMANDO, BLANCO);
		vFnImprimir("\nMuestra los frames correspondientes a ese proceso");
		vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
		vFnImprimir("\nCmd>bitmap");
		vFnSysSetearColor(HWND_COMANDO, BLANCO);
		vFnImprimir("\nMuestra el mapa de memoria libre y alocada");
		vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
		vFnImprimir("\nCmd>check [pid] [pag] [offs]");
		vFnSysSetearColor(HWND_COMANDO, BLANCO);
		vFnImprimir("\nVerifica si para el proceso dado los valores de pagina y offset caen dentro de los limites de memoria que le fueron asignados. En caso contrario lo mata");
	} else {
		vFnImprimir("\nEn MODO SEGMENTADO\n");
		vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
		vFnImprimir("\nCmd>segs");
		vFnSysSetearColor(HWND_COMANDO, BLANCO);
		vFnImprimir("\nMuestra el nro, posicion inicial y posicion final de los segmentos de memoria ocupados");
	}
}
/**
\fn void vFnMenuVerSemaforos()
\brief Visualiza el estado de los Semaforos en el SODIUM.
\date 23/02/2008
*/
void vFnMenuVerSemaforos()
{
	int i,j;


	vFnImprimir("\n\nSemaforos del Sistema:\n");
	for (i=0;i<CANTMAXSEM;i++)
	{
		vFnImprimir("Valor: %d, Pshared: %d, Inicializado: %d, Cola: ",semaforosEnElSistema[i].valor,semaforosEnElSistema[i].pshared,semaforosEnElSistema[i].inicializado);

		for (j=0;j<CANTMAXPROCCOLA;j++){
			vFnImprimir("[%d]",semaforosEnElSistema[i].cola[j]);
		}

		vFnImprimir("\n");
	}
}

/**
\fn void vFnMenuVerShm()
\brief Visualiza el estado de la memoria compartida simulada por software.
\date 23/02/2008
*/
void vFnMenuVerShm()
{
	int i,j;

	vFnImprimir("\n\nMemorias Compartidas del Sistema:\n");
	for (i=0;i<CANTMAXSHM;i++)
	{
		vFnImprimir("Key: %d, Tamanio: %d, Declarada %d, Pids: ", memoriasCompartidas[i].key, memoriasCompartidas[i].tamanio, memoriasCompartidas[i].declarada);

		for (j=0;j<CANTMAXPROCSHM;j++)
		{
			vFnImprimir("[%d]",memoriasCompartidas[i].procAtt[j].pid);
		}

		vFnImprimir("\n");
	}
}

/**
\fn void vFnMenuKill(int iComandoPos)
\brief Implementación del comando kill. Elimina al proceso solicitado por línea de comandos.
\param iComandoPos Posición inicial del comando en el buffer de cCaracteres. Con esta
 información se podrán recuperar los parámetros pasados desde la línea de comandos.
\date 09/04/2006
*/
void vFnMenuKill(int iComandoPos)
{
    char stArg1[16];
    int iProcPos, iPid;

    if ((iFnGetArg(iComandoPos, 1, stArg1, 16) == 1)) {
        iPid = iFnCtoi(stArg1);

        if ((iProcPos = iFnBuscaPosicionProc(iPid)) != -1) {
            if ((iPid != 0) &&  //No dejamos matar al IDLE
                (iPid != 1) &&  //Ni al SHELL
                (iPid != 2) &&  //Ni al RELOJ TODO: Dejar matar al reloj
                (pstuPCB[iProcPos].iEstado != PROC_ELIMINADO)) {
                    pstuPCB[iProcPos].iEstado = PROC_ELIMINADO;
                    iFnLimpiarMemoria( pstuPCB[iProcPos].uiTamProc,
                                       pstuPCB[iProcPos].pstuTablaPaginacion);
                    vFnImprimir("\n OK, PID=%d, POS=%d, ESTADO=%d",
                                pstuPCB[iProcPos].ulId,
                                iProcPos, pstuPCB[iProcPos].iEstado);
            } else {
                vFnImprimir("\n No se puede matar al proceso %d",iPid);
            }
        } else {
            vFnImprimir("\nEl pid ingresado no corresponde a un "
                        "abonado en servicio...");
        }
    } else {
        vFnImprimir("\nError, argumento(s) incorrecto(s).\nForma de uso: "
                    "Cmd>kill [pid]");
    }
}


/**
\fn void vFnMenuInstanciarInit()
\brief Instancia al proceso Init (lado usuario) por linea de comando.
*/
void vFnMenuInstanciarInit()
{
    unsigned int uiPosicion;
    int iStatus;
    long lRetorno;
	
    uiPosicion = iFnInstanciarInit();

    ulPidProcesoForeground = pstuPCB[uiPosicion].ulId;
    //Esperamos a Init, asi no queda zombie, y ademas, lFnSysWaitPid se encarga
    //de eliminar el segmento del proceso hijo, por lo que no perdemos memoria
    lRetorno = lFnSysWaitPid( ulPidProcesoForeground, &iStatus, 0 );
}


/**
\fn void vFnMenuCambiaTeclado(int iComandoPos)
\brief Cambia la distribucion del teclado usada por el shell
\param iComandoPos Posición inicial del comando en el buffer de cCaracteres.
 Con esta información se podrán recuperar los parámetros pasados desde la
 línea de comandos.
\date 12/07/2008
*/
void vFnMenuCambiaTeclado(int iComandoPos)
{
    char strArg1[10];
    int iParam1;
    int estado;

    if (iFnGetArg(iComandoPos, 1, strArg1, sizeof(strArg1) - 1) == 1 && (iParam1 = iFnCtoi(strArg1)) > 0)
        estado=iFnCambiaTecladoI( iParam1 );
    else
        estado=iFnCambiaTecladoS( strArg1 );

    if (estado == 1)
    {
	vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
	vFnImprimir("\nAyuda del comando cambiateclado:\n");
	vFnSysSetearColor(HWND_COMANDO, BLANCO);
	vFnImprimir("Forma de uso: Cmd>cambiateclado [NUMERO_DISTRIBUCION | CODIGO_DISTRIBUCION]\n"
	            "Las distribuciones existentes son:");
	vFnListarKeymaps();
    }
}

/**
\fn void vFnMenuCls()
\brief Limpia la pantalla, dejando al prompt en la última línea.
\date 09/04/2006
*/
void vFnMenuCls()
{
	vFnClsWin(HWND_COMANDO);
}

/**
\fn void vFnMenuLs(int iComandoPos)
\brief Muestra un listado de los archivos que pertenecen al directorio 
 solicitado por parámetros.
\date 09/04/2006
*/
void vFnMenuLs(int iComandoPos)
{
	char strDirectorio[40];

	if ((iFnGetArg
	     (iComandoPos, 1, strDirectorio,
	      sizeof(strDirectorio)) == 1)) {
		iFnRamFsLs(strDirectorio);
	} else {
		iFnRamFsLs("/mnt/usr");
	}
}


/**
\fn void vFnMenuVer()
\brief Limpia la pantalla y muestra el logo junto con la versión del
 Sistema operativo
\date 02/05/2007
*/
void vFnMenuVer()
{
	vFnMenuCls();
	vFnImprimir("\nVersion: %s ....emmm... Estable???",
		    VERSION_SODIUM);
}


//TODO lala - Actualizar segun las nuevas estrategias de asig. de memoria
/**
\fn void vFnMenuMem()
\brief  Muestra por pantalla información relevante a la memoria del Sistema
\date 01/05/2008
*/
void vFnMenuMem()
{
	unsigned int uiMemoriaUsada = MEM_BASE;	//partimos desde la memoria ya reservada para el so
	unsigned int uiMemoriaUsadaOverhead = MEM_BASE;
	unsigned int uiMemoriaDisponible = 0;
	unsigned int uiMemoriaPerdida;
	unsigned int uiMemoriaProceso;
	int iN = 0;

	// WV:En modo protegido no sedivide la memoria en convecional y alta.
	// vFnListarKMem();

	if (uiModoMemoria == MODOPAGINADO) {
		// Total de memoria usada y perdida.
		while (iN < CANTMAXPROCS) {
			if (pstuPCB[iN].iEstado != PROC_ELIMINADO
			    && pstuPCB[iN].iEstado !=
			    PROC_NO_DEFINIDO && pstuPCB[iN].ulId > 1) {
				uiMemoriaUsada += (pstuPCB[iN].uiTamProc);
				uiMemoriaUsadaOverhead +=
				    pstuPCB[iN].uiTamProc / 4096 * 4096;

				uiMemoriaUsadaOverhead += 4096 * (1 + pstuPCB[iN].uiTamProc / (iLimCantPaginasProceso * 4096));	//cada proceso tiene al menos 1 pag extra de PTABLE.

				if (pstuPCB[iN].uiTamProc % 4096 != 0)
					uiMemoriaUsadaOverhead += 4096;	//Se agrega para contabilizar la Fragmentacion Interna

			}
			iN++;
		}

		uiMemoriaDisponible = iTamanioMemoria * 8 * 4096 - uiMemoriaUsadaOverhead;
		uiMemoriaPerdida    = uiMemoriaUsadaOverhead - uiMemoriaUsada;

		// Memoria para el proceso mas grande.
		if (uiMemoriaDisponible - 4096 * (1 + uiMemoriaDisponible /
			    (iLimCantPaginasProceso * 4096)) >= 8192) 
		{
			uiMemoriaProceso = uiMemoriaDisponible - 4096 * (1 + uiMemoriaDisponible /
							           (iLimCantPaginasProceso * 4096));
			} else {
			uiMemoriaProceso = 0;
		}
		
		//Ya tenemos floats, dividimos para pasar a Kb o Mb

		//En los dos primeros totales: De donde salia * 2^15 ?
		//Y si dividiamos despues por 2^20 directamente multiplicamos por 32

		vFnImprimir("\nSimulando modelo de Memoria Paginada\n");
		vFnImprimir("------------------------------------\n" );
		vFnImprimir("  Memoria Total           = %.2f Mb (%db)\n",
			(float)iTamanioMemoria / 32, iTamanioMemoria<<15 );
		vFnImprimir("  Memoria utilizada       = %.2f Mb (%db)\n",
			(float)uiMemoriaUsada / (1024*1024), uiMemoriaUsada );
		vFnImprimir("  Memoria disponible      = %.2f Mb (%db)\n",
			(float)uiMemoriaDisponible / (1024*1024), uiMemoriaDisponible );
												vFnImprimir("  Memoria perdida por \n"
			    "   fragmentacion Interna  = %.2f Kb (%db)\n",
			    (float)uiMemoriaPerdida / 1024, uiMemoriaPerdida );
		vFnImprimir("  Capacidad maxima de un\n"
			    "   proceso en memoria     = %.2f Mb (%db)\n",
			    (float)uiMemoriaProceso / (1024*1024), uiMemoriaProceso );
	} 
	
	else // SEGMENTACION
	{
		// Total de memoria usada y perdida.
		while (iN < CANTMAXPROCS) {
			if (pstuPCB[iN].iEstado != PROC_ELIMINADO)
				uiMemoriaUsada += (pstuPCB[iN].uiTamProc);
			iN++;
		}
		iN = 0;
		/* TODO lala Arreglar
        while (iN < 30) {
			if (iMatrizMf[iN][0] != -1)	//Si el segmento está ocupado lo suma
				uiMemoriaUsadaOverhead += (iMatrizMf[iN][1] - iMatrizMf[iN][0]);
			else
				uiMemoriaDisponible  += (iMatrizMf[iN][1] - iMatrizMf[iN-1][1] +1);
			iN++;
		}
		iN = 0;
        */

		iTamanioMemoria     = uiMemoriaDisponible + uiMemoriaUsadaOverhead;
		uiMemoriaPerdida    = uiMemoriaUsadaOverhead - uiMemoriaUsada;
		uiMemoriaProceso    = 0; //TODO lala antes decia iFnSegmentoMaximo();

		//Ya tenemos floats, dividimos para pasar a Kb o Mb

		vFnImprimir("\nSimulando modelo de Memoria Segmentada - Celdas de tamanio Fijo\n");
		vFnImprimir("---------------------------------------------------------------\n\n" );
		vFnImprimir("  Memoria Total           = %.2f Mb (%db)\n",
			(float)iTamanioMemoria / (1024*1024), iTamanioMemoria );
		vFnImprimir("  Memoria utilizada       = %.2f Mb (%db)\n",
			(float)uiMemoriaUsada / (1024*1024), uiMemoriaUsada );
		vFnImprimir("  Memoria disponible      = %.2f Mb (%db)\n",
			(float)uiMemoriaDisponible / (1024*1024),uiMemoriaDisponible );
		vFnImprimir("  Memoria perdida por \n"
			    "   fragmentacion Interna  = %.2f Kb (%db)\n",
			(float)uiMemoriaPerdida / 1024, uiMemoriaPerdida );
		vFnImprimir("  Capacidad maxima de un\n"
			    "   proceso en memoria     = %.2f Kb (%db)\n",
			(float)uiMemoriaProceso / 1024, uiMemoriaProceso );
											}
	
	vFnImprimir("-------------------------------------------\n" );

	vFnImprimir(" Memoria Total (BIOS)    = %.2f Mb (%db)\n",
	    (float)uiTamanioMemoriaBios / (1024*1024), uiTamanioMemoriaBios );
	vFnImprimir(" Tamanio del kernel      = %.2f Kb (%db)\n",
	    (float)uiTamanioKernel / 1024, uiTamanioKernel );
	vFnImprimir(" Tamanio del bss         = %.2f Kb (%db)\n",
	    (float)uiTamanioBSS /1024, uiTamanioBSS );
}


/**
\fn void vFnMenuPs()
\brief Muestra por pantalla los procesos activos
\date 09/04/2006
*/
void vFnMenuPs()
{
	int iN;
	for (iN = 0; iN < CANTMAXPROCS; iN++) {
		if (pstuPCB[iN].iEstado != PROC_ELIMINADO
		    && pstuPCB[iN].iEstado != PROC_NO_DEFINIDO) {
			vFnImprimir
			    ("\nPid=%d, PPid=%d, %s, IndiceGDT=%d, Tam=%d bytes, Estado= ",
			     pstuPCB[iN].ulId,
			     pstuPCB[iN].ulParentId,
			     pstuPCB[iN].stNombre,
			     pstuPCB[iN].uiIndiceGDT_TSS,
			     pstuPCB[iN].uiTamProc);

			switch (pstuPCB[iN].iEstado) {
			case PROC_EJECUTANDO:
				vFnImprimir("Ejecutando ");
				break;
			case PROC_LISTO:
				vFnImprimir("Listo      ");
				break;
			case PROC_ESPERANDO:
				vFnImprimir("Esperando  ");
				break;
			case PROC_DETENIDO:
				vFnImprimir("Detenido   ");
				break;
			case PROC_ELIMINADO:
				vFnImprimir("Eliminado  ");
				break;
			case PROC_ZOMBIE:
				vFnImprimir("Zombie     ");
				break;
			default:
				vFnImprimir("%d", pstuPCB[iN].iEstado);
				break;
			}
		}
	}
}

/**
\fn void vFnMenuDesc(int iComandoPos)
\brief Devuelve el contenido de los descriptores para el Pid pasado por paràmetro
 Sino encuentra imprime las 20 primeras posiciones de la GDT
\param iComandoPos Pid pasado por paràmetro.
\date: 18/04/2008
\autor: Gerardo Puyo
*/
void vFnMenuDesc(int iComandoPos)
{
	char stArg1[16],* stPos;
	int iArg1 = 0,iPosProc,iValor,iPos1,iPos2,iN;
//	dword *pdwVolcado; //No se esta utilizando
	if (iFnGetArg(iComandoPos, 1, stArg1, 15) == 1) {
		iArg1 = iFnCtoi(stArg1);
		if (iFnEsNumero(stArg1)==0)
		{
			iPosProc=iFnBuscaPosicionProc(iArg1);
			if (iPosProc!=-1)
			{
		  	vFnImprimir("\n Proceso %d, indice en la PCB %d, indice en la GDT %d",iArg1, iPosProc,pstuPCB[iPosProc].uiIndiceGDT_CS);
		  	vFnImprimir("\n Descriptor de Còdigo");
		  	vFnImprimir("\n Limite bajo: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_CS].usLimiteBajo);
		  	vFnImprimir("\n Limite Alto: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_CS].bitLimiteAlto);
	  	  	vFnImprimir("\n Base Bajo: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_CS].usBaseBajo);
		  	vFnImprimir("\n Base Medio: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_CS].ucBaseMedio);
		  	vFnImprimir("\n Base Alto: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_CS].usBaseAlto);
		  	vFnImprimir("\n Acceso: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_CS].ucAcesso);
		  	vFnImprimir("\n Granularidad: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_CS].bitGranularidad);
		  	//cFnPausa();
			SHELL_INTERACTIVO;
			vFnImprimir("\n Proceso %d, indice en la PCB %d, indice en la GDT %d",iArg1, iPosProc,pstuPCB[iPosProc].uiIndiceGDT_DS);
		  	vFnImprimir("\n Descriptor de Datos");
		  	vFnImprimir("\n Limite bajo: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_DS].usLimiteBajo);
		  	vFnImprimir("\n Limite Alto: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_DS].bitLimiteAlto);
	  	  	vFnImprimir("\n Base Bajo: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_DS].usBaseBajo);
		  	vFnImprimir("\n Base Medio: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_DS].ucBaseMedio);
		  	vFnImprimir("\n Base Alto: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_DS].usBaseAlto);
		  	vFnImprimir("\n Acceso: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_DS].ucAcesso);
		  	vFnImprimir("\n Granularidad: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_DS].bitGranularidad);
		   	//cFnPausa();
			vFnImprimir("\n Proceso %d, indice en la PCB %d, indice en la GDT %d",iArg1, iPosProc,pstuPCB[iPosProc].uiIndiceGDT_TSS);
		  	vFnImprimir("\n Descriptor de TSS");
		  	vFnImprimir("\n Limite bajo: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_TSS].usLimiteBajo);
		  	vFnImprimir("\n Limite Alto: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_TSS].bitLimiteAlto);
	  	  	vFnImprimir("\n Base Bajo: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_TSS].usBaseBajo);
		  	vFnImprimir("\n Base Medio: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_TSS].ucBaseMedio);
		  	vFnImprimir("\n Base Alto: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_TSS].usBaseAlto);
		  	vFnImprimir("\n Acceso: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_TSS].ucAcesso);
		  	vFnImprimir("\n Granularidad: %x ",pstuTablaGdt->stuGdtDescriptorDescs[pstuPCB[iPosProc].uiIndiceGDT_TSS].bitGranularidad);
		   	//cFnPausa();
			SHELL_INTERACTIVO;
			//pstuTablaGdt[pstuPCB[iPosProc].uiIndiceGDT_CS]
		 	}
			else
			vFnMenuGdt();
		}
		else
		{
		iValor=iFnBuscarEnCadena(stArg1,"-",0);
		if(iValor<0) 
			{
			 vFnMenuGdt();
			}
		else
			{
			stPos=pstFnCadenaIzquierda(stArg1,iValor-1);
			if (iFnEsNumero(stPos)==0)
			 {
				iPos1=iFnCtoi(stPos);
				stPos=pstFnCadenaDerecha(stArg1,iFnLongitudCadena(stArg1)-iValor-1);
				if (iFnEsNumero(stPos)<0)
					vFnMenuGdt();
				iPos2=iFnCtoi(stPos);
				if(iPos1<8192 && iPos1>=0 && iPos2>0 && iPos2<=8192)
					{
					  for(iN=iPos1;iN<iPos2;iN++)
					  {
						vFnImprimir("\n Limite bajo: %x ",pstuTablaGdt->stuGdtDescriptorDescs[iN].usLimiteBajo);
						vFnImprimir("\n Limite Alto: %x ",pstuTablaGdt->stuGdtDescriptorDescs[iN].bitLimiteAlto);
	  					vFnImprimir("\n Base Bajo: %x ",pstuTablaGdt->stuGdtDescriptorDescs[iN].usBaseBajo);
						vFnImprimir("\n Base Medio: %x ",pstuTablaGdt->stuGdtDescriptorDescs[iN].ucBaseMedio);
						vFnImprimir("\n Base Alto: %x ",pstuTablaGdt->stuGdtDescriptorDescs[iN].usBaseAlto);
						vFnImprimir("\n Acceso: %x ",pstuTablaGdt->stuGdtDescriptorDescs[iN].ucAcesso);
						vFnImprimir("\n Granularidad: %x \n",pstuTablaGdt->stuGdtDescriptorDescs[iN].bitGranularidad);
						SHELL_INTERACTIVO;
					  }
					}
				else
					vFnMenuGdt();
			 }
			else
				vFnMenuGdt();
			}
		}
	} else {
		/*vFnImprimir
		    ("\nError, argumento(s) incorrecto(s).\nForma de uso: Cmd>desc [indice]");*/
		vFnMenuGdt();
	}
}


/**
\fn void vFnMenuDump(iComandoPos)
\brief Vuelca en pantalla el contenido de las direcciones de memoria.
Desde línea de comandos se ingresan como parámetros la posición inicial 
desde donde queremos leer, ya sea en hexa como en decimal, y la cantidad 
de doublewords que queremos leer. Util para debuguear lo indebugeable.
\param iComandoPos Posición inicial del comando en el buffer de cCaracteres. 
 Con esta información se podrán recuperar los parámetros pasados desde la
 línea de comandos.
\date 09/04/2006
*/
void vFnMenuDump(iComandoPos)
{
	int iArg2 = 0;
	int iN;
	char stArg1[16];
	char stArg2[16];
	dword *pdwVolcado;
	if ((iFnGetArg(iComandoPos, 1, stArg1, 16) == 1)
	    && (iFnGetArg(iComandoPos, 2, stArg2, 16) == 1)) {
		pdwVolcado = (unsigned int *) iFnXtoi(stArg1);
		iArg2 = iFnXtoi(stArg2);
		vFnImprimir
		    ("\nIniciando vuelco de memoria de %d dwords, desde %d",
		     iArg2, pdwVolcado);
		cFnPausa();

		for (iN = 0; iN < iArg2; iN++) {
			vFnImprimir("\ni= %d Dir: %x, cont: %x",
				    iN, &pdwVolcado[iN],
				    (unsigned int) pdwVolcado[iN]);
			if (((iN +
			      1) %
			     (pstuVentana[HWND_COMANDO].iAlto -
			      1)) == 0 && iN > 0)
			{
				SHELL_INTERACTIVO;
			}
		}
		vFnImprimir("\nFin");
	} else {
		vFnImprimir
		    ("\nError, argumento(s) incorrecto(s).\nForma de uso: Cmd>dump [dir mem] [intervalo]");
	}
}


/**
\fn void vFnMenuStack(iComandoPos)
\brief Vuelca en pantalla el contenido del stack del proceso deseado. 
Util para debuguear lo indebugeable. 
Por el momento el parametro RING no tiene uso.
\param iComandoPos Posición inicial del comando en el buffer de Caracteres. 
Con esta información se podrán recuperar los parámetros pasados desde la
 línea de comandos.
\date 09/04/2006
*/
void vFnMenuStack(iComandoPos)
{
	int iPid, iRing, iTSS, iPCB;
	int iN, iEstadoAnterior, iCantidadDWordsStack;
	char stArg1[6];
	char stArg2[16];
	dword *pdwVolcado;

	if ((iFnGetArg(iComandoPos, 1, stArg1, 4) == 1)
	    && (iFnGetArg(iComandoPos, 2, stArg2, 4) == 1)) {
		iPid = iFnXtoi(stArg1);
		iRing = iFnXtoi(stArg2);

		iPCB = iFnBuscaPosicionProc(iPid);
		if (iPCB > 0 && pstuPCB[iPCB].iEstado != PROC_ELIMINADO) {

			iTSS = pstuPCB[iPCB].ulLugarTSS;
			iEstadoAnterior = pstuPCB[iPCB].iEstado;
			vFnImprimir
			    ("\nIntentando congelar al Proceso %d", iPid);
			if (iEstadoAnterior != PROC_EJECUTANDO) {
				pstuPCB[iPCB].iEstado = PROC_DETENIDO;
				vFnImprimirOk(55);

				vFnImprimir("\nesp: %x",
					    stuTSSTablaTareas[iTSS].esp);
				vFnImprimir("\nebp: %x",
					    stuTSSTablaTareas[iTSS].ebp);

				vFnImprimir
				    ("\nIniciando vuelco de stack: TOS=esp=0x%x, Segmento de stack=ss=0x%x, Dir. base=0x%x",
				     stuTSSTablaTareas[iTSS].esp,
				     stuTSSTablaTareas[iTSS].ss,
				     pstuPCB[iPCB].uiDirBase);
				cFnPausa();

				if (pstuPCB[iPCB].uiDirBase) {
					vFnImprimir("\nProceso de usuario (Dir. base != 0x0)");

					switch (iRing) {
					case 3:
						pdwVolcado = (unsigned int *) pstuPCB[iPCB].uiDirBase +
                            stuTSSTablaTareas[iTSS].esp;
                        //TODO - Revisar cambio:
                        //pstuPCB.uiLimite no corresponde a la direccion
                        //absoluta de limite, sino a la LONGITUD del segmento
						//iCantidadDWordsStack = pstuPCB[iPCB].uiLimite -
                        //    stuTSSTablaTareas[iTSS].esp;
						iCantidadDWordsStack = (pstuPCB[iPCB].uiLimite +
                                                pstuPCB[iPCB].uiDirBase  ) -
                                                stuTSSTablaTareas[iTSS].esp;
						vFnImprimir
				    		("\nValores calculados: Dir. inicial volcado=0x%x, "
                             "Cant.Words=%d",pdwVolcado, iCantidadDWordsStack); 
						break;
					default:
						vFnImprimir("\nSeleccion no valida: RING=%d",iRing);
						return;
					}

				} else {
					switch (iRing) {
					case 0:
						pdwVolcado = (unsigned int *)
						    stuTSSTablaTareas
						    [iTSS].espacio0;
						iCantidadDWordsStack =
						    TSS_TAMANIO_STACK_R0 / 4;
						break;
					case 1:
						pdwVolcado = (unsigned int *)
						    stuTSSTablaTareas
						    [iTSS].espacio1;
						iCantidadDWordsStack =
						    TSS_TAMANIO_STACK_R1 / 4;
						break;
					case 2:
						pdwVolcado = (unsigned int *)
						    stuTSSTablaTareas
						    [iTSS].espacio2;
						iCantidadDWordsStack =
						    TSS_TAMANIO_STACK_R2 / 4;
						break;
					default:
						pdwVolcado = (unsigned int *)
						    stuTSSTablaTareas
						    [iTSS].espacio0;
						iCantidadDWordsStack =
						    TSS_TAMANIO_STACK_R0 / 4;
					}

				}
				for (iN = 0;
				     iN < iCantidadDWordsStack; iN++) {
					vFnImprimir
					    ("\nP: %d  Dir: 0x%x\tCont: %x%x%x%x%x%x%x%x \t %c%c%c%c",
					     iN, &pdwVolcado[iN],
					     (unsigned int) (pdwVolcado[iN]
							     >> 28) & 0xF,
					     (unsigned int) (pdwVolcado[iN]
							     >> 24) & 0xF,
					     (unsigned int) (pdwVolcado[iN]
							     >> 20) & 0xF,
					     (unsigned int) (pdwVolcado[iN]
							     >> 16) & 0xF,
					     (unsigned int) (pdwVolcado[iN]
							     >> 12) & 0xF,
					     (unsigned int) (pdwVolcado[iN]
							     >> 8) & 0xF,
					     (unsigned int) (pdwVolcado[iN]
							     >> 4) & 0xF,
					     (unsigned
					      int) (pdwVolcado[iN])
					     & 0xF,
					     (unsigned
					      char) (pdwVolcado[iN]
						     >> 24) & 0xFF,
					     (unsigned
					      char) (pdwVolcado[iN]
						     >> 16) & 0xFF,
					     (unsigned
					      char) (pdwVolcado[iN]
						     >> 8) & 0xFF,
					     (unsigned
					      char) (pdwVolcado
						     [iN]) & 0xFF);

					if (&pdwVolcado[iN] ==
					    (unsigned int *)
					    stuTSSTablaTareas[iTSS].esp)
						vFnImprimir
						    (" <-esp (TOP OF STACK)");

					if (&pdwVolcado[iN] ==
					    (unsigned int *)
					    stuTSSTablaTareas[iTSS].ebp)
						vFnImprimir
						    (" <-ebp (STACK FRAME POINTER)");

					if (((iN +
					      1) %
					     (pstuVentana
					      [HWND_COMANDO].
					      iAlto - 1)) == 0 && iN > 0)
					 {
						SHELL_INTERACTIVO;
					 }
				}

				vFnImprimir
				    ("\nReanudando Proceso %d", iPid);
				pstuPCB[iPCB].iEstado = iEstadoAnterior;
				vFnImprimirOk(55);

				vFnImprimir("\nFin");

			} else {
				vFnImprimirNOk(55);
				vFnImprimir
				    ("\nJejeje... Hablando del principio de incertidumbre...");
			}
		} else {
			vFnImprimir
			    ("\nEl pid indicado no pertenece a un abonado en servicio...");
		}

	} else {
		vFnImprimir
		    ("\nError, argumento(s) incorrecto(s).\nForma de uso: Cmd>stack [pid] [ring]");
	}
}


/**
\fn void vFnMenuGdt()
\brief Muestra por pantalla las primeras 20 posiciones de la GDT
\date 18/04/2008
*/
void vFnMenuGdt()
{
	int iN;
	dword *pdwVolcado;

	vFnImprimir("\nposicion inicial de la gdt: %d ", pstuTablaGdt);
	vFnImprimir("\nComienzo del volcado de memoria:");
	pdwVolcado = (dword *) pdwGDT;

	for (iN = 0; iN < 20; iN++) {

		vFnImprimir("\n Limite bajo: %x ",pstuTablaGdt->stuGdtDescriptorDescs[iN].usLimiteBajo);
		vFnImprimir("\n Limite Alto: %x ",pstuTablaGdt->stuGdtDescriptorDescs[iN].bitLimiteAlto);
	  	vFnImprimir("\n Base Bajo: %x ",pstuTablaGdt->stuGdtDescriptorDescs[iN].usBaseBajo);
		vFnImprimir("\n Base Medio: %x ",pstuTablaGdt->stuGdtDescriptorDescs[iN].ucBaseMedio);
		vFnImprimir("\n Base Alto: %x ",pstuTablaGdt->stuGdtDescriptorDescs[iN].usBaseAlto);
		vFnImprimir("\n Acceso: %x ",pstuTablaGdt->stuGdtDescriptorDescs[iN].ucAcesso);
		vFnImprimir("\n Granularidad: %x \n",pstuTablaGdt->stuGdtDescriptorDescs[iN].bitGranularidad);
		SHELL_INTERACTIVO;
	
	}
}


/**
\fn void vFnMenuIdt()
\brief Muestra por pantalla el contenido de la Idt.
\date 09/04/2006
*/
void vFnMenuIdt()
{
	int iN;
	dword *pdwVolcado;

	vFnImprimir("\nPosicion inicial de la idt: %x ", pstuIDT);
	vFnImprimir("\nComienzo del volcado de memoria:");
	cFnPausa();
	pdwVolcado = (dword *) pstuIDT;
	for (iN = 0; iN < 512; iN++) {
		vFnImprimir("\ni= %d Dir: %x, cont: %x", iN,
			    pdwVolcado, (unsigned int) *pdwVolcado);
		pdwVolcado++;
		if (((iN + 1) % (pstuVentana[HWND_COMANDO].iAlto -
				 1)) == 0 && iN > 0)
		{
			SHELL_INTERACTIVO;
		}
	}
}

/**
\fn void vFnMenuTSS(int iComandoPos)
\brief Muestra por pantalla la TSS del proceso especificado por línea
 de comandos.
\param iComandoPos Posición inicial del comando en el buffer de cCaracteres. 
Con esta información se podrán recuperar los parámetros pasados desde la
 línea de comandos.
\date 09/04/2008
*/
void vFnMenuTSS(int iComandoPos)
{
	int iPid, iPCB, iTSS, iEstadoAnterior=-1;

	char stArg1[16];

	if ((iFnGetArg(iComandoPos, 1, stArg1, 16) == 1)) {
		iPid = iFnCtoi(stArg1);
	} else {
		vFnImprimir
		    ("\nError, argumento(s) incorrecto(s).\nForma de uso: Cmd>tss [pid]");
		return;
	}

	iPCB = iFnBuscaPosicionProc(iPid);
	
	if (iPCB < 0) {
		vFnImprimir ("\nEl pid indicado no pertenece a un abonado en servicio...");
		return;
	}

	iEstadoAnterior = pstuPCB[iPCB].iEstado;
	vFnImprimir ("\nIntentando detener al Proceso %d", iPid);
	if (iEstadoAnterior != PROC_EJECUTANDO) {
		pstuPCB[iPCB].iEstado = PROC_DETENIDO;
		vFnImprimirOk(55);

	} else {
		vFnImprimirNOk(55);
		vFnImprimir ("\nJejeje... Hablando del principio de incertidumbre...");
		return;
	}
  
	vFnImprimir (	"\nMostrando contenido de la TSS:");

	iTSS = pstuPCB[iPCB].ulLugarTSS;
/*
	vFnImprimir(	"\n IndiceTSS = %d, direccion lineal: %x"	\
  			"\n esp0:\t%d\t(0x%x)\tss0:\t%d\t(0x%x)"	\
  			"\n esp1:\t%d\t(0x%x)\tss1:\t%d\t(0x%x)"	\
  			"\n esp2:\t%d\t(0x%x)\tss2:\t%d\t(0x%x)"	\
  			"\n cr3:\t%d\t(0x%x)"				\
  			"\n eip:\t%d\t(0x%x)"				\
  			"\n uiEFlags:\t%d\t(0x%x)"			\
  			"\n eax:\t%d\t(0x%x)\tecx:\t%d\t(0x%x)"		\
  			"\n edx:\t%d\t(0x%x)\tebx:\t%d\t(0x%x)"		\
  			"\n esp:\t%d\t(0x%x)\tebp:\t%d\t(0x%x)"		\
  			"\n esi:\t%d\t(0x%x)\tedi:\t%d\t(0x%x)"		\
  			"\n es:\t%d\t(0x%x)\t  cs:\t%d\t(0x%x)"		\
  			"\n ss:\t%d\t(0x%x)\t  ds:\t%d\t(0x%x)"		\
  			"\n fs:\t%d\t(0x%x)\t  gs:\t%d\t(0x%x)"		\
  			"\n ldt:\t%d\t(0x%x)"				\
  			"\n trapbit:\t%d\t(0x%x)"			\
  			"\n uIOMapBase:\t%d\t(0x%x)",			\
			iTSS,				\
			(unsigned int)&stuTSSTablaTareas[iTSS],	\
			stuTSSTablaTareas[iTSS].esp0,	\
			stuTSSTablaTareas[iTSS].esp0,	\
			stuTSSTablaTareas[iTSS].ss0,	\
			stuTSSTablaTareas[iTSS].ss0,	\
			stuTSSTablaTareas[iTSS].esp1,	\
			stuTSSTablaTareas[iTSS].esp1,	\
			stuTSSTablaTareas[iTSS].ss1,	\
			stuTSSTablaTareas[iTSS].ss1,	\
			stuTSSTablaTareas[iTSS].esp2,	\
			stuTSSTablaTareas[iTSS].esp2,	\
			stuTSSTablaTareas[iTSS].ss2,	\
			stuTSSTablaTareas[iTSS].ss2,	\
			stuTSSTablaTareas[iTSS].cr3,	\
			stuTSSTablaTareas[iTSS].cr3,	\
			stuTSSTablaTareas[iTSS].eip,	\
			stuTSSTablaTareas[iTSS].eip,	\
			stuTSSTablaTareas[iTSS].uiEBandera,	\
			stuTSSTablaTareas[iTSS].uiEBandera,	\
			stuTSSTablaTareas[iTSS].eax,	\
			stuTSSTablaTareas[iTSS].eax,	\
			stuTSSTablaTareas[iTSS].ecx,	\
			stuTSSTablaTareas[iTSS].ecx,	\
			stuTSSTablaTareas[iTSS].edx,	\
			stuTSSTablaTareas[iTSS].edx,	\
			stuTSSTablaTareas[iTSS].ebx,	\
			stuTSSTablaTareas[iTSS].ebx,	\
			stuTSSTablaTareas[iTSS].esp,	\
			stuTSSTablaTareas[iTSS].esp,	\
			stuTSSTablaTareas[iTSS].ebp,	\
			stuTSSTablaTareas[iTSS].ebp,	\
			stuTSSTablaTareas[iTSS].esi,	\
			stuTSSTablaTareas[iTSS].esi,	\
			stuTSSTablaTareas[iTSS].edi,	\
			stuTSSTablaTareas[iTSS].edi,	\
			stuTSSTablaTareas[iTSS].es,	\
			stuTSSTablaTareas[iTSS].es,	\
			stuTSSTablaTareas[iTSS].cs,	\
			stuTSSTablaTareas[iTSS].cs,	\
			stuTSSTablaTareas[iTSS].ss,	\
			stuTSSTablaTareas[iTSS].ss,	\
			stuTSSTablaTareas[iTSS].ds,	\
			stuTSSTablaTareas[iTSS].ds,	\
			stuTSSTablaTareas[iTSS].fs,	\
			stuTSSTablaTareas[iTSS].fs,	\
			stuTSSTablaTareas[iTSS].gs,	\
			stuTSSTablaTareas[iTSS].gs,	\
			stuTSSTablaTareas[iTSS].ldt,	\
			stuTSSTablaTareas[iTSS].ldt,	\
			stuTSSTablaTareas[iTSS].trapbit,	\
			stuTSSTablaTareas[iTSS].trapbit,	\
			stuTSSTablaTareas[iTSS].uIOMapeoBase,	\
			stuTSSTablaTareas[iTSS].uIOMapeoBase);
			*/
	vFnImprimir(	"\n IndiceTSS = %d, direccion lineal: %x",	\
			iTSS,				\
			(unsigned int)&stuTSSTablaTareas[iTSS]);
  	vFnImprimir(	"\n esp0:\t%d\t(0x%x)\tss0:\t%d\t(0x%x)",	\
			stuTSSTablaTareas[iTSS].esp0,	\
			stuTSSTablaTareas[iTSS].esp0,	\
			stuTSSTablaTareas[iTSS].ss0,	\
			stuTSSTablaTareas[iTSS].ss0);
  	vFnImprimir(	"\n esp1:\t%d\t(0x%x)\tss1:\t%d\t(0x%x)",	\
			stuTSSTablaTareas[iTSS].esp1,	\
			stuTSSTablaTareas[iTSS].esp1,	\
			stuTSSTablaTareas[iTSS].ss1,	\
			stuTSSTablaTareas[iTSS].ss1);
	vFnImprimir(	"\n esp2:\t%d\t(0x%x)\tss2:\t%d\t(0x%x)",	\
			stuTSSTablaTareas[iTSS].esp2,	\
			stuTSSTablaTareas[iTSS].esp2,	\
			stuTSSTablaTareas[iTSS].ss2,	\
			stuTSSTablaTareas[iTSS].ss2);
	vFnImprimir(	"\n cr3:\t%d\t(0x%x)",				\
			stuTSSTablaTareas[iTSS].cr3,	\
			stuTSSTablaTareas[iTSS].cr3);
	vFnImprimir(	"\n eip:\t%d\t(0x%x)",				\
			stuTSSTablaTareas[iTSS].eip,	\
			stuTSSTablaTareas[iTSS].eip);
	vFnImprimir(	"\n uiEFlags:\t%d\t(0x%x)",			\
			stuTSSTablaTareas[iTSS].uiEBandera,	\
			stuTSSTablaTareas[iTSS].uiEBandera);
	vFnImprimir(	"\n eax:\t%d\t(0x%x)\tecx:\t%d\t(0x%x)",		\
			stuTSSTablaTareas[iTSS].eax,	\
			stuTSSTablaTareas[iTSS].eax,	\
			stuTSSTablaTareas[iTSS].ecx,	\
			stuTSSTablaTareas[iTSS].ecx);
	vFnImprimir(	"\n edx:\t%d\t(0x%x)\tebx:\t%d\t(0x%x)",		\
			stuTSSTablaTareas[iTSS].edx,	\
			stuTSSTablaTareas[iTSS].edx,	\
			stuTSSTablaTareas[iTSS].ebx,	\
			stuTSSTablaTareas[iTSS].ebx);
	vFnImprimir(	"\n esp:\t%d\t(0x%x)\tebp:\t%d\t(0x%x)",		\
			stuTSSTablaTareas[iTSS].esp,	\
			stuTSSTablaTareas[iTSS].esp,	\
			stuTSSTablaTareas[iTSS].ebp,	\
			stuTSSTablaTareas[iTSS].ebp);
	vFnImprimir(	"\n esi:\t%d\t(0x%x)\tedi:\t%d\t(0x%x)",		\
			stuTSSTablaTareas[iTSS].esi,	\
			stuTSSTablaTareas[iTSS].esi,	\
			stuTSSTablaTareas[iTSS].edi,	\
			stuTSSTablaTareas[iTSS].edi);
	vFnImprimir(	"\n es:\t%d\t(0x%x)\t  cs:\t%d\t(0x%x)",		\
			stuTSSTablaTareas[iTSS].es,	\
			stuTSSTablaTareas[iTSS].es,	\
			stuTSSTablaTareas[iTSS].cs,	\
			stuTSSTablaTareas[iTSS].cs);
	vFnImprimir(	"\n ss:\t%d\t(0x%x)\t  ds:\t%d\t(0x%x)",		\
			stuTSSTablaTareas[iTSS].ss,	\
			stuTSSTablaTareas[iTSS].ss,	\
			stuTSSTablaTareas[iTSS].ds,	\
			stuTSSTablaTareas[iTSS].ds);
	vFnImprimir(	"\n fs:\t%d\t(0x%x)\t  gs:\t%d\t(0x%x)",		\
			stuTSSTablaTareas[iTSS].fs,	\
			stuTSSTablaTareas[iTSS].fs,	\
			stuTSSTablaTareas[iTSS].gs,	\
			stuTSSTablaTareas[iTSS].gs);
	vFnImprimir(	"\n ldt:\t%d\t(0x%x)",				\
			stuTSSTablaTareas[iTSS].ldt,	\
			stuTSSTablaTareas[iTSS].ldt);
	vFnImprimir(	"\n trapbit:\t%d\t(0x%x)",			\
			stuTSSTablaTareas[iTSS].trapbit,	\
			stuTSSTablaTareas[iTSS].trapbit);
	vFnImprimir(	"\n uIOMapBase:\t%d\t(0x%x)",			\
			stuTSSTablaTareas[iTSS].uIOMapeoBase,	\
			stuTSSTablaTareas[iTSS].uIOMapeoBase);

	vFnImprimir ("\nReanudando Proceso %d", iPid);
	pstuPCB[iPCB].iEstado = iEstadoAnterior;
	vFnImprimirOk(55);
}


/**
\fn void vFnMenuBitmap()
\brief Muestra por pantalla el contenido del bitmap de memoria paginada
\date 09/04/2006
*/
void vFnMenuBitmap()
{
	vFnImprimirMapaBits();
}


/**
\fn void vFnMenuSegs()
\brief  Muestra el nro, la posicion inicial y final en bytes de los
segmentos ocupados
\date 09/04/2006
*/
void vFnMenuSegs()
{
/* CHAU	vFnMostrarMemoriaSegmentada();*/
    //TODO lala - Mostrar los segmentos ocupados, no solo los bloques libres
    vFnListarBloquesLibres();
}


/**
\fn void vFnMenuPag(int iComandoPos)
\brief Muestra las páginas asignadas al proceso solicitado por línea de
 comando, si no se pasan parámetros se listan la totalidad de las páginas.
\param iComandoPos Posición inicial del comando en el buffer de cCaracteres. 
Con esta información se podrán recuperar los parámetros pasados desde la
línea de comandos.
\date 09/04/2006
*/
void vFnMenuPag(int iComandoPos)
{
	int iPid, iProcPos;
	char stArg1[16];

	if ((iFnGetArg(iComandoPos, 1, stArg1, 16) == 1)) {
		iPid = iFnCtoi(stArg1);
		vFnImprimir("\nPAGINAS POR PROCESOS:");
		vFnImprimir
		    ("El proceso %d posee los siguientes frames:\n", iPid);
		if ((iProcPos = iFnBuscaPosicionProc(iPid)) != -1)
			if (pstuPCB[iProcPos].iEstado != PROC_ELIMINADO)
				vFnImprimirTablaPaginas(pstuPCB
							[iProcPos].
							uiTamProc,
							pstuPCB
							[iProcPos].
							pstuTablaPaginacion);
			else
				vFnImprimir("El proceso no existe");
		else
			vFnImprimir("El proceso no existe");
	} else {
		vFnImprimir("Debe especificar un numero de proceso (PID)");
	}
}


/**
\brief Lanza un proceso de usuario a partir de un archivo ejecutable
\param iComandoPos Posición inicial del comando en el buffer de cCaracteres. Con esta información se podrán recuperar los parámetros pasados desde la línea de comandos.
\date 21/10/2008
*/
void vFnMenuEjecutar(int iComandoPos) {
	char stArg1[12];
    char * pstNombreArchivo;
    int iPosicion;
    int iStatus;
    long lRetorno;

	if ((iFnGetArg(iComandoPos, 1, stArg1, 11) == 1)){
        //Pasamos el nombre a mayusculas (cambiar si Sodium es case-sensitive)
        vFnStrUpr(stArg1);
        pstNombreArchivo = pstFnConcatenarCadena(stArg1,".BIN");

        //Creamos el proceso
        iPosicion = iFnCrearProceso( pstNombreArchivo );
        ulPidProcesoForeground = pstuPCB[iPosicion].ulId;

        if(iPosicion < 0) {
            vFnImprimir("\nNo se pudo crear un proceso a partir de %s",
                    pstNombreArchivo);
            return;
        }

        //Y lo esperamos para que no quede zombie
        lRetorno = lFnSysWaitPid( ulPidProcesoForeground, &iStatus, 0 );
    } else {
		vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
        vFnImprimir("\nAyuda del comando ejecutar:\n");
		vFnSysSetearColor(HWND_COMANDO, BLANCO);
        vFnImprimir("Forma de uso: Cmd>ejecutar ARCHIVO");
        vFnImprimir("\nDonde ARCHIVO es el nombre del archivo a ejecutar "
                    "omitiendo la extension .BIN");
    }
}


/**
\fn void vFnMenuExec(int iComandoPos)
\brief Lanza un proceso de sistema o de usuario y le reserva la cantidad de paginas necesarias según el tamaño declarado por línea de comando. Si no hay espacio suficiente o no se pueden alocar más procesos lo indicará con un mensaje de error.
\param iComandoPos Posición inicial del comando en el buffer de cCaracteres. Con esta información se podrán recuperar los parámetros pasados desde la línea de comandos.
\date 09/04/2006
*/
void vFnMenuExec(int iComandoPos)
{				//1
	char stArg1[16];
	char stArg2[16];
	int iArg1 = 0;
	int iArg2 = 0;
	struct stuTablaPagina *pstuTablaPagina = 0;
	int indiceEnPcbs = -1;

	if ((iFnGetArg(iComandoPos, 1, stArg1, 16) == 1)
	    && (iFnGetArg(iComandoPos, 2, stArg2, 16) == 1)) {	//2
		iArg1 = iFnCtoi(stArg1);
		iArg2 = iFnXtoi(stArg2);	//tamanio en bytes del proceso.

		if ((iArg1 > 0) && (iArg1 < 3)) {
			if ((iArg2 > 0)
			    && (iArg2 < iTamanioMemoria * 8 * 4096)) {
				vFnImprimir
				    ("\n Asignando memoria para un proceso de %d Bytes.",
				     iArg2);
				pstuTablaPagina =
				    pstuTablaPaginaFnAsignarPaginas(iArg2);
				if (pstuTablaPagina != 0)	//si habia memoria suficiente...
				{
					vFnImprimir
					    ("\nIniciando Proceso ... ");
					switch (iArg1) {
					case 1:
						indiceEnPcbs =
						    iFnNuevaTareaEspecial
						    (iFnSistema,
						     "PR_SISTEMA............");
						break;
					case 2:
						indiceEnPcbs =
						    iFnNuevaTareaEspecial
						    (iFnProceso1,
						     "PR_USUARIO............");
						break;
					}

					if (indiceEnPcbs != -1) {
						pstuPCB
						    [indiceEnPcbs].
						    uiTamProc = iArg2;
						pstuPCB
						    [indiceEnPcbs].
						    iPrioridad = iArg1;
						pstuPCB
						    [indiceEnPcbs].
						    pstuTablaPaginacion
						    = pstuTablaPagina;
						vFnImprimir("[OK]");
					} else {
						vFnImprimir
						    ("\nEl sistema no acepta mas tareas, Limite = %d",
						     CANTMAXPROCS);
						iFnLimpiarMemoria
						    (iArg2,
						     pstuTablaPagina);
						vFnImprimir
						    ("\nSe ha liberado la memoria reservada previamente");
					}
				} else {
					vFnImprimir
					    ("\nMemoria insuficiente para alocar el proceso");
				}
			} else {
				vFnImprimir
				    ("\nTamanio especificado no valido. Maximo = %d",
				     iTamanioMemoria * 8 * 4096 - 1);
			}
		} else {
			vFnImprimir
			    ("\n Por favor ingrese 1 para lanzar un proceso de sistema o 2 para un proceso usuario");
		}
	} else {
		vFnImprimir
		    ("\nError, argumento(s) incorrecto(s).\nForma de uso: Cmd>exec [1 o 2] [tamanio en bytes del proceso]");
	}
}


/**
\fn void vFnMenuCheck(int iComandoPos)
\brief Comprueba que el número de página y offset sea válido para el
 proceso indicado. Se ingresan dichos parámetros por línea de comandos.
\param iComandoPos Posición inicial del comando en el buffer de cCaracteres. 
Con esta información se podrán recuperar los parámetros pasados desde la
 línea de comandos.
\date09/04/2006
*/
void vFnMenuCheck(int iComandoPos)
{
	char stArg1[16];
	char stArg2[16];
	char stArg3[16];
	int iArg1 = 0, iArg2 = 0, iArg3 = 0, iProcPos, iValorRetorno;
	if ((iFnGetArg(iComandoPos, 1, stArg1, 16) == 1)
	    && (iFnGetArg(iComandoPos, 2, stArg2, 16) == 1)
	    && (iFnGetArg(iComandoPos, 3, stArg3, 16) == 1)) {
		iArg1 = iFnCtoi(stArg1);
		iArg2 = iFnXtoi(stArg2);
		iArg3 = iFnXtoi(stArg3);
		if ((iProcPos = iFnBuscaPosicionProc(iArg1)) != -1) {
			if (iArg1 == 1) {
				vFnImprimir
				    ("\nEhh, no querrias quedarte sin shell...");
			} else {
				iValorRetorno =
				    iFnDarDireccion(pstuPCB
						    [iProcPos].
						    pstuTablaPaginacion,
						    pstuPCB
						    [iProcPos].
						    uiTamProc,
						    iArg2, iArg3);
				switch (iValorRetorno) {
				case -2:
					vFnImprimir
					    ("\nPagina Invalida, Pagina: %d",
					     iArg2);
					iFnLimpiarMemoria(pstuPCB
							  [iProcPos].
							  uiTamProc,
							  pstuPCB
							  [iProcPos].
							  pstuTablaPaginacion);
					pstuPCB[iProcPos].iEstado =
					    PROC_ELIMINADO;
					break;
				case -3:
					vFnImprimir
					    ("\nOffset Invalida, Offset: %d",
					     iArg3);
					iFnLimpiarMemoria(pstuPCB
							  [iProcPos].
							  uiTamProc,
							  pstuPCB
							  [iProcPos].
							  pstuTablaPaginacion);
					pstuPCB[iProcPos].iEstado =
					    PROC_ELIMINADO;
					break;
				default:
					vFnImprimir
					    ("\n Direccion Fisica:%x",
					     iValorRetorno);
				}
			}
		} else
			vFnImprimir("\nEl proceso no existe");
	}
}


/**
\fn void vFnMenuExecSeg(int iComandoPos)
\brief Lanza un proceso de sistema o de usuario y le reserva la cantidad
 necesaria segun el tamano declarado por linea de comando. Si no hay espacio
 suficiente o no se pueden alojar mas procesos lo indicara con un mensaje de
 error.
\param iComandoPos Posición inicial del comando en el buffer de cCaracteres. Con
 esta información se podrán recuperar los parámetros pasados desde la linea de
 comandos.
\date 29/09/2008
*/
void vFnMenuExecSeg(int iComandoPos) {
    char stArg1[16];
    char stArg2[16];
    int iArg1;
    int iArg2;
    int indiceEnPcbs;
    char * pcProceso;

    if ((iFnGetArg(iComandoPos, 1, stArg1, 16) == 1) && 
        (iFnGetArg(iComandoPos, 2, stArg2, 16) == 1)) {

		iArg1 = iFnCtoi(stArg1);    //Tipo de proceso (SISTEMA/USUARIO)
		iArg2 = iFnCtoi(stArg2);	//Tamanio en bytes del proceso

        if ( (iArg1 < 1) || (iArg1 > 2) ) {
            vFnImprimir("\n Parametro invalido: Por favor ingrese: "
                        "1 para lanzar un proceso de sistema o "
                        "2 para un proceso usuario");
        } else if ((iArg2 <= 0)/* || (iArg2 > iFnSegmentoMaximo())*/) {
            //TODO lala Cambiar limites ( Desde 1byte, Hasta MemDisponible )
            //Tamanio invalido
            vFnImprimir("\n Tamanio de proceso invalido. "
                        "Ingrese un valor superior.");
        } else {
            //Parametros correctos; se intenta crear el segmento
            vFnImprimir("\n Creando un segmento de memoria para un proceso de "
                        "%d bytes.", iArg2);
            //TODO - Sacar el pvFnKMalloc (pertenece al kernel)
            pcProceso = (char *) pvFnKMalloc( iArg2, MEM_DEFAULT );

            if (pcProceso == NULL) {
                //No hay memoria para el segmento
                vFnImprimir("\n No se pudo alojar el proceso. "
                            "Tamanio solicitado: %d", iArg2);
            } else {
                //Se creo el segmento, ahora solo falta crear el proceso
                vFnImprimir("\n Intentando iniciar Proceso...");
                switch (iArg1) {
                    case 1:     //Proceso de sistema
                        indiceEnPcbs=iFnNuevaTareaEspecial(
                                                   iFnSistema,"PR_SISTEMA...");
                        break;
                    case 2:     //Proceso de usuario
                        indiceEnPcbs=iFnNuevaTareaEspecial(
                                                   iFnProceso1,"PR_USUARIO...");
                        break;
                }

                if (indiceEnPcbs >= 0) {
                    //Se pudo crear el proceso
                    /* Asignamos Tamanio, Prioridad y Direccion Base en el PCB
                     * del proceso creado.
                     * La direccion base solo la usara el comando kill para
                     * saber que bloque de memoria liberar con vFnKFree, ya que
                     * los procesos creados con iFnNuevaTareaEspecial usan como
                     * Stack parte de su TSS (espacio0, espacio1, etc).
                     */
                    /* Se asigna manualmente uiTamProc, ya que por ser
                     * TAREA_ESPECIAL, iFnCrearPCB lo calcula 'mal' (toma el
                     * valor del tamano del segmento, el cual es 4Gb)
                     */
                    pstuPCB[indiceEnPcbs].uiTamProc = iArg2;
                    pstuPCB[indiceEnPcbs].iPrioridad = iArg1;
                    pstuPCB[indiceEnPcbs].uiDirBase = (unsigned int) pcProceso;

                    vFnImprimir("[OK]");
                } else if (indiceEnPcbs ==-1) {
                    //No se pudo crear el proceso, limite de tareas alcanzado
                    vFnImprimir("\n El sistema no acepta mas tareas, "
                                "Limite = %d", CANTMAXPROCS);

                    //Se borra el segmento recien creado
                    vFnKFree(pcProceso);
                    vFnImprimir("\n Se ha liberado el segmento reservado");
                }
            }
        }	
    } else {
        vFnImprimir("\nError, argumento(s) incorrecto(s).\nForma de uso: "
                    "Cmd>exec [1 o 2] [tamanio en bytes del proceso]");
    }
}


/**
\fn void vFnMenuKillSeg(int iComandoPos)
\brief Comando utilizado matar procesos del sistema.
\param iComandoPos Posición inicial del comando en el buffer de cCaracteres.
 Con esta información se podrán recuperar los parámetros pasados desde la
 línea de comandos.
*/
void vFnMenuKillSeg(int iComandoPos)
{
	char stArg1[16];
	int iProcPos, iPid;
	if ((iFnGetArg(iComandoPos, 1, stArg1, 16) == 1)) {
		iPid = iFnCtoi(stArg1);
		if ((iProcPos = iFnBuscaPosicionProc(iPid)) != -1) {
            if ((iPid != 0) &&  //No dejamos matar al IDLE
                (iPid != 1) &&  //Ni al SHELL
                (iPid != 2) &&  //Ni al RELOJ
                (pstuPCB[iProcPos].iEstado != PROC_ELIMINADO)) {
                    //Liberamos el segmento que usa el proceso
                    /* En realidad, lo unico que se esta eliminando aqui es el
                     * bloque de memoria que se creo con el comando exec, ya
                     * que los procesos creados con iFnNuevaTareaEspecial usan
                     * como Stack parte de su TSS (espacio0, espacio1, etc).
                     */
                    vFnKFree( (void*)pstuPCB[iProcPos].uiDirBase );
                    pstuPCB[iProcPos].iEstado = PROC_ELIMINADO;
                    vFnImprimir("\n OK, PID=%d, POS=%d, ESTADO=%d",
                                 pstuPCB[iProcPos].ulId, iProcPos,
                                 pstuPCB[iProcPos].iEstado);
			} else {
				vFnImprimir("\nNo se puede matar al proceso %d", iPid);
			}
		} else {
			vFnImprimir("\nEl pid ingresado no corresponde a un "
                        "abonado en servicio...");
		}
	} else {
		vFnImprimir("\nError, argumento(s) incorrecto(s)."
                    "\nForma de uso: Cmd>kill [pid]");
	}
}


/**
\fn void vFnMenuSet(int iComandoPos)
\brief Setea y elimina variables de entorno.Para eliminar solamente se
 debe hacer variable="eliminate", y el comando la elimina
\param iComandoPos un parametro que es la variable. Si esta solo, esta se 
muestra si recibe mas de un parametro, prueba que no sea el segundo igual a 
la cadena "eliminate". Si se cumple, le asigna el valor del parametro dos a
la variable del parametro 1. Sino la elimina.
\date 23/04/2007
*/
void vFnMenuSet(int iComandoPos)
{
	char strPar[ENVVARSIZE + 1];
	char strVariable[ENVVARSIZE + 1];
	char strValor[ENVVARSIZE + 1];
	int i = 0;
	if (iFnGetArg(iComandoPos, 1, strPar, sizeof(strPar) - 1)
	    == 1) {
		while (strPar[i] != '=') {
			//hasta que encuentre el = en el buffer del parametro
			strVariable[i] = strPar[i];	//MC: acá cambié la asignación para evitar ambiguedades. ver diff.
			i++;
			if (strPar[i] == '\0')
				//si encuentra un fin de linea, antes del igual, corta
				break;
		}
		strVariable[i] = '\0';
		if (strPar[i] == ' ' || strPar[0] == '='
		    || (strPar[(++i) - 1] == '=' && strPar[i] == '\0')) {
			//es decir: tiene formato [planif set variable=]
			vFnImprimir
			    ("\nParametros incorrectos, tipee ayuda para mas informacion");
		} else {
			if (iFnCompararCadenas
			    (strVariable, "showall") == 1) {
				vFnGetAllEnv();
			} else {
				if (strPar[i - 1] == '\0') {
					char strValorVariable
					    [ENVVARSIZE + 1];
					if (cpFnGetEnv(strVariable)
					    != NULL) {
						vFnImprimirString
						    (strValorVariable,
						     cpFnGetEnv
						     (strVariable));
						//en este punto uno puede usar el valor traido como quiera
						//yo te lo muestro                                              
						vFnImprimir
						    ("\nValor: %s",
						     strValorVariable);
					} else {
						vFnImprimir
						    ("\nEsa variable no existe");
					}
				} else {

					int j = i;
					while (strPar[i] != '\0') {
						strValor[i - j] =
						    strPar[i];
						i++;	//MC: Modificado para evitar ambiguedades
					}
					strValor[i - j] = '\0';

					if (iFnCompararCadenas
					    (strValor, "eliminate") == 1) {
						vFnUnSetEnv(strVariable);
						vFnImprimir
						    ("\nVariable eliminada. Puede comprobarlo tipeando ");
						vFnSysSetearColor
						    (HWND_COMANDO,
						     BLANCO_BRILLANTE);
						vFnImprimir
						    ("\"set %s\"",
						     strVariable);
						vFnSysSetearColor
						    (HWND_COMANDO, BLANCO);
					} else {
						vFnSetEnv
						    (strVariable,
						     strValor);
						vFnImprimir
						    ("\nVariable seteada, puede consultarla tipeando ");
						vFnSysSetearColor
						    (HWND_COMANDO,
						     BLANCO_BRILLANTE);
						vFnImprimir
						    ("\"set %s\"",
						     strVariable);
						vFnSysSetearColor
						    (HWND_COMANDO, BLANCO);
					}
				}
			}
		}

	} else {
		vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
		vFnImprimir("\nAyuda del comando set:");
		vFnSysSetearColor(HWND_COMANDO, BLANCO);
		vFnImprimir
		    ("\nSi [valor]==\"eliminate\" se elimina la variable. "
		     "\nSi no ingresa un valor se muestra [variable] por pantalla\n"
		     "si [valor]==\"showall\" se muestran todas las variables cargadas");
	}
}

/**
\fn void vFnMenuSumaFpu(int iComandoPos)
\brief Realiza la suma de dos numeros en formato de punto flotante usando la FPU
\param iComandoPos Posición inicial del comando en el buffer de cCaracteres.
 Con esta información se podrán recuperar los parámetros pasados desde la
 línea de comandos.
\date 10/06/2008
*/
void vFnMenuSumaFpu(int iComandoPos)
{
	char strArg1[512];
	char strArg2[512];
    double fArg1;
    double fArg2;

	if (iFnGetArg(iComandoPos, 1, strArg1, sizeof(strArg1) - 1) == 1 &&
	    iFnGetArg(iComandoPos, 2, strArg2, sizeof(strArg1) - 1) == 1) {
        fArg1 = fFnAtof(strArg1);
        fArg2 = fFnAtof(strArg2);
        vFnImprimir("\n%f + %f = %f", fArg1, fArg2, fArg1+fArg2);
	} else {
		vFnSysSetearColor(HWND_COMANDO, BLANCO_BRILLANTE);
        vFnImprimir("\nAyuda del comando sumafpu:\n");
		vFnSysSetearColor(HWND_COMANDO, BLANCO);
        vFnImprimir("Forma de uso: Cmd>sumafpu NUMERO1 NUMERO2");
	}
}

/**
\fn void vFnInicializarEnv()
\brief carga el vector de variables de eentorno con variables no inicializadas
\date 23/09/2007
*/
void vFnInicializarEnv()
{
	int i;
	for (i = 0; i < VARIABLESENV; i++) {
		pstuENV[i].iActiva = -1;
	}
}

/**
\fn void vFnSetEnv(char *cpVariable, char *cpValor)
\brief  Setea variables de entorno
\param cpVariable Es el nombre de la variable a setear en el shell.
\param cpValor Es el valor de la variable.
\date 23/09/2007
*/
void vFnSetEnv(char *cpVariable, char *cpValor)
{
	int iEncontroLugar = -1;
	int i = 0;

	if (cpFnGetEnv(cpVariable) != NULL) {
		//reemplaza una variable existente (si la encuentra siempre ejecuta)
		while (i < VARIABLESENV) {
			if (iFnCompararCadenas
			    (pstuENV[i].cpVariable, cpVariable) == 1) {
				iEncontroLugar = 1;
				vFnImprimirString(pstuENV[i].
						  cpValor, "%s", cpValor);
				break;
			}
			i++;
		}
	} else {
		//crea una nueva
		while (i < VARIABLESENV) {
			if (pstuENV[i].iActiva == -1) {
				iEncontroLugar = 1;
				pstuENV[i].iActiva = 1;
				vFnImprimirString(pstuENV[i].
						  cpVariable, "%s",
						  cpVariable);
				vFnImprimirString(pstuENV[i].
						  cpValor, "%s", cpValor);
				break;
			}
			i++;
		}
	}
	if (iEncontroLugar == -1) {
		vFnImprimir
		    ("\nEspacio de direccionamiento de variables de entorno Lleno!!");
	}
}

/**
\fn char *cpFnGetEnv(char *cpVariable)
\brief  Obtiene variables de entorno particulares
\param cpVariable es el nombre de la variable.
\return El valor de la variable
\date 23/09/2007
*/
char *cpFnGetEnv(char *cpVariable)
{

	int iEncontrada = -1;
	int i = 0;

	while (i < VARIABLESENV) {
		if (iFnCompararCadenas
		    (pstuENV[i].cpVariable, cpVariable) == 1) {
			//si es la buscada
			if (pstuENV[i].iActiva != -1) {
				//si esta activa
				iEncontrada = 1;
				break;
			}
		}
		i++;
	}
	if (iEncontrada == -1) {
		return NULL;
	}
	return pstuENV[i].cpValor;
}

/**
\fn void vFnUnSetEnv(char *cpVariable)
\brief Elimina variables de entorno
\param cpVariable es el nombre de la variable. 
\date 23/09/2007
*/
void vFnUnSetEnv(char *cpVariable)
{

	int iEncontrada = -1;
	int i = 0;

	while (i < VARIABLESENV) {
		if (iFnCompararCadenas
		    (pstuENV[i].cpVariable, cpVariable) == 1) {
			//si es la buscada
			if (pstuENV[i].iActiva == 1) {
				//si esta activa
				iEncontrada = 1;
				break;
			}
		}
		i++;
	}
	if (iEncontrada == -1) {
		vFnImprimir("\nLa variable ingresada no existe");
	}
	pstuENV[i].iActiva = -1;
}

/**
void vFnGetAllEnv()
\brief Muestra todas las variables por pantalla
\date 23/09/2007
*/
void vFnGetAllEnv()
{

	int i = 0;
	vFnImprimir("\nVariable : Valor");
	vFnImprimir("\n----------------");
	while (i < VARIABLESENV) {
		if (pstuENV[i].iActiva != -1) {
			//si esta activa
			vFnImprimir("\n %s : %s ",
				    pstuENV[i].cpVariable,
				    pstuENV[i].cpValor);
		}
		i++;
	}
}

/**
\fn void vFnTermLog(unsigned char ucCaracter)
\brief procesa el caracter ingresado al teclado para realizar algo en la "terminal" log..
\param  ucCaracter Caracter leido desde el teclado.
\date 05/06/2006
*/
void vFnTermLog(unsigned char ucCaracter)
{
	switch (ucCaracter) {
	case TECLA_ARR:
	case '8':
		vFnLogScrollArriba();
		break;
	case TECLA_ABA:
	case '2':
		vFnLogScrollAbajo();
		break;
	case '9':
		vFnLogScrollMaximo();
		break;
	case '3':
	case TECLA_ENTER:
		vFnLogScrollMinimo();
		break;
	case 'b':
	case 'B':
		vFnLogBorrar();
		break;
	}
}
