typedef unsigned short int u_short;
typedef unsigned short int ushort;

/* Descomentar la siguiente linea para que se generen errores de compilacion
 * si no se conoce la equivalencia de algun keycode de linux
 */
//#define DEBUG_TECLADO

#include <stdio.h>
#include "../../../include/shell/teclado.h"
#include TECLADO

char matriz[3][30] = {
	"MatrizNormal",
	"MatrizShifted",
	"MatrizAltGred",
	};

void generarMatriz(int nro);

int main(void)
{
	generarMatriz(0);
	generarMatriz(1);
	generarMatriz(2);
	return 0;
}

void generarMatriz(int nro)
{
	int i,j;
	int tipo,valor;
	unsigned short int usiKeyCode;
	
	printf("\nunsigned char uc%s[] = {",matriz[nro]);

	//Los ScanCodes de los kmaps de linux son 256,
	//pero los que utiliza SODIUM son 86
	//Para facilitar la generacion de los codigos guardamos 104
	for(j=0;j<16;j++)
	{	
		printf("\n\t");
		for(i=0;i<8;i++)
		{
			switch(nro)
			{
				case 0:
					usiKeyCode=plain_map[j*8+i];
					break;
				case 1:
					usiKeyCode=shift_map[j*8+i];
					break;
				case 2:
					usiKeyCode=altgr_map[j*8+i];
					break;
			}

			tipo=KTYP(usiKeyCode);
			valor=KVAL(usiKeyCode);

//            printf("0x%x,", (usiKeyCode & 0xFF00)>>8 );
			printf("%2d,", tipo );

			switch(tipo)
			{
				case KT_FN:
					switch(valor)
					{
						case 0x00: //F1
						case 0x0C: //F1 con shift
							printf("TECLA_F1");
							break;
						case 0x01: //F2
						case 0X0D: //F2 con shift, y asi...
							printf("TECLA_F2");
							break;
						case 0x02:
						case 0x0E:
							printf("TECLA_F3");
							break;
						case 0x03:
						case 0x0F:
							printf("TECLA_F4");
							break;
						case 0x04:
						case 0x10:
							printf("TECLA_F5");
							break;
						case 0x05:
						case 0x11:
							printf("TECLA_F6");
							break;
						case 0x06:
						case 0x12:
							printf("TECLA_F7");
							break;
						case 0x07:
						case 0x13:
							printf("TECLA_F8");
							break;
						case 0x08:
						case 0x1E:
							printf("TECLA_F9");
							break;
						case 0x09:
						case 0x1F:
							printf("TECLA_F10");
							break;
						case 0x0A:
						case 0x20:
							printf("TECLA_F11");
							break;
						case 0x0B:
						case 0x21:
							printf("TECLA_F12");
							break;
						case 0x14:
							printf("TECLA_F20");
							break;
						case 0x15:
							printf("TECLA_INS");
							break;
						case 0x16:
							printf("TECLA_DEL");
							break;
						case 0x18:
							printf("TECLA_PGUP");
							break;
						case 0x19:
							printf("TECLA_PGDN");
							break;
						case 0x1D:
							printf("TECLA_PAUSA");
							break;
						//Ignorados
						case 0x17:
							printf("'\\0'");
							break;
						default:
#ifdef DEBUG_TECLADO
							printf("'FALTA DEFINIR KT_FN %d'", valor);
#else
							printf("'\\0'");
#endif

							break;
					}
					break;
				case KT_LETTER:
					switch(valor)
					{ 
						case 0xC7: //Cedilla mayuscula
							printf("128");
							break;
						case 0xD1: //Enie mayuscula
							printf("165");
							break;
						case 0xE7: //Cedilla minuscula
							printf("135");
							break;
						case 0xF1: //Enie minuscula
							printf("164");
							break;
						default:
							if( valor >= 128 ) //ASCII extendido
#ifdef DEBUG_TECLADO
								printf("'FALTA DEFINIR KT_LATIN %d'", valor);
#else 
							printf("'\\0'");
#endif
							else
								printf("'%c'",valor);
							break;
					}
					break;
				case KT_LATIN:
					switch(valor)
					{ 
						case 0x09:
							printf("TECLA_TAB");
							break;
						case 0x0A:
							printf("'\\n'");
							break;
						case 0x0D:
							printf("'\\r'");
							break;
						case 0x1C: //Separador de archivos
							printf("0x1c");
							break;
						case 0x26: //Ampersand
							printf("38");
							break;
						case 0x27:
							printf("'\\\''");
							break;
						case 0x5C:
							printf("'\\\\'");
							break;
						case 0xA1: //Abre signo admiracion
						case 0xEC: 
							printf("173");
							break;
						case 0x7F:
							printf("TECLA_BACKSPACE");
							break;
						case 0xAA:  //Esquina sup der (ALTGR+6 en teclado ESP)
						//En vez de imprimir ese simbolo, imprimimos el de 
						//'a' EN SUPERINDICE
							printf("166");
							break;
						case 0xAC: //Simbolo 1/4
						//En vez de imprimir ese simbolo, imprimimos el de 
						//ESQUINA SUPERIOR DERECHA (ALTGR+6 teclado ESP)
							printf("170");
							break;
						case 0xB7: //Punto en el medio (SHIFT+3 en teclado ESP)
							printf("250");
							break;
						case 0xBF: //Abre signo interrogacion
							printf("168");
							break;
						case 0xF1: //Enie minuscula (LAT)
							printf("164");
							break;
						case 0xBA: //Cero en superindice
							printf("167");
							break;
						case 0xD1: //Enie mayuscula (LAT)
							printf("165");
							break;
						case 0xA3: //Simbolo Euro 
							//Por no existir en el ASCII se utiliza este
							printf("238");
							break;
						default:
							if( valor >= 128 ) //ASCII extendido?
#ifdef DEBUG_TECLADO
								printf("'FALTA DEFINIR KT_LATIN %d'", valor);
#else 
							printf("'\\0'");
#endif
							else
								printf("'%c'",valor);
							break;
					}
					break;
				case KT_SHIFT:
					switch(valor)
					{ 
						case 0x00:  //SHIFT, no especifica lado, usamos derecho
							printf("SHIFT_DERECHO");
							break;
						case 0x01:
							printf("TECLA_ALTGR");
							break;
						case 0x02:
							printf("TECLA_CONTROL");
							break;
						case 0x03:
							printf("TECLA_ALT");
							break;
						case 0x04:
							printf("SHIFT_IZQUIERDO");
							break;
						case 0x05:
							printf("SHIFT_DERECHO");
							break;
						default:
#ifdef DEBUG_TECLADO
							printf("'FALTA DEFINIR KT_SHIFT %d'", valor);
#else 
							printf("'\\0'");
#endif
					}
					break;
				case KT_PAD:
					switch(valor)
					{
					/*	case 0x0A:
							printf("'+'");
							break;
						case 0x0B:
							printf("'-'");
							break;
						case 0x0C:
							printf("'*'");
							break;
						case 0x0D:
							printf("'/'");
							break;
						case 0x0E:
							printf("TECLA_ENTER");
							break;
						case 0x0F:
						case 0x10:
							printf("TECLA_DEL");
							break;
					*/
						default:
							printf("%d",valor);
					}
					break;
				case KT_SPEC:
					switch(valor)
					{
						case 0x00:
							printf("'\\0'");
							break;
						case 0x01:
							printf("TECLA_ENTER");
							break;
						case 0x07:
							printf("CAPS_LOCK");
							break;
						case 0x08:
							printf("NUM_LOCK");
							break;
						case 0x0A:
							printf("TECLA_HOME");
							break;
						case 0x0B:
							printf("TECLA_END");
							break;
                        //Ignorados
						case 0x02:
						case 0x03:
						case 0x05:  //SHIFT DERECHO
						case 0x06:  //CTRL IZQUIERDO
						case 0x09:
							printf("'\\0'");
							break;
						default:
#ifdef DEBUG_TECLADO
							printf("'FALTA DEFINIR KT_SPEC %d'", valor);
#else 
							printf("'\\0'");
#endif
							break;
					}
					break;
				case KT_CUR:
					switch(valor)
					{
						case 0x00:
							printf("TECLA_ABA");
							break;
						case 0x01:
							printf("TECLA_IZQ");
							break;
						case 0x02:
							printf("TECLA_DER");
							break;
						case 0x03:
							printf("TECLA_ARR");
							break;
						default:
#ifdef DEBUG_TECLADO
							printf("'FALTA DEFINIR KT_CUR %d'", valor);
#else 
							printf("'\\0'");
#endif
							break;
					}
					break;
				case KT_DEAD:
					switch(valor)
					{
						case 0x00:  //Acento grave (tilde invertida)
							printf("96");
							break;
						case 0x01:  //Acento (tilde, NO comilla simple)
						//En los teclados espanioles esta al lado de la enie
						//No figura en el codigo ascii, asi que usamos ' 
							printf("'\\\''");
							break;
						case 0x02:  //Acento circunflexo (^)
							printf("'^'");
							break;
						case 0x04:  //Dieresis (dieresis, NO comillas dobles)
						//En los teclados espanioles esta al lado de la enie
						//(usando shift)
						//No figura en el codigo ascii, asi que usamos "
							printf("'\\\"'");
							break;
						default:
#ifdef DEBUG_TECLADO
							printf("'FALTA DEFINIR KT_DEAD %d'", valor);
#else 
							printf("'\\0'");
#endif
							break;
					}
					break;
				case KT_CONS:
#ifdef DEBUG_TECLADO
					printf("'FALTA DEFINIR KT_CONS %d'", valor);
#else 
							printf("'\\0'");
#endif
					break;
				case KT_ASCII:
				//printf("'FALTA DEFINIR KT_ASCII %d'", valor);
				//TODO - Util para ingresar caract en ascii: ALT + 164
					printf("'\\0'");
					break;
			}
			printf(",");
		}
	}
	printf("\n};\n");
}
