#!/bin/sh

export CC=${CC:-"gcc"}

# Archivo configurar.sh
# Genera el archivo Makefile.cfg necesario para definir el entorno
# de compilación SODIUM

# Nombre del archivo de configuración del entorno.
MAKEFILE_CFG="Makefile.cfg"

#
# Seteo los CFLAGS de acuerdo a las capacidades del compilador
#

# Ubicación del script que valida las banderas candidatas
CFLAGCHK_SH="./herramientas/cflag_chk.sh"

# Ubicación del script que valida la presencia de los binarios necesarios 
# y deseables para desarrollar el SODIUM
CHECK_UTILS_SH="./herramientas/check_utils.sh"
echo ""
echo ""
echo "================================================================================"
echo "Validando la presencia de los binarios requeridos para la compilacion, "
echo "	depuracion, documentacion y prueba del SODIUM"
echo "================================================================================"
`$CHECK_UTILS_SH > ./check_utils.log`
if test $? -ne 0
then
	echo "ERROR: No se cumplieron los requisitos basicos para la compilacion del SODIUM"
	echo "	Para mas informacion revise el archivo ./check_utils.log"
	exit 1
else
	echo "OK."
	echo "	Para mas informacion revise el archivo ./check_utils.log"
fi

# Banderas de uso obligatorio para compilar sodium
CFLAGS_BASE=`awk ' 
{
    if( $1 == "CFLAGS_BASE" )
     {
      # aquí extraemos los cflags base de la línea donde dice 
      # "CFLAGS_BASE := -fetc -fetc -fetc". 
      # Usamos el := de separador de campos.
      split($0, arrAsignacion, " := "); 
      split(arrAsignacion[2], arrFlags, " "); 
      for (flag in arrFlags)
	      if (index(arrFlags[flag],"\$")==0) print arrFlags[flag];
     }
}' $MAKEFILE_CFG`

# Banderas que debemos usar si el compilador las acepta.
CFLAGS_EXTRA=`awk ' 
{
    if( $1 == "CFLAGS_EXTRA" )
     {
      # aquí extraemos los cflags extra de la línea donde dice 
      # "CFLAGS_EXTRA := -fetc -fetc -fetc". 
      # Usamos el := de separador de campos.
      split($0, arrAsignacion, " := "); 
      split(arrAsignacion[2], arrFlags, " "); 
      for (flag in arrFlags)
	      if (index(arrFlags[flag],"\$")==0) print arrFlags[flag];
     }
}' $MAKEFILE_CFG`
 
echo ""
echo ""
echo "================================================================================"
echo "Verificando si $CC soporta los flags basicos de compilacion utilizados por el "
echo "	SODIUM"
echo "================================================================================"
for flag in $CFLAGS_BASE
do
	echo -n \	${flag}:
	$CFLAGCHK_SH $flag > /dev/null
	if test $? -eq 0
	then
		echo \	OK
	else
		echo \	Error! Flag no soportado! No es posible compilar SODIUM bajo esta plataforma. Para mayor informacion revise el archivo ./cflag-err${flag}.log
		exit 1
	fi
done

echo ""
echo ""
echo "================================================================================"
echo "Verificando si $CC requiere flags adicionales"
echo ""
echo "================================================================================"
# Aquí vemos cuáles de las cflags extra son soportadas por el GCC.
for flag in $CFLAGS_EXTRA
do
	echo -n \	${flag}:
	CFLAGS_OK="$CFLAGS_OK `$CFLAGCHK_SH $flag`"
	if test $? -eq 0
	then
		echo \	OK, se utilizara como flag adicional
	else
		echo \	OK, su uso no es necesario
	fi
done
echo ""
echo ""
if test "" != "`echo $CFLAGS_OK | awk '{print $1}' `"
then
	echo Se utilizaran las siguientes banderas extra para gcc version `$CC -dumpversion` := "$CFLAGS_OK"
else
	echo No fue necesario utilizar banderas extra para gcc version `$CC -dumpversion`
fi	

awk -v cflags_extra="$CFLAGS_OK" ' 
BEGIN{ 
   nombreCFLAGS = "CFLAGS";
   nombreCFLAGS_BASE = "CFLAGS_BASE";
   cflags_base= ""
}
{
   if( $1 == nombreCFLAGS_BASE )
     {
      # aquí extraemos los cflags de base de la línea donde dice 
      # "CFLAGS_BASE := -fetc -fetc -fetc". 
      # Usamos el := de separador de campos. Asumimos que se encuentan 
      # antes que CFLAGS
      split($0, arrTemp, " := "); 
      cflags_base = arrTemp[2];
      print $0
     }
   else
   if( $1 == nombreCFLAGS )
     {
	printf nombreCFLAGS " := " cflags_base cflags_extra "\n"
     }
   else
      print $0

}' $MAKEFILE_CFG > $MAKEFILE_CFG.aux

mv $MAKEFILE_CFG.aux $MAKEFILE_CFG
if test $? -eq 0 
then
	echo Configuracion exitosa
else	
	echo Error al actualizar el archivo $MAKEFILE_CFG !
fi
