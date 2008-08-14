# Path relativo al directorio principal del proyecto
DIR_SODIUM_BASE = ..

include $(DIR_SODIUM_BASE)/Makefile.cfg

all: mbr.bin 

mbr.bin: mbr.asm
	$(AS) $(ARFLAGS) $< -o $@ -l $@.lst

install:
	- mount $(DIR_MOUNT)
	cp mbr.bin $(DIR_MOUNT)/
	umount $(DIR_MOUNT)

clean:
	@$(RM) *.bin *.o *.lst
