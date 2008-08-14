# Path relativo al directorio principal del proyecto
DIR_SODIUM_BASE = ..

include $(DIR_SODIUM_BASE)/Makefile.cfg

OBJETOS_USR:= reloj.o \
              idle.bin \
              idle_wrapper.o \
              init.bin \
              init_wrapper.o \
              tst_ptrc.bin \
              tst_sgnl.bin \
              tst_ipc.bin \
              tst_time.bin \
              tst_flt.bin \
              prog.bin \
              prog_wrapper.o \
              p1.bin

all:	$(OBJETOS_USR)  

clean:
	$(RM) *.lst *.bin *.o *.s *.i *.ld *.sys mapa_memoria.txt init_wrapper.c prog_wrapper.c idle_wrapper.c

install: init.bin prog.bin p1.bin idle.bin tst_ptrc.bin tst_sgnl.bin tst_ipc.bin tst_time.bin tst_flt.bin 
	-mount $(DIR_MOUNT)
	cp prog.bin  $(DIR_MOUNT)/
	cp init.bin $(DIR_MOUNT)/
	cp tst_ptrc.bin $(DIR_MOUNT)/
	cp tst_sgnl.bin $(DIR_MOUNT)/
	cp tst_ipc.bin $(DIR_MOUNT)/
	cp tst_time.bin $(DIR_MOUNT)/
	cp tst_flt.bin $(DIR_MOUNT)/
	cp idle.bin $(DIR_MOUNT)/
	cp p1.bin $(DIR_MOUNT)/
	cp $(DIR_TECLADO)/keymaps.bin $(DIR_MOUNT)/
	umount $(DIR_MOUNT)

p1.bin: p1.o libsodium.o
	ld --cref -o p1.ld -Ttext 0x0 -N -e main $^
	objcopy -R .comment -R .note -S -O binary p1.ld p1.bin

init.bin: init.o libsodium.o
	ld --cref -o init.ld -Ttext 0x0 -N -e main $^
	objcopy -R .comment -R .note -S -O binary init.ld init.bin

init_wrapper.c: init.bin
	cat /dev/null > $@
	echo -n "unsigned char __init[] = {"	>> $@
	hexdump $(HEXDUMP_FLAGS) init.bin	>> $@
	echo "0x00 };"				>> $@
	echo "unsigned int __init_size(){ return sizeof(__init); }" >> $@
	echo "unsigned char *__init_begin(){ return __init; }" >> $@

prog.bin: prog.o libsodium.o
	ld --cref -o prog.ld -Ttext 0x0 -N -e main $^
	objcopy -R .comment -R .note -S -O binary prog.ld $@

prog_wrapper.c: prog.bin
	cat /dev/null > $@
	echo -n "unsigned char __prog[] = {"	>> $@
	hexdump $(HEXDUMP_FLAGS) prog.bin	>> $@
	echo "0x00 };"				>> $@
	echo "unsigned int __prog_size(){ return sizeof(__prog); }" >> $@
	echo "unsigned char *__prog_begin(){ return __prog; }" >> $@

idle.bin: idle.o libsodium.o
	ld --cref -o idle.ld -Ttext 0x0 -N -e main $^
	objcopy -R .comment -R .note -S -O binary idle.ld idle.bin

idle_wrapper.c: idle.bin
	cat /dev/null > $@
	echo -n "unsigned char __idle[] = {"	>> $@
	hexdump $(HEXDUMP_FLAGS) idle.bin	>> $@
	echo "0x00 };"				>> $@
	echo "unsigned int __idle_size(){ return sizeof(__idle); }" >> $@
	echo "unsigned char *__idle_begin(){ return __idle; }" >> $@

tst_flt.bin: tst_flt.o libsodium.o sodstdio.o sodstdlib.o sodstdlib_asm.o
	ld --cref -o tst_flt.ld -Ttext 0x0 -N -e main $^
	objcopy -R .comment -R .note -S -O binary tst_flt.ld $@

tst_%.bin: tst_%.o libsodium.o
	ld --cref -o tst_$*.ld -Ttext 0x0 -N -e main $^
	objcopy -R .comment -R .note -S -O binary tst_$*.ld $@

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.o: %.asm
	$(AS) $(ARFLAGS) -f elf $< -o $@ -l $@.lst

%.bin: %.asm
	$(AS) -o $@ $< -l $@.lst
