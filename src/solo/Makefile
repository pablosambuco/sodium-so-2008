# Path relativo al directorio principal del proyecto
DIR_SODIUM_BASE = ..

include $(DIR_SODIUM_BASE)/Makefile.cfg


ARMA_LISTADO:=arma_listado.sh

#El tamaño adicional del sodium.sys es el espacio que ocupa la bss + gdt + idt en memoria.
#Como es un caso particular... lo calculamos adentro del arma_listado.sh
#De manera que para el archivo sodium.sys el parametro tam_adicional= no tiene efecto.

#TODO - lala - Volver a poner esto en el listado
#              '$(DIR_USR)/tst_time.bin',\
#              '$(DIR_USR)/tst_ptrc.bin',\
#              '$(DIR_USR)/tst_sgnl.bin',\
#              '$(DIR_USR)/tst_ipc.bin',\

LIST_PARAMS:= '$(DIR_KERNEL)/sodium.sys: seg=0x7e0 offs=0x0',\
              '$(DIR_USR)/prog.bin',\
              '$(DIR_USR)/init.bin',\
              '$(DIR_USR)/tst_flt.bin',\
              '$(DIR_USR)/tst_memd.bin',\
              '$(DIR_USR)/p1.bin',\
              '$(DIR_USR)/idle.bin',\
              '$(DIR_SOLO)/pbr.bin',\
              '$(DIR_MBR)/mbr.bin',\
              '$(DIR_TECLADO)/keymaps.bin',\
              'listado.bin: seg=0x8000 offs=0x0'   

all: bootSector.bin pbr.bin listado.bin loader.sys

.PHONY: install listado.asm
#Compilación del sector de booteo del disquete en el cual el SODIUM se ejecuta.
bootSector.bin: bootSector.asm
	$(AS) $(ARFLAGS) $< -o $@

#Compilación del sector de booteo que se copiará en el primer sector de la partición del disco rígido el que se elija instalar el sodium
pbr.bin: pbr.asm
	$(AS) $(ARFLAGS) $< -o $@

loader.sys: loader.asm listado.bin
	$(AS) $(ARFLAGS) -d TAMANIO_LISTADO=`wc -c listado.bin  | awk '{print $$1}'` $< -o $@ -l loader.lst 

listado.asm:
	./$(ARMA_LISTADO) $(LIST_PARAMS)

listado.bin: listado.asm
	$(AS) $(ARFLAGS) $< -o $@ 

install:
	$(DD) if=bootSector.bin of=$(DISPOSITIVO_INST)

	- mount $(DIR_MOUNT)
	cp pbr.bin $(DIR_MOUNT)/
	cp loader.sys $(DIR_MOUNT)/
	cp listado.bin $(DIR_MOUNT)/
	umount $(DIR_MOUNT)

clean:
	@$(RM) listado.asm *.lst *.sys *.bin *.o
