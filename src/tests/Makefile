# Path relativo al directorio principal del proyecto
DIR_SODIUM_BASE = ..

include $(DIR_SODIUM_BASE)/Makefile.cfg

BIN_MBR		= mbr.bin
BIN_BOOTSECTOR	= bootSector.bin
BIN_PBR		= pbr.bin
BIN_LOADER	= loader.sys
BIN_LISTADO	= listado.bin
BIN_KERNEL	= sodium.sys
BIN_USR_PROG	= prog.bin
BIN_USR_INIT	= init.bin
BIN_USR_TST_TIME= tst_time.bin
BIN_USR_TST_PTRC= tst_ptrc.bin
BIN_USR_TST_SGNL= tst_sgnl.bin
BIN_USR_TST_IPC	= tst_ipc.bin
BIN_USR_TST_FLT	= tst_flt.bin
BIN_USR_IDLE	= idle.bin
BIN_USR_P1	= p1.bin
BIN_USR_L1	= lote1.txt
BIN_KEYMAPS	= keymaps.bin


MBR		= $(DIR_MBR)/$(BIN_MBR)
BOOTSECTOR	= $(DIR_SOLO)/$(BIN_BOOTSECTOR)
PBR		= $(DIR_SOLO)/$(BIN_PBR)
LOADER		= $(DIR_SOLO)/$(BIN_LOADER)
LISTADO		= $(DIR_SOLO)/$(BIN_LISTADO)
KERNEL		= $(DIR_KERNEL)/$(BIN_KERNEL)
PROG		= $(DIR_USR)/$(BIN_USR_PROG)
INIT		= $(DIR_USR)/$(BIN_USR_INIT)
IDLE		= $(DIR_USR)/$(BIN_USR_IDLE)
TST_TIME	= $(DIR_USR)/$(BIN_USR_TST_TIME)
TST_PTRC	= $(DIR_USR)/$(BIN_USR_TST_PTRC)
TST_SGNL	= $(DIR_USR)/$(BIN_USR_TST_SGNL)
TST_IPC		= $(DIR_USR)/$(BIN_USR_TST_IPC)
TST_FLT		= $(DIR_USR)/$(BIN_USR_TST_FLT)
P1			= $(DIR_USR)/$(BIN_USR_P1)
L1			= $(DIR_USR)/$(BIN_USR_L1)
KEYMAPS		= $(DIR_TECLADO)/$(BIN_KEYMAPS)

QEMU		= qemu
BOCHS		= bochs
BXIMAGE		= bximage

MKFS_VFAT	= /sbin/mkfs.vfat
PARTED		= /sbin/parted -s

CYLS		= `awk -F', ' '{split($$4,array,"=");print array[2];}' < $(SODIUM_HDD_CFG)`
HEADS		= `awk -F', ' '{split($$5,array,"=");print array[2];}' < $(SODIUM_HDD_CFG)`
SPT		= `awk -F', ' '{split($$6,array,"=");print array[2];}' < $(SODIUM_HDD_CFG)`

SFDISK		= /sbin/sfdisk -D -C$(CYLS) -H$(HEADS) -S$(SPT)

SODIUM_FDD		= sodium_fdd_test
SODIUM_FDD_IMG		= $(SODIUM_FDD).img
SODIUM_FDD_MKFS		= $(SODIUM_FDD).mkfslog
SODIUM_FDD_IMG_SIZE	= 1474560 # 18SECTORES_POR_TRACK * 80TRACkS_POR_CARA * 2CARAS * 512BYTES_POR_SECTOR

SODIUM_HDD		= sodium_hdd_test
SODIUM_HDD_IMG		= $(SODIUM_HDD).img
SODIUM_HDD_CFG		= $(SODIUM_HDD).cfg
SODIUM_HDD_MKFS		= $(SODIUM_HDD).mkfslog
SODIUM_HDD_IMG_SIZE	= 300 #DADO EN MBYTES #no usar menos de 100 para evitar problemas con el parted y la cantidad de particiones que estamos creando


LANZAR_QEMU	:= $(QEMU) -s -m 16 -localtime -fda $(SODIUM_FDD_IMG) -hda $(SODIUM_HDD_IMG) 

LANZAR_BOCHS	:= $(BOCHS) -q 'clock: sync=realtime'\
		'floppya: 1_44="$(SODIUM_FDD_IMG)", status=inserted' 'keyboard_type: at'\
		'vgaromimage: /usr/share/vgabios/vgabios.bin'

BIN_MBR_EN_FLOPPY	= $(BIN_MBR)
BIN_PBR_EN_FLOPPY	= $(BIN_PBR)
BIN_KERNEL_EN_FLOPPY	= $(BIN_KERNEL)
BIN_LOADER_EN_FLOPPY	= $(BIN_LOADER)
BIN_LISTADO_EN_FLOPPY	= $(BIN_LISTADO)
BIN_USR_PROG_EN_FLOPPY	= $(BIN_USR_PROG)
BIN_USR_INIT_EN_FLOPPY	= $(BIN_USR_INIT)
BIN_USR_TST_TIME_EN_FLOPPY=$(BIN_USR_TST_TIME)
BIN_USR_TST_PTRC_EN_FLOPPY=$(BIN_USR_TST_PTRC)
BIN_USR_TST_SGNL_EN_FLOPPY=$(BIN_USR_TST_SGNL)
BIN_USR_TST_IPC_EN_FLOPPY=$(BIN_USR_TST_IPC)
BIN_USR_TST_FLT_EN_FLOPPY=$(BIN_USR_TST_FLT)
BIN_USR_IDLE_EN_FLOPPY	= $(BIN_USR_IDLE)
BIN_USR_P1_EN_FLOPPY	= $(BIN_USR_P1)
BIN_USR_L1_EN_FLOPPY	= $(BIN_USR_L1)
BIN_KEYMAPS_EN_FLOPPY	= $(BIN_KEYMAPS)

FAT_TOOL = sodium_fat_drv

ALL := $(FAT_TOOL) img tags

IMGS = $(SODIUM_HDD_IMG) $(SODIUM_HDD_CFG) $(SODIUM_HDD_MKFS) $(SODIUM_FDD_IMG) $(SODIUM_FDD_MKFS)

all: $(ALL)

$(MBR):	
	$(MAKE) -C $(DIR_MBR)
$(PBR):	
	$(MAKE) -C $(DIR_SOLO)
$(KERNEL):	
	$(MAKE) -C $(DIR_KERNEL)
$(BOOTSECTOR):	
	$(MAKE) -C $(DIR_SOLO)
$(LOADER):	
	$(MAKE) -C $(DIR_SOLO)
tags:	
	$(MAKE) -C $(DIR_SODIUM_BASE) $@

$(FAT_TOOL): sodium_fat_drv.c sodium_fat_drv.h
	$(CC) $< -I . -o $@

$(SODIUM_FDD_IMG): $(KERNEL) $(MBR) $(BOOTSECTOR) $(PBR) $(LOADER) $(LISTADO) 
	$(DD) if=/dev/zero of=$(SODIUM_FDD_IMG) bs=$(SODIUM_FDD_IMG_SIZE) count=1
	$(MKFS_VFAT) -v -F12 $(SODIUM_FDD_IMG) > $(SODIUM_FDD_MKFS) 

	$(DD) if=$(BOOTSECTOR) of=$(SODIUM_FDD_IMG) "conv=notrunc"
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(KERNEL)	$(BIN_KERNEL_EN_FLOPPY)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(MBR)		$(BIN_MBR_EN_FLOPPY)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(PBR)		$(BIN_PBR_EN_FLOPPY)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(BOOTSECTOR)	$(BIN_BOOTSECTOR_EN_FLOPPY)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(LOADER)	$(BIN_LOADER_EN_FLOPPY)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(LISTADO)	$(BIN_LISTADO_EN_FLOPPY)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(PROG)		$(BIN_USR_PROG)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(INIT)		$(BIN_USR_INIT)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(TST_TIME)	$(BIN_USR_TST_TIME)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(TST_PTRC)	$(BIN_USR_TST_PTRC)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(TST_SGNL)	$(BIN_USR_TST_SGNL)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(TST_IPC)	$(BIN_USR_TST_IPC)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(TST_FLT)	$(BIN_USR_TST_FLT)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(IDLE)		$(BIN_USR_IDLE)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(P1)		$(BIN_USR_P1)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(L1)		$(BIN_USR_L1)
	./$(FAT_TOOL) $(SODIUM_FDD_IMG) $(KEYMAPS)	$(BIN_KEYMAPS)

$(SODIUM_HDD_IMG):  
	rm -f $(SODIUM_HDD_IMG)
	$(BXIMAGE) -q -hd -mode=flat -size=$(SODIUM_HDD_IMG_SIZE) $(SODIUM_HDD_IMG) | tail -1 > $(SODIUM_HDD_CFG)
	#$(MKFS_VFAT) -v -F16 $(SODIUM_HDD_IMG) > $(SODIUM_HDD_MKFS)
	$(PARTED) $(SODIUM_HDD_IMG) mklabel msdos

	echo     0,40,,   \\n\
	          ,40,,   \\n\
	          ,40,,   \\n\
	          ,1,13,  | $(SFDISK) -x $(SODIUM_HDD_IMG)
	
	$(PARTED) $(SODIUM_HDD_IMG) mkfs 1 fat16 
	$(PARTED) $(SODIUM_HDD_IMG) mkfs 2 fat16 
	$(PARTED) $(SODIUM_HDD_IMG) mkfs 3 fat16 

	$(PARTED) $(SODIUM_HDD_IMG) set 1 boot on 
	$(PARTED) $(SODIUM_HDD_IMG) set 1 hidden on 
	$(PARTED) $(SODIUM_HDD_IMG) set 1 lba off 
	$(PARTED) $(SODIUM_HDD_IMG) set 2 lba off 
	$(PARTED) $(SODIUM_HDD_IMG) set 3 lba off 
	
	$(PARTED) $(SODIUM_HDD_IMG) print

# WV: Comentado la creacion de imagen de disco, aun faltan unos ajustes.
img: $(SODIUM_FDD_IMG) $(SODIUM_HDD_IMG)
#img: $(SODIUM_FDD_IMG)

qemu_a: all
	$(LANZAR_QEMU) -boot a

qemu_c: all
	$(LANZAR_QEMU) -boot c

qemu_cdrom: all iso
	$(LANZAR_QEMU) -cdrom sodium.iso -boot d


bochs_a: all
	$(LANZAR_BOCHS)	'$(shell cat $(SODIUM_HDD_CFG))' 'boot: floppy'

bochs_c: all
	$(LANZAR_BOCHS)	'$(shell cat $(SODIUM_HDD_CFG))' 'boot: disk'

bochs_cdrom: all iso
	$(LANZAR_BOCHS)	'$(shell cat $(SODIUM_HDD_CFG))' \
        'ata1-master: type=cdrom, path="sodium.iso"' 'boot: cdrom'

gdb:
	gdb --quiet --ex 'target remote localhost:1234' -s ../kernel/main.ld

clean: 
#clean_img
	$(MAKE) -C $(DIR_SODIUM_BASE) $@
	rm -f $(ALL) *.o *.log tags
	-rm -rf cdrom_iso_tmp

clean_img:
	rm -f $(IMGS) 
	-rm -f *.iso
iso: all
	-mkdir cdrom_iso_tmp
	cp $(SODIUM_FDD_IMG) cdrom_iso_tmp/$(SODIUM_FDD_IMG)
	mkisofs -V SODIUM -b $(SODIUM_FDD_IMG) -o sodium.iso cdrom_iso_tmp
	-rm -rf cdrom_iso_tmp

