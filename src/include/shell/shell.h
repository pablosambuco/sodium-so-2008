/**
	\file shell/shell.h
	\brief Biblioteca de funciones del shell
*/
#ifndef _SHELL_H_
#define _SHELL_H_

#define VARIABLESENV 10 /*!< cantidad de variables de entorno */
#define ENVVARSIZE 32 /*!< tamanio de la variable de entorno */

void vFnManejadorTecladoShell(unsigned char  scanCode);

char cFnGetch();
char cFnPausa();
int iFnGetArg(int iPos, int iNumero, char* stResultado, int iAnchoMaximo);
int iFnXtoI (char *c);
int iFnHtoI (char *c);

void vFnLoopPrincipalShell();
void vFnLoopReloj();
void vFnImprimirMenu ();
void vFnImprimirContexto();
void vFnMenuInstanciarInit();

void vFnImprimirMenu ();
void vFnImprimirSeparadores ();
/**
	\defgroup shellCMD Funciones del shell
*/
/*@{*/
void vFnShell ();  
void vFnMenuAyuda ();
void vFnMenuKill (int);
void vFnMenuKillSeg (int);
void vFnMenuLs ();
void vFnMenuMem ();
void vFnMenuVer ();
void vFnMenuDesc (int);
void vFnMenuDump (int);
void vFnMenuStack (int);
void vFnMenuGdt ();
void vFnMenuIdt ();
void vFnMenuCambiaTeclado (int);
void vFnMenuCls ();
void vFnMenuTSS (int);
void vFnMenuBitmap ();
void vFnMenuSegs ();
void vFnMenuPag (int);
void vFnMenuPs ();
void vFnMenuExec (int);
void vFnMenuExecSeg (int);
void vFnMenuCheck (int);
void vFnMenuSet (int);
void vFnSumaFpu (int);
void vFnMenuVerSemaforos();
void vFnMenuVerShm();
void vFnMenuTestptrace(int);


void vFnTermLog (unsigned char ucCaracter);

char *cpFnGetEnv(char *cpVariable);
void vFnGetAllEnv(); 
void vFnSetEnv(char *cpVariable, char *cpValor); 
void vFnUnSetEnv(char *cpVariable);
/*@}*/

/** \brief Almacena las variables de entorno */
typedef struct {
	char cpVariable[ENVVARSIZE+1]; 
	char cpValor[ENVVARSIZE+1];
	int	iActiva;
}stuENV;

/**
	\ingroup shellCMD
*/
void vFnInicializarEnv();

stuENV pstuENV[VARIABLESENV]; /*!< vector de variables de entorno */

#endif
