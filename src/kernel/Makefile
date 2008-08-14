# Path relativo al directorio principal del proyecto
DIR_SODIUM_BASE = ..

include $(DIR_SODIUM_BASE)/Makefile.cfg

OBJETOS_DRIVERS:= floppy.o \
                  dma.o \
                  serial.o \
                  ide.o

OBJETOS_LIBK:=	  libk.o \
                  libk_asm.o \
                  string.o 
                 
OBJETOS_MEM:=     memoria_s.o \
                  memoria_k.o \
                  paginas.o
 
OBJETOS_SHELL:=   teclado.o \
                  shell.o \
                  cmd_planif.o \
                  cmd_lotes.o \
                  cmd_regs.o \
                  cmd_log.o

OBJETOS_TECLADO:= keymaps.bin

OBJETOS_USR:=     reloj.o \
		  idle_wrapper.o \
                  init_wrapper.o \
                  prog_wrapper.o

OBJETOS_FS:=      fat.o \
                  ramfs.o

OBJETOS_LOG:=     log.o

OBJETOS_LOTES:=   lotes.o

OBJETOS_PLANIF:=  rr.o \
                  fifo.o \
                  prioridad.o \
                  sjf.o \
                  bts.o \
                  srtf.o

OBJETOS_SO:=      main.o gdt.o registros.o puertos.o video.o sys_video.o \
                  system_asm.o system.o syscall.o semaforo.o shm.o \
                  $(addprefix $(DIR_KERNEL)/drivers/, $(OBJETOS_DRIVERS))\
                  $(addprefix $(DIR_KERNEL)/libk/, $(OBJETOS_LIBK))\
                  $(addprefix $(DIR_KERNEL)/mem/, $(OBJETOS_MEM))\
                  $(addprefix $(DIR_KERNEL)/log/, $(OBJETOS_LOG))\
                  $(addprefix $(DIR_KERNEL)/lotes/, $(OBJETOS_LOTES))\
                  $(addprefix $(DIR_KERNEL)/planif/, $(OBJETOS_PLANIF))\
                  $(addprefix $(DIR_SHELL)/, $(OBJETOS_SHELL))\
                  $(addprefix $(DIR_USR)/, $(OBJETOS_USR))\
                  $(addprefix $(DIR_FS)/, $(OBJETOS_FS))

all: sodium.sys

install:
 
# Si se desea se puede utilizar las mtools para copiar el sodium.sys al disquete
# Para ello hay que descomentar las siguientes dos lineas y comentar las que
# montan y desmontan la disquetera 
#	-mdel a:\sodium.sys
#	mcopy sodium.sys a:
# Mientras lo siguiente funcione, es mas portable que usar las mtools, 
# aparte al permitir que el comando mount falle hace que la instalacion soporte
# que se haya disparado con la disquetera montada:
	- mount $(DIR_MOUNT)
	cp sodium.sys $(DIR_MOUNT)/
#Aca se intentan copiar las distribuciones de teclado al diskette, TODO ver si esto anda bien!

	umount $(DIR_MOUNT)

clean:
	$(RM) *.lst *.bin *.o *.s *.i *.ld *.sys mapa_memoria.txt init_wrapper.c idle_wrapper.c prog_wrapper.c
	$(MAKE) -C $(DIR_KERNEL)/drivers $@
	$(MAKE) -C $(DIR_KERNEL)/libk $@
	$(MAKE) -C $(DIR_KERNEL)/mem $@
	$(MAKE) -C $(DIR_KERNEL)/log $@
	$(MAKE) -C $(DIR_KERNEL)/lotes $@
	$(MAKE) -C $(DIR_KERNEL)/planif $@
	$(MAKE) -C $(DIR_SHELL) $@
	$(MAKE) -C $(DIR_TECLADO) $@
	$(MAKE) -C $(DIR_USR) $@
	$(MAKE) -C $(DIR_FS) $@

sodium.sys: instalador.bin sodium.bin main.bin
	cat instalador.bin sodium.bin main.bin > sodium.sys

$(addprefix $(DIR_KERNEL)/drivers/, $(OBJETOS_DRIVERS)):
	$(MAKE) -C $(DIR_KERNEL)/drivers $(OBJETOS_DRIVERS)

$(addprefix $(DIR_KERNEL)/libk/, $(OBJETOS_LIBK)):
	$(MAKE) -C $(DIR_KERNEL)/libk $(OBJETOS_LIBK)

$(addprefix $(DIR_KERNEL)/mem/, $(OBJETOS_MEM)):
	$(MAKE) -C $(DIR_KERNEL)/mem $(OBJETOS_MEM)

$(addprefix $(DIR_KERNEL)/log/, $(OBJETOS_LOG)):
	$(MAKE) -C $(DIR_KERNEL)/log $(OBJETOS_LOG)

$(addprefix $(DIR_KERNEL)/lotes/, $(OBJETOS_LOTES)):
	$(MAKE) -C $(DIR_KERNEL)/lotes $(OBJETOS_LOTES)

$(addprefix $(DIR_KERNEL)/planif/, $(OBJETOS_PLANIF)):
	$(MAKE) -C $(DIR_KERNEL)/planif $(OBJETOS_PLANIF)

$(addprefix $(DIR_SHELL)/, $(OBJETOS_SHELL)):
	$(MAKE) -C $(DIR_SHELL) $(OBJETOS_SHELL)

$(addprefix $(DIR_TECLADO)/, $(OBJETOS_TECLADO)):
	$(MAKE) -C $(DIR_TECLADO) $(OBJETOS_TECLADO)

$(addprefix $(DIR_USR)/, $(OBJETOS_USR)):
	$(MAKE) -C $(DIR_USR) all

$(addprefix $(DIR_FS)/, $(OBJETOS_FS)):
	$(MAKE) -C $(DIR_FS) all

main.bin: $(OBJETOS_SO)
	ld -Map mapa_memoria.txt --cref -o main.ld -Ttext 0x0 \
	-N -e main $(OBJETOS_SO)
	objcopy -R .comment -R .note -S -O binary main.ld main.bin
	nm main.ld | sort | awk '{print $$3, 0x$$1}' > $(DIR_TEST)/simbolos.txt

sodium.bin: sodium.asm instalador.bin main.bin
	$(ENSAMBLA_NUCLEO) $< instalador.bin

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.o: %.asm
	$(AS) $(ARFLAGS) -f elf $< -o $@ -l $@.lst

%.bin: %.asm
	$(AS) -o $@ $< -l $@.lst
