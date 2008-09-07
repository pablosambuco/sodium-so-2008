#include <shell/shell.h>
#include <shell/cmd_planif.h>
#include <shell/teclado.h>
#include <shell/sys_video.h>
#include <kernel/libk/libk.h>
#include <kernel/libk/string.h>

#include <kernel/system.h>
#include <kernel/sched.h>

void vFnMenuPlanif(int iComandoPos)
{
	char strSubCMD[16];

  	if ((iFnGetArg (iComandoPos, 1, strSubCMD, sizeof(strSubCMD)) == 1)){
		if(iFnCompararCadenas(strSubCMD,"aplicar")==1){
			vFnSubCmdPlanifAplicar();
		} else if(iFnCompararCadenas(strSubCMD,"cambiar")==1){
			vFnSubCmdPlanifCambiar(iComandoPos);
		} else if(iFnCompararCadenas(strSubCMD,"defecto")==1){
			vFnSubCmdPlanifDefecto();
		} else if(iFnCompararCadenas(strSubCMD,"mostrar")==1){
			vFnSubCmdPlanifMostrar();
		} else if(iFnCompararCadenas(strSubCMD,"set")==1){
			vFnSubCmdPlanifSet(iComandoPos);
		} else {
			vFnImprimir("\n Uso incorrecto, tipee [planif] para ver las opciones disponibles");
		}
	} else {
	    vFnImprimir( "\n uso: planif [subcomando] [parametros]"
			 "\n subcomandos: "
		      	 "\n  aplicar"
		         "\n  cambiar [nombre_algoritmo] (Cambia el planificador actual por el que se especificado)"
		         "\n  defecto (Cambia el planificador actual por el planificador por defecto: RR)"
		         "\n  mostrar (Muestra el planificador que esta corriendo en este momento)"
				 "\n  set [parametro]=[valor]"
		         );
	}
}


/******************************************************************************
Funciónes: Subcomandos del planificador
Descripción:  cambian de algoritmos de planificacion, muestra los valores 
			 para el algoritmo elegido, establece parametros por defecto

Recibe:   posicion de comienzo del comando en el buffer del shell
Devuelve: nada

Fecha última modificación: 09/09/2007
******************************************************************************/

void vFnSubCmdPlanifAplicar(){
	vFnImprimir("\nTODO planif aplicar");
}


void vFnSubCmdPlanifCambiar(int iComandoPos){
	char strPar[16];
	if(iFnGetArg (iComandoPos, 2, strPar, sizeof(strPar)) == 1){
		if(iFnCompararCadenas(strPar,"RR")==1){
			lFnSysSchedSetScheduler(RR);
		}else if(iFnCompararCadenas(strPar,"BTS")==1){
			lFnSysSchedSetScheduler(BTS);
		}else if(iFnCompararCadenas(strPar,"FIFO")==1){
			lFnSysSchedSetScheduler(FIFO);
		}else{
			vFnImprimir("\nEse algoritmo no existe (en el sodium quizas)");
		}
		
	}
	else{
		vFnImprimir("\nIngrese el nombre del algoritmo a cambiar!!");
	}
}


void vFnSubCmdPlanifDefecto(){
	lFnSysSchedSetScheduler(RR);
}


void vFnSubCmdPlanifMostrar(){
	long planificador=lFnSysSchedGetScheduler();
	switch(planificador){
		case RR: vFnImprimir("\nestas en RR");
				 break;			
		case FIFO: vFnImprimir("\nestas en FIFO");
				 break;
		case BTS: vFnImprimir("\nestas en BTS");
				 break;
		default: vFnImprimir("\nquien sabe que algoritmo es este...");
	}
}


void vFnSubCmdPlanifSet(int iComandoPos){
    char strPar[32];
    char strVariable[32+1];
    char strValor[32+1];
    int i;

    if(iFnGetArg (iComandoPos, 2, strPar, sizeof(strPar)) == 1) {
        i = 0;
        while( (strPar[i] != '=') && (strPar[i] != '\0') ) {
            strVariable[i] = strPar[i];
            i++;
        }
        strVariable[i] = '\0';
        
        /* Si tiene formato [planif set variable] o [planif set variable=],
         * entonces falta el valor de la variable; informamos por pantalla.
         */
        /* NOTA: al evaluar la segunda condicion, el '++i' deja incrementado el
         * valor de i; si se ejecuta la clausula ELSE, 'i' ya apunta al primer
         * caracter despues del '='.
         */
        if(strPar[i]=='\0' || strPar[++i]=='\0' || strPar[0]=='=') {
            vFnImprimir("\nParametros incorrectos, "
                        "tipee [planif] para mas informacion");
        } else {
            int j = i;
            while( strPar[i] != '\0' ) {
                strValor[i-j] = strPar[i];
                i++;
            }			
            strValor[i-j] = '\0';
           
            vFnImprimir("\nVariable: %s Valor: %s",strVariable,strValor);

            if(iFnCompararCadenas(strVariable,"quantum")==1) {
                lFnSysSchedSetParam(iFnCtoi(strValor));
                vFnImprimir("\n quantum= %d", uliQuantum);
            }
        }
    } else {
        vFnImprimir("\nIngrese el nombre de la variable y "
                    "el valor correspondiente!!");
    }
}
