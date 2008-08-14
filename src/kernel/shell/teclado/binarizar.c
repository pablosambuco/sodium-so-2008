typedef unsigned short int u_short;
typedef unsigned short int ushort;

#include <stdio.h>
#include <string.h>
//#include <fcntl.h>
#include <sys/stat.h>

#include "../../../include/shell/teclado.h"
#include KM 

#define CANTIDAD 6

void generarMatriz(int nro);
char * nombre();

struct _nombres {
    char codigo[5];
    char nombre[25];
};

typedef struct _nombres nombres;

nombres listado[CANTIDAD];

void cargarListado(int pos, char codigo[5], char nombre[25]) {
   strcpy(listado[pos].codigo,codigo);
   strcpy(listado[pos].nombre,nombre);
}

int main(void)
{
    stuDefinicionTeclado definicion;
    int i;
    FILE * fp;

    cargarListado(0, "us", "Ingles US");	
    cargarListado(1, "es", "Espa\244ol");
    cargarListado(2, "la", "Latinoamericano");
    cargarListado(3, "uk", "Ingles UK");
    cargarListado(4, "it", "Italiano");
    cargarListado(5, "de", "Aleman");

    fp=fopen(TECLADO,"wb");

    //Se cargan los valores provenientes de KM
    strcpy(definicion.sCodigo,IDIOMA);
    strcpy(definicion.sNombre,nombre());
    memcpy(definicion.stashMatrizNormal, ucMatrizNormal,
            sizeof(ucMatrizNormal));
    memcpy(definicion.stashMatrizShifted, ucMatrizShifted,
            sizeof(ucMatrizShifted));
    memcpy(definicion.stashMatrizAltGred, ucMatrizAltGred,
            sizeof(ucMatrizAltGred));

    //Y se graban en TECLADO
    fwrite(&definicion, sizeof(definicion),1L,fp);
    fclose(fp);
       
	return 0;
}

char *nombre()
{  int i;
   for(i=0;i<CANTIDAD;i++)
      if(strcmp(IDIOMA,listado[i].codigo)==0)
         return listado[i].nombre;
   return NULL;
}
