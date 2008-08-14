#!/bin/bash
apt-get update

# Compilacion
apt-get install --force-yes nasm exuberant-ctags build-essential glibc-doc manpages-dev

# Maquinas virtuales
# Para tener todas las funcionalidades, convendria compilarlas a mano
apt-get install --force-yes qemu bximage gdb

# Documentacion
apt-get install --force-yes doxygen graphviz

# Subversion
apt-get install --force-yes subversion

# Para empezar a trabajar con SVN, ejecutar:
#
#   svn checkout svn://pablosambuco.no-ip.org:3690 Sodium --username APELLIDO
#
# eso crea un directorio Sodium en el directorio actual con la copia del
# repositorio.
# ATENCION: Puede tardar mucho tiempo! Mandarle un mail a Pablo Sambuco para
# que apague el eMule :P
#
# Un buen GUI para SVN en Ubuntu es esvn
#   sudo apt-get install esvn 
