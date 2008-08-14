#include <stdio.h>
#include <string.h>
#define BUFFER_SIZE 512 //El numero entero mas largo en un double tiene:
                        //  309 cifras
                        //  + 1 signo 
                        //  + punto decimal
                        //  + las cifras decimales que se pidan
                        //  + 1 cifra decimal extra (para redondeo)
                        //  + 1 caracter nulo
#define N_TESTS 36
#define N_SPECIAL_VALS 14
#define N_MASKS 6
#define DOUBT_DIGITS 15


extern void vFnFtoa(char*, double, int);


int main(void)
{
    //Numeros a convertir
    double test_nums [N_TESTS] = {
        0,  // Estos primeros valores se cambian de valor mas abajo de manera
        0,  // especial (con operadores a nivel de bit), para que C no rompa
        0,  // todo con los casteos automaticos entre enteros y reales.
        0,  // Por las dudas los inicializamos en 0.
        0,  //
        0,  //
        0,  //
        0,  //
        0,  //
        0,  //
        0,  //
        0,  //
        0,  //
        0,  //
        5,
        -5,
        0.6555,
        -0.6555,
        556123.8759,
        -556123.8759,
        3.625,
        -3.625,
        556123.875,
        -556123.875,
        0.297,
        -0.297,
        100,
        -100,
        999.999,
        -999.999,
        123456789123456789123456789.0,
        -123456789123456789123456789.0,
        1000200030004000500060007000.0,
        -1000200030004000500060007000.0,
		9999999999999999999999999999999.0,
		-9999999999999999999999999999999.0
    };

    //Valores especiales
    //[0] Parte alta, [1] Parte baja
    unsigned long special_vals [N_SPECIAL_VALS][2] = {
	{0x00000000,0x7FF00000}, // Infinito +
	{0x00000000,0xFFF00000}, // Infinito -
	{0x00000001,0x7FF00000}, // NaN +
	{0x00000001,0xFFF00000}, // Nan -
	{0x00000000,0x00000000}, // Cero +
	{0x00000000,0x80000000}, // Cero -
    {0x00000001,0x00100000}, // Mas chico +
    {0x00000001,0x80100000}, // Mas chico -
    {0xFFFFFFFF,0x7FEFFFFF}, // Mas grande +
    {0xFFFFFFFF,0xFFEFFFFF}, // Mas grande -
    {0x00000000,0x7FE00000}, // Exponente grande, matisa chica, +
    {0x00000000,0xFFE00000}, // Exponente grande, matisa chica, -
    {0xFFFFFFFF,0x001FFFFF}, // Exponente chico, matisa grande, +
    {0xFFFFFFFF,0x801FFFFF}  // Exponente chico, matisa grande, -
    };

    //Mascaras de salida
    char masks[N_MASKS][15] = {
        "%.0f",
        "%.2f",
        "%.7f",
        "%.10f",
        "%.30f",
        "%.50f"
// Mascaras mas complejas que funcionan parcialmente
//        "%f"      //Default = 8 decimales
//        "%20.7f"
//        "%05.2f"
//        "%-25.10f"
//        "%+3.02f"
    };

    //Mascaras traducidas
    //TODO - Esto deberia ser automatico y completo
    int trans_masks[N_MASKS] = {
        0,
        2,
        7,
        10,
        30,
        50
// Mascaras mas complejas que funcionan parcialmente
// Traduccion de las mascaras mas complejas que funcionan parcialmente
//        8
//        7
//        2
//        10
//        2
    };


    //Buffer a donde se guardan las conversiones a string
    char buffer[BUFFER_SIZE];

    //Variables auxiliares
    char c_output[BUFFER_SIZE];
    int tests_ok = 0;
    int tests_err = 0;
    int tests_doubt = 0;
    int i,j;


    //Asignacion de valores especiales
    //Con un poco de magia de punteros podemos asignar valores 'enteros'
    //conocidos en hexa (4 bytes) a variables double (8 bytes),
    //sin que C haga el casteo
    long * punt = (long*) test_nums;
    for(i=0; i<N_SPECIAL_VALS; i++)
    {
        punt[i*2] = special_vals[i][0];
        punt[i*2+1] = special_vals[i][1];
    }


    //TESTEO
    for(i=0; i<N_TESTS; i++ )
    {
        for(j=0; j<N_MASKS; j++ )
        {
            sprintf(c_output, masks[j], test_nums[i]);
            vFnFtoa(buffer, test_nums[i], trans_masks[j]);

//            sprintf(buffer, masks[j],test_nums[i]-1e291);
//
//            //Probar con los tests 28 y 29
//            sprintf(buffer, masks[j],test_nums[i]+50000000000.0);
//
//            xftoa(test_nums[i],buffer);

            if( !strcmp(c_output, buffer) )
            {
                tests_ok++;
                printf("\n\033[37;0m### OK Test %d %c\n", i, j+'A');
                printf("No a convertir: (%.50f)\n",test_nums[i]);
                //printf("Numero a convertir: (%08x %08x)\n\n",
                //        punt[i*2],punt[i*2+1]);

                printf("Valor esperado: (%s)\n", c_output);
                printf("Valor obtenido: (%s)\n", buffer);
            }
            else
            { 
                tests_err++;
                printf("\n\033[31;1m### ERROR en el Test %d %c\n", i, j+'A');
                printf("No a convertir: (%.50f)\n",test_nums[i]);
                //printf("Numero a convertir: (%08x %08x)\n\n",
                //        punt[i*2],punt[i*2+1]);

                printf("Valor esperado: (%s)\n", c_output);
                printf("Valor obtenido: (%s)\n", buffer);
    
                if( strlen(c_output) == strlen(buffer) )
                {
                    c_output[DOUBT_DIGITS] = '\0';
                    buffer[DOUBT_DIGITS] = '\0';
                    if( !strcmp(c_output, buffer) )
                    {
                        tests_doubt++;
                        printf("\033[34;1m### DUDA en el Test %d %c\n",i,j+'A');
                        //printf("Numero a convertir: (%.50f)\n",test_nums[i]);
                        //printf("Numero a convertir: (%08x %08x)\n\n",
                        //        punt[i*2],punt[i*2+1]);

                        printf("Salidas para HASTA %d DIGITOS:\n",DOUBT_DIGITS);
                        printf("Valor esperado: (%s)\n", c_output);
                        printf("Valor obtenido: (%s)\n", buffer);
                    }
                }
            }
        }
    }

    printf("\n\033[37;0m#########################\n"
            "%5d Tests\n%5d Aciertos\n%5d Fallos (%d dudosos)\n\n",
            tests_ok + tests_err, tests_ok, tests_err, tests_doubt);

    return 0;
}
