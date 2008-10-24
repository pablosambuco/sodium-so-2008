#!/bin/bash
apt-get update

# Compilacion
apt-get install --force-yes nasm exuberant-ctags build-essential glibc-doc manpages-dev console-keymaps

# Maquinas virtuales
# Para tener todas las funcionalidades de bochs, usar instalar_bochs.sh
apt-get install --force-yes qemu bximage gdb

# Documentacion
apt-get install --force-yes doxygen graphviz

# Subversion
apt-get install --force-yes subversion
