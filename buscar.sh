#!/bin/bash

if (test $# -eq 0)
	then
		echo
		echo USO: $0 EXPRESION [[EXTENSION] i]
		echo
		echo EXPRESION Expresion a buscar
		echo EXTENSION Filtra por extension. \\* para todas
		echo i Ignora mayusculas/minusculas de EXPRESION
		echo
		exit
fi

if (test $3 && test $3 == i);
	then
   		insensitive="-i"
fi

if (test $2);
	then
    	find . -iname '*.'$2 -print | xargs grep -n -T -C 1 --color=auto --exclude-dir=.svn --exclude-dir=docs --exclude=tags -I -r $insensitive -e $1 /dev/null
	else
    	grep -n -T -C 1 --color=auto --exclude-dir=.svn --exclude-dir=docs --exclude=tags -I -r $insensitive -e $1 *
fi
