#!/bin/bash

# Archivo check_utils.sh
# Verifica la presencia de cada programa necesario para la compilación, 
# instalación y prueba del SODIUM

export CC=${CC:-"gcc"}

# Ubicación del script que valida las banderas candidatas
CFLAGCHK_SH="./herramientas/cflag_chk.sh"


# Listado de binarios necesarios y deseables
HERRAMIENTAS_NECESARIAS="gcc ld nasm nm make cat wc dd sh mount objcopy objdump hexdump sed awk"
HERRAMIENTAS_DESEABLES_TEST="sfdisk parted qemu bochs bximage gdb mkfs.vfat mkisofs"
HERRAMIENTAS_DESEABLES_DOCS="doxygen dot"
HERRAMIENTAS_DESEABLES_ED="ctags indent diff vim"
HERRAMIENTAS_DESEABLES_CVS="svn"


# Funcion que recibe un listado de binarios ejecutables y valida la presencia
# de cada uno en alguno de los directorios listados en la variable de entorno
# PATH
VerificarEnPath()
{
errores=0	
for bin in $HERRAMIENTAS
do
	
	IFS=':'
	encontrado=0
	for path in '' $PATH
	do
		if test -x "${path}/${bin}"
		then
			echo "	"${bin}"	OK"	Encontrado en "${path}"
			encontrado=1
		fi
	done
	if test "$encontrado" -eq 0
	then
		echo "ADVERTENCIA:	${bin}	no fue encontrado en el path de ejecucion."
		echo "Descripcion detallada de "`grep ${bin} ./herramientas/whatis.txt`
		errores=1	
	fi	
	IFS=' '
done
return $errores
}

# Para verificar que el compilador $CC se encuentre en el sistema, y compile, 
# comprobamos que un parámetro estándar como -c devuelva un resultado válido
echo -n "Verificando funcionamiento de "$CC
if test -z "`$CFLAGCHK_SH -c`"
then
	echo " NOK! $CC no se encuentra correctamente instalado" 
	exit 1
else
	echo " OK"
fi
echo ""
echo ""
echo "================================================================================"
echo "Verificando la presencia en el path de los binarios necesarios para compilar el "
echo "     SODIUM"
echo "================================================================================"
HERRAMIENTAS=$HERRAMIENTAS_NECESARIAS
VerificarEnPath
if test $? -ne 0
then
	echo "Se han encontrado uno o mas errores en las verificaciones anteriores..."
	echo "Si no lo(s) resuelve no sera posible compilar SODIUM exitosamente en esta plataforma"
	exit 1
fi

echo ""
echo ""
echo "================================================================================"
echo "Verificando la presencia en el path de los binarios deseables para probar y "
echo "     depurar SODIUM dentro de un entorno simulado de ejecucion"
echo "================================================================================"
HERRAMIENTAS=$HERRAMIENTAS_DESEABLES_TEST
VerificarEnPath
if test $? -ne 0
then
	echo "Se han encontrado uno o mas errores en las verificaciones anteriores..."
	echo "Si no lo(s) resuelve no sera posible probar SODIUM exitosamente en esta plataforma"
fi

echo ""
echo ""
echo "================================================================================"
echo "Verificando la presencia en el path de los binarios deseables para generar la "
echo "     documentacion requerida por la catedra" 
echo "================================================================================"
HERRAMIENTAS=$HERRAMIENTAS_DESEABLES_DOCS
VerificarEnPath
if test $? -ne 0
then
	echo "Se han encontrado uno o mas errores en las verificaciones anteriores..."
	echo "Si no lo(s) resuelve no sera posible generar la documentacion del SODIUM desde esta plataforma"
fi

echo ""
echo ""
echo "================================================================================"
echo "Verificando la presencia en el path de los binarios deseables para editar el "
echo "     codigo fuente dentro del entorno del laboratorio" 
echo "================================================================================"
HERRAMIENTAS=$HERRAMIENTAS_DESEABLES_ED
VerificarEnPath
if test $? -ne 0
then
	echo "Se han encontrado uno o mas errores en las verificaciones anteriores..."
	echo "Si no lo(s) resuelve no tendra todas las herramientas de edicion a su disposicion"
fi

echo ""
echo ""
echo "================================================================================"
echo "Verificando la presencia en el path de los binarios deseables para versionar "
echo "    codigo fuente. Esta herramienta facilita ademas el trabajo remoto en grupo"
echo "================================================================================"
HERRAMIENTAS=$HERRAMIENTAS_DESEABLES_CVS
VerificarEnPath
if test $? -ne 0
then
	echo "Se han encontrado uno o mas errores en las verificaciones anteriores..."
	echo "Si no lo(s) resuelve no sera posible acceder a un repositorio svn desde esta plataforma"
fi
exit 0
