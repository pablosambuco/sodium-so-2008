#!/bin/bash

LIST=listado
LIST_ASM=$LIST.asm
LIST_BIN=$LIST.bin
DIR_VARIABLE=../herramientas/dir_variable.sh
#Limpiamos el archivo
cat /dev/null > $LIST_ASM

# Lo que quiero es que el script me arme un listado de archivos assembler con nombre, segmento, offset y tamanio de cada archivo.
# Lo destacable es que el segmento y offset lo tiene que calcular de acuerdo al tamaño de archivo pasado al script como parámetro sólo si no es pasado en forma explícita.
#forma de uso:

#./arma_listado.sh 'sodium.sys: pos=0x7e00 tam_adicional=12546', 'loader.bin: tam_adicional=1000', 'mbr.bin'

limpiar_nombre()
{
NOMBRE_ARCH=`echo \`basename $PARAM_NOMBRE\``
echo $NOMBRE_ARCH | awk '{split (toupper($$0), a, ".");  printf ("\t\"%s",a[1]);printf("%*s\"\n",(15-length($0)),a[2]);}'

return 0
}

calcular_segmento_y_offset()
{
if test $PARAM_SEG -eq 0
then
	echo "calculando seg y offs"
	PARAM_SEG=`echo 1 | awk -v ult_segm=$ULTIMO_SEG -v ult_offs=$ULTIMO_OFFS -v ult_tam=$ULTIMO_TAM '{printf("%d",(ult_segm * 16 + ult_offs + ult_tam)/16 )}'`
	PARAM_OFFS=`echo 1 | awk -v act_segm=$PARAM_SEG -v ult_segm=$ULTIMO_SEG -v ult_offs=$ULTIMO_OFFS -v ult_tam=$ULTIMO_TAM '{printf("%d",(ult_segm * 16 + ult_offs + ult_tam) - (act_segm * 16))}'`

fi
ULTIMO_SEG=$PARAM_SEG
ULTIMO_OFFS=$PARAM_OFFS
ULTIMO_TAM=$PARAM_LEN
return 0
}

calcular_tamanio()
{
if test ! -f "$PARAM_NOMBRE"
then
	if test "$PARAM_NOMBRE" != "$LIST_BIN"
	then
		echo El archivo $PARAM_NOMBRE no existe. Abortando
		exit 1
	else
		PARAM_LEN=`echo $I | awk '{printf("%d",(int($1)+2)*19)}'`
	fi

else
	if test "`basename $PARAM_NOMBRE`" = "sodium.sys"
	then
		BSS_START=`$DIR_VARIABLE ../kernel/main.ld A __bss_start`
		END=`$DIR_VARIABLE ../kernel/main.ld A _end`
		BSS_SIZE=`echo \`printf "%d" $END\` \`printf "%d" $BSS_START\` | awk '{printf("%d",int($1) - int($2))}'`
		TAM_ADICIONAL_SODIUM=`echo $BSS_SIZE | awk '{printf("%d",int($1) + 65536 + 1024)}'`  # BSS + GDT + IDT

		# calculamos el proximo valor posible multiplo de 512 bytes
		TAM_ADICIONAL_SODIUM=`echo $TAM_ADICIONAL_SODIUM | awk '{printf("%d",int($1) + 512 - int($1)%512.0)}'`  
		
		#BIN_LEN=`wc -c "$PARAM_NOMBRE" | awk '{printf("%d",$1)}'`
		BIN_LEN=`du -b --block-size\=512 "$PARAM_NOMBRE" | awk '{printf("%d",int($1)*512)}'`
		PARAM_LEN=`echo $TAM_ADICIONAL_SODIUM $BIN_LEN | awk '{printf("%d",$1 + $2)}'`
		echo Tamanio adicional calculado para sodium.sys: $TAM_ADICIONAL_SODIUM
		echo BSS_START=$BSS_START, END=$END, BSS_SIZE=$BSS_SIZE
	else
	#BIN_LEN=`wc -c "$PARAM_NOMBRE" | awk '{printf("%d",$1)}'`
	BIN_LEN=`du -b --block-size\=512 "$PARAM_NOMBRE" | awk '{printf("%d",int($1)*512)}'`
	PARAM_LEN=`echo $PARAM_LEN $BIN_LEN | awk '{printf("%d",$1 + $2)}'`
	fi
fi
return 0
}

imprimir_tamanio()
{
echo \	$ULTIMO_TAM
return 0

}

imprimir_offset()
{
echo \	0x`printf "%x" $ULTIMO_OFFS`
return 0
}

imprimir_segmento()
{
echo \	0x`printf "%x" $ULTIMO_SEG`
return 0
}

parsear_param()
{
PARAM_NOMBRE=`echo $PARAM | awk '{split($$0,a,":");printf("%s",a[1])}'`
PARAM_RESTO=`echo $PARAM | awk '{split($$0,a,":");printf("%s",a[2])}'`

HEX_PARAM_OFFS=`echo $PARAM_RESTO | awk '{split($$0,a,"offs=");split(a[2],b," ");printf("%s",b[1])}'`
HEX_PARAM_SEG=`echo $PARAM_RESTO | awk  '{split($$0,a,"seg=");split(a[2],b," ");printf("%s",b[1])}'`
PARAM_LEN=`echo $PARAM_RESTO | awk  '{split($$0,a,"tam_adicional=");split(a[2],b," ");printf("%s",b[1])}'`

echo hex_param_offs $HEX_PARAM_OFFS
echo hex_param_seg $HEX_PARAM_SEG

PARAM_SEG=`printf "%d" $HEX_PARAM_SEG` 
PARAM_OFFS=`printf "%d" $HEX_PARAM_OFFS`


return 0
}

IFS=','
I=0
for PARAM in "$@"
do
	parsear_param
	calcular_tamanio
	calcular_segmento_y_offset

	echo offs $HEX_PARAM_OFFS $PARAM_OFFS
	echo seg  $HEX_PARAM_SEG $PARAM_SEG
	echo len  $PARAM_LEN

	I=`expr $I + 1`
	echo    Archivo$I:				>> $LIST_ASM
	echo -n .strNombre\	db			>> $LIST_ASM
	limpiar_nombre					>> $LIST_ASM
	echo -n .wSegmento \	dw			>> $LIST_ASM
	imprimir_segmento 				>> $LIST_ASM
	echo -n .wOffset \	dw			>> $LIST_ASM
	imprimir_offset 				>> $LIST_ASM
	echo -n .dwTamanio\	dd			>> $LIST_ASM
	imprimir_tamanio 				>> $LIST_ASM
	echo 						>> $LIST_ASM
done
echo    FIN:\	times 19 db\	\' \'			>> $LIST_ASM

exit 0

