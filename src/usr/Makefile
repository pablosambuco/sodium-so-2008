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
              tst_memd.bin \
              prog.bin \
              prog_wrapper.o \
              p1.bin

OBJETOS_LIBSODIUM:= sodstdio.o \
					sodstd.o \
                    sodstdlib.o \
                    sodstdlib_asm.o \
					sodstring.o \
					memoria_dinamica.o 

all:	$(OBJETOS_USR)  

clean:
	$(RM) *.lst *.bin *.o *.s *.i *.ld *.sys libsodium.tmp mapa_memoria.txt init_wrapper.c prog_wrapper.c idle_wrapper.c

install: init.bin prog.bin p1.bin idle.bin tst_ptrc.bin tst_sgnl.bin tst_ipc.bin tst_time.bin tst_flt.bin tst_memd.bin
	-mount $(DIR_MOUNT)
	cp prog.bin  $(DIR_MOUNT)/
	cp init.bin $(DIR_MOUNT)/
	cp tst_ptrc.bin $(DIR_MOUNT)/
	cp tst_sgnl.bin $(DIR_MOUNT)/
	cp tst_ipc.bin $(DIR_MOUNT)/
	cp tst_time.bin $(DIR_MOUNT)/
	cp tst_flt.bin $(DIR_MOUNT)/
	cp tst_memd.bin $(DIR_MOUNT)/
	cp idle.bin $(DIR_MOUNT)/
	cp p1.bin $(DIR_MOUNT)/
	cp $(DIR_TECLADO)/keymaps.bin $(DIR_MOUNT)/
	umount $(DIR_MOUNT)

# ATENCION: El 'ld' de libsodium se hace INCREMENTAL (opcion -r) ya que luego se
# 			debe linkear con los programas de usuario.
# 			De esta manera libsodium.o ya tiene todas las bibliotecas comunes.
# 			Esta NO es la mejor forma de hacerlo, pero sirve, al menos hasta que
# 			Sodium soporte bibliotecas dinamicas
libsodium.o: libsodium.tmp $(OBJETOS_LIBSODIUM)
	ld -o $@ --cref -N -r libsodium.tmp $(OBJETOS_LIBSODIUM)

libsodium.tmp: libsodium.c
	$(CC) $(CFLAGS) -c $^ -o $@

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

tst_%.bin: tst_%.o libsodium.o
	ld --cref -o tst_$*.ld -Ttext 0x0 -N -e main $^
	objcopy -R .comment -R .note -S -O binary tst_$*.ld $@

%.o: %.c
#	$(MAKEDEPEND) -a -- $(CFLAGS) $^
	$(CC) $(CFLAGS) -c $<

%.o: %.asm
	$(AS) $(ARFLAGS) -f elf $< -o $@ -l $@.lst

%.bin: %.asm
	$(AS) -o $@ $< -l $@.lst


# NO EDITAR EL MAKEFILE POR DEBAJO DE ESTA LINEA, SE MODIFICA AUTOMATICAMENTE
# NO SACAR LA LINEA ESCRITA EN INGLES, MARCA EL INICIO DE LO QUE MAKEDEPEND TOCA
# DO NOT DELETE THIS LINE -- make depend depends on it.

p1.o: ../include/usr/libsodium.h ../include/kernel/syscall.h
p1.o: ../include/kernel/signal.h ../include/kernel/semaforo.h
p1.o: ../include/kernel/shm.h ../include/kernel/tiempo.h
p1.o: ../include/kernel/definiciones.h ../include/usr/memoria_dinamica.h
../include/usr/libsodium.o: ../include/kernel/syscall.h
../include/usr/libsodium.o: ../include/kernel/signal.h
../include/usr/libsodium.o: ../include/kernel/semaforo.h
../include/usr/libsodium.o: ../include/kernel/shm.h
../include/usr/libsodium.o: ../include/kernel/tiempo.h
../include/usr/libsodium.o: ../include/kernel/definiciones.h
../include/usr/libsodium.o: ../include/usr/memoria_dinamica.h
../include/kernel/syscall.o: ../include/kernel/signal.h
../include/kernel/syscall.o: ../include/kernel/semaforo.h
../include/kernel/syscall.o: ../include/kernel/shm.h
../include/kernel/syscall.o: ../include/kernel/tiempo.h
../include/kernel/syscall.o: ../include/kernel/definiciones.h
../include/kernel/tiempo.o: ../include/kernel/definiciones.h

reloj.o: ../include/usr/reloj.h ../include/kernel/puertos.h
reloj.o: ../include/kernel/definiciones.h ../include/video.h
reloj.o: ../include/kernel/libk/string.h ../include/kernel/tiempo.h

idle.o: ../include/usr/libsodium.h ../include/kernel/syscall.h
idle.o: ../include/kernel/signal.h ../include/kernel/semaforo.h
idle.o: ../include/kernel/shm.h ../include/kernel/tiempo.h
idle.o: ../include/kernel/definiciones.h ../include/usr/memoria_dinamica.h

sodstdio.o: ../include/usr/sodstdio.h ../include/usr/libsodium.h
sodstdio.o: ../include/kernel/syscall.h ../include/kernel/signal.h
sodstdio.o: ../include/kernel/semaforo.h ../include/kernel/shm.h
sodstdio.o: ../include/kernel/tiempo.h ../include/kernel/definiciones.h
sodstdio.o: ../include/usr/memoria_dinamica.h ../include/usr/sodstdlib.h

sodstd.o: ../include/usr/sodstd.h ../include/usr/libsodium.h
sodstd.o: ../include/kernel/syscall.h ../include/kernel/signal.h
sodstd.o: ../include/kernel/semaforo.h ../include/kernel/shm.h
sodstd.o: ../include/kernel/tiempo.h ../include/kernel/definiciones.h
sodstd.o: ../include/usr/memoria_dinamica.h

sodstdlib.o: ../include/usr/sodstdlib.h

sodstring.o: ../include/usr/sodstring.h

memoria_dinamica.o: ../include/usr/libsodium.h ../include/kernel/syscall.h
memoria_dinamica.o: ../include/kernel/signal.h ../include/kernel/semaforo.h
memoria_dinamica.o: ../include/kernel/shm.h ../include/kernel/tiempo.h
memoria_dinamica.o: ../include/kernel/definiciones.h
memoria_dinamica.o: ../include/usr/memoria_dinamica.h
memoria_dinamica.o: ../include/usr/sodstdio.h

init.o: ../include/usr/libsodium.h ../include/kernel/syscall.h
init.o: ../include/kernel/signal.h ../include/kernel/semaforo.h
init.o: ../include/kernel/shm.h ../include/kernel/tiempo.h
init.o: ../include/kernel/definiciones.h ../include/usr/memoria_dinamica.h

tst_ptrc.o: ../include/usr/libsodium.h ../include/kernel/syscall.h
tst_ptrc.o: ../include/kernel/signal.h ../include/kernel/semaforo.h
tst_ptrc.o: ../include/kernel/shm.h ../include/kernel/tiempo.h
tst_ptrc.o: ../include/kernel/definiciones.h
tst_ptrc.o: ../include/usr/memoria_dinamica.h

tst_sgnl.o: ../include/usr/libsodium.h ../include/kernel/syscall.h
tst_sgnl.o: ../include/kernel/signal.h ../include/kernel/semaforo.h
tst_sgnl.o: ../include/kernel/shm.h ../include/kernel/tiempo.h
tst_sgnl.o: ../include/kernel/definiciones.h
tst_sgnl.o: ../include/usr/memoria_dinamica.h

tst_ipc.o: ../include/usr/libsodium.h ../include/kernel/syscall.h
tst_ipc.o: ../include/kernel/signal.h ../include/kernel/semaforo.h
tst_ipc.o: ../include/kernel/shm.h ../include/kernel/tiempo.h
tst_ipc.o: ../include/kernel/definiciones.h ../include/usr/memoria_dinamica.h

tst_time.o: ../include/usr/libsodium.h ../include/kernel/syscall.h
tst_time.o: ../include/kernel/signal.h ../include/kernel/semaforo.h
tst_time.o: ../include/kernel/shm.h ../include/kernel/tiempo.h
tst_time.o: ../include/kernel/definiciones.h
tst_time.o: ../include/usr/memoria_dinamica.h

tst_flt.o: ../include/usr/libsodium.h ../include/kernel/syscall.h
tst_flt.o: ../include/kernel/signal.h ../include/kernel/semaforo.h
tst_flt.o: ../include/kernel/shm.h ../include/kernel/tiempo.h
tst_flt.o: ../include/kernel/definiciones.h ../include/usr/memoria_dinamica.h
tst_flt.o: ../include/usr/sodstdio.h

tst_memd.o: ../include/usr/sodstdio.h ../include/usr/libsodium.h
tst_memd.o: ../include/kernel/syscall.h ../include/kernel/signal.h
tst_memd.o: ../include/kernel/semaforo.h ../include/kernel/shm.h
tst_memd.o: ../include/kernel/tiempo.h ../include/kernel/definiciones.h
tst_memd.o: ../include/usr/memoria_dinamica.h

prog.o: ../include/usr/libsodium.h ../include/kernel/syscall.h
prog.o: ../include/kernel/signal.h ../include/kernel/semaforo.h
prog.o: ../include/kernel/shm.h ../include/kernel/tiempo.h
prog.o: ../include/kernel/definiciones.h ../include/usr/memoria_dinamica.h

p1.o: ../include/usr/libsodium.h ../include/kernel/syscall.h
p1.o: ../include/kernel/signal.h ../include/kernel/semaforo.h
p1.o: ../include/kernel/shm.h ../include/kernel/tiempo.h
p1.o: ../include/kernel/definiciones.h ../include/usr/memoria_dinamica.h
../include/usr/libsodium.o: ../include/kernel/syscall.h
../include/usr/libsodium.o: ../include/kernel/signal.h
../include/usr/libsodium.o: ../include/kernel/semaforo.h
../include/usr/libsodium.o: ../include/kernel/shm.h
../include/usr/libsodium.o: ../include/kernel/tiempo.h
../include/usr/libsodium.o: ../include/kernel/definiciones.h
../include/usr/libsodium.o: ../include/usr/memoria_dinamica.h
../include/kernel/syscall.o: ../include/kernel/signal.h
../include/kernel/syscall.o: ../include/kernel/semaforo.h
../include/kernel/syscall.o: ../include/kernel/shm.h
../include/kernel/syscall.o: ../include/kernel/tiempo.h
../include/kernel/syscall.o: ../include/kernel/definiciones.h
../include/kernel/tiempo.o: ../include/kernel/definiciones.h
