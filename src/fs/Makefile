# Path relativo al directorio principal del proyecto
DIR_SODIUM_BASE = ..

include $(DIR_SODIUM_BASE)/Makefile.cfg
OBJETOS_FS:= ramfs.o fat.o

all: $(OBJETOS_FS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.o: %.asm
	$(AS) $(ARFLAGS) -f elf $< -o $@ -l $@.lst

clean:
	$(RM) *.lst *.o *.s *.i
