#include <stdio.h>
#include <stdlib.h>
#define N_TESTS 46

extern double vFnAtof(char*);

int main(void)
{
    //Numeros a convertir
    char test_strings [N_TESTS][50] = {
        "0",
        "+0",
        "-0",
        "+a",
        ".4-",
        "3.+",
        " 5",
        "    -5",
        "3.625",
        "-3.625",
        "556123.875",
        "-556123.875",
        "+0.2973232",
        "-0.2973232",
        "+0.297",
        "-0.297",
        "+100",
        "-100",
        "a3.625",
        "-a3.625",
        "556123,875-",
        "-556123,875-",
        "+0.qq297",
        "-0.297%",
        "+10q0",
        "-100",
        "99 9.9 99",
        "-999..999",
        "3.62  5",
        "-3.625  ",
        "556.123.875",
        "-556123.875",
        "-.",
        "+.",
        "999.9990234375",
        "999.999023437500000000000000000000",
        "-100.00000000000000000000000000",
        "-0.296999990940093994140625000000",
        "0.6555",
        "-0.6555",
        "556123.8759",
        "-556123.8759",
        "123456789123456789123456789.0",
        "-123456789123456789123456789.0",
        "1000200030004000500060007000.0",
        "-1000200030004000500060007000.0"
    };

    double output;
    double c_output;
    
    int tests_ok = 0;
    int tests_err = 0;
    int i;


    //TESTEO
    for(i=0; i<N_TESTS; i++ ) 
    {
        c_output = atof(test_strings[i]);
        output = vFnAtof(test_strings[i]);

        if( c_output == output )
        {
            tests_ok++;
            printf("\n\033[37;0m### OK Test %d\n", i);
            printf("No a convertir: (%s)\n",test_strings[i]);

            printf("Valor esperado: (%.50f)\n", c_output);
            printf("Valor obtenido: (%.50f)\n", output);
        }
        else
        { 
            tests_err++;
            printf("\n\033[31;1m### ERROR en el Test %d\n", i);
            printf("No a convertir: (%s)\n",test_strings[i]);

            printf("Valor esperado: (%.50f)\n", c_output);
            printf("Valor obtenido: (%.50f)\n", output);
        }
    }

    printf("\n\033[37;0m#########################\n"
            "%5d Tests\n%5d Aciertos\n%5d Fallos\n",
            tests_ok + tests_err, tests_ok, tests_err);

    return 0;
}
