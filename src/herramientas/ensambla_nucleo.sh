# El primer parametro de ensambla nucleo es el path correcto al sodium.asm y el segundo es el path a instalador.asm
SODIUM_ASM=$1
INSTALADOR_BIN=$2

DIR_VARIABLE="sh ../herramientas/dir_variable.sh"

#Constantes pasadas al nasm al compilar el sodium.asm
LOW_MEMORY_SIZE_ADDR=`$DIR_VARIABLE main.ld B uiTamanioMemoriaBaja`
BIOS_MEMORY_SIZE_ADDR=`$DIR_VARIABLE main.ld B uiTamanioMemoriaBios`
BOOT_DRIVE_ADDR=`$DIR_VARIABLE main.ld B uiUnidadBoot`
BSS_SIZE_ADDR=`$DIR_VARIABLE main.ld B uiTamanioBSS`
KERNEL_SIZE_ADDR=`$DIR_VARIABLE main.ld B uiTamanioKernel`
GDT_ADDR=`$DIR_VARIABLE main.ld B pdwGDT`
IDT_ADDR=`$DIR_VARIABLE main.ld B pstuIDT`
MEMORY_MODE_ADDR=`$DIR_VARIABLE main.ld B uiModoMemoria`
BSS_START=`$DIR_VARIABLE main.ld A __bss_start`
END=`$DIR_VARIABLE main.ld A _end`

BSS_SIZE=`expr \`printf "%d" $END\` - \`printf "%d" $BSS_START\``

KERNEL_SIZE=`printf "0x%x" \`wc -c main.bin | awk '{print $1}'\``
MAIN=`$DIR_VARIABLE main.ld T main` # direccion de la funcion main()!!!

INSTALADOR_SIZE=`printf "0x%x" \`wc -c $INSTALADOR_BIN | awk '{print $1}'\`` 

echo "LOW_MEMORY_SIZE_ADDR=$LOW_MEMORY_SIZE_ADDR"
echo "BIOS_MEMORY_SIZE_ADDR=$BIOS_MEMORY_SIZE_ADDR"
echo "BOOT_DRIVE_ADDR=$BOOT_DRIVE_ADDR"
echo "BSS_SIZE_ADDR=$BSS_SIZE_ADDR"
echo "KERNEL_SIZE_ADDR=$KERNEL_SIZE_ADDR"
echo "GDT_ADDR=$GDT_ADDR"
echo "IDT_ADDR=$IDT_ADDR"
echo "BSS_SIZE=$BSS_SIZE"
echo "KERNEL_SIZE=$KERNEL_SIZE"
echo "MAIN=$MAIN"
echo "MEMORY_MODE_ADDR=$MEMORY_MODE_ADDR"
echo "INSTALADOR_SIZE=$INSTALADOR_SIZE"

nasm -o sodium.bin \
-d LOW_MEMORY_SIZE_ADDR=$LOW_MEMORY_SIZE_ADDR \
-d BIOS_MEMORY_SIZE_ADDR=$BIOS_MEMORY_SIZE_ADDR \
-d BOOT_DRIVE_ADDR=$BOOT_DRIVE_ADDR \
-d BSS_SIZE_ADDR=$BSS_SIZE_ADDR \
-d KERNEL_SIZE_ADDR=$KERNEL_SIZE_ADDR \
-d GDT_ADDR=$GDT_ADDR \
-d BSS_SIZE=$BSS_SIZE \
-d KERNEL_SIZE=$KERNEL_SIZE \
-d IDT_ADDR=$IDT_ADDR \
-d TSS_ADDR=$TSS_ADDR \
-d MAIN=$MAIN \
-d INICIO_MEMORIA_ADDR=$INICIO_MEMORIA_ADDR \
-d MEMORY_MODE_ADDR=$MEMORY_MODE_ADDR \
-d INSTALADOR_SIZE=$INSTALADOR_SIZE \
$SODIUM_ASM -l sodium.lst
