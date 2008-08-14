# Makefile para los teclados
#
# Genera keymaps.bin para que sean incluidos los mapas de teclado en SODIUM

DIR_SODIUM_BASE=../../..

include ${DIR_SODIUM_BASE}/Makefile.cfg

ARCHIVOS=$(wildcard *.kmap.gz)
IDIOMAS=$(patsubst %.kmap.gz,%,$(ARCHIVOS))
TMPKEYMAP=tmp/defkeymap.c

all: keymaps.bin

$(IDIOMAS): unicode_a_sodium.c binarizar.c
	loadkeys -m $@.kmap.gz > tmp/$@.tmp
	$(CC) -DTECLADO=\"tmp/$@.tmp\" unicode_a_sodium.c -o unicode_a_sodium
	./unicode_a_sodium > $(TMPKEYMAP)
	$(CC) -DKM=\"$(TMPKEYMAP)\" -DTECLADO=\"tmp/$@.bin\" -DIDIOMA=\"$@\" binarizar.c -o binarizar
	./binarizar

keymaps.bin: $(IDIOMAS)
	cat tmp/*.bin > keymaps.bin

clean: 
	- rm unicode_a_sodium binarizar *.s *.i tmp/* keymaps.bin
