# Path relativo al directorio principal del proyecto
DIR_SODIUM_BASE = ../..
DIR_DISTR=teclado

include $(DIR_SODIUM_BASE)/Makefile.cfg
OBJETOS_SHELL:= shell.o cmd_lotes.o cmd_planif.o cmd_log.o cmd_regs.o teclado.o

all: $(OBJETOS_SHELL) 

teclado.o: teclado.c keymaps.bin
	$(CC) $(CFLAGS) -DTAMANIO_ARCHKM=`wc -c teclado/keymaps.bin  | awk '{print $$1}'` -c $<

keymaps.bin:
	$(MAKE) -C teclado all

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.o: %.asm
	$(AS) $(ARFLAGS) -f elf $< -o $@ -l $@.lst

clean:
	$(RM) *.lst *.o *.s *.i
