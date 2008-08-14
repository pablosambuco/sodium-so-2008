#!/bin/bash
# Este pequenio script comprueba el parametro recibido puede a su vez ser usado
# como parametro del compilador.
# De esta forma se logra desactivar selectivamente las protecciones adicionales 
# propuestas por gcc, que probablemente harían que el SODIUM falle en tiempo 
# de ejecución.
# La idea es colocar este script "empaquetando" los parametros listados en las 
# CFLAGS. Aquellos que no sean comprendidos por gcc, quedarán "invisibles".
 
ARCH_TMP=`mktemp /tmp/prueba.XXXXXXXX`
echo "int main(){}" > $ARCH_TMP.c
echo "Probando flag "$1>./cflag_err${1}.log
$CC ${ARCH_TMP}.c -o $ARCH_TMP $1 2>>./cflag_err${1}.log

if test $? -eq 0  #si no hubo error, mostrar el parametro en stdout.
then
	echo $1
	rm -f ${ARCH_TMP}.[iosc] ./cflag_err${1}.log
	#El flag save-temps tiene un efecto colateral en la prueba, y es que 
	#deja los archivos temporales en el directorio de invocacion de gcc
	if test "$1" == "-save-temps"
	then
		rm -f `basename $ARCH_TMP`.[ios]	
	fi

	exit 0
fi
#si hubo error... no mostrar nada. De esta manera la bandera no se utilizará
rm -f ${ARCH_TMP}.[iosc]
exit 1
