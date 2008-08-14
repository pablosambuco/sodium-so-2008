MBR_EN_MEM		equ	0x0100 ; MBR leida del disco destino
PBR_EN_MEM		equ	0x0120 ; PBR leida del disco destino
FILE_MBR_EN_MEM		equ	0x0140 ; el MBR que leamos del diskette
FILE_PBR_EN_MEM		equ	0x0160 ; el PBR que leamos del diskette
ROOT_DIR_EN_MEM		equ	0x0180 ; aca cargamos el root dir del diskette/particion
FAT_EN_MEM		equ	0x2000 ; aca cargamos la fat del diskette/particion

SODIUM_EN_MEM		equ	0x1000 

MBR_MAGICNUMBER		equ	0xDED0
MBR_MN_OFFSET		equ	0x01BC
BEGIN_PT		equ	0x01BE
PART_ID_OFFSET		equ	0x04
PART_CHS_BEGIN_OFFSET	equ	0x01
PART_CHS_END_OFFSET	equ	0x05
PT_ENTRY_SIZE		equ	0x10

BPB_OEM_ID		equ	0x0003
BPB_BYTES_PER_SECT	equ	BPB_OEM_ID 	     + 0x08
BPB_SECT_PER_CLUSTER	equ	BPB_BYTES_PER_SECT   + 0x02
BPB_RESERVED_SECT	equ	BPB_SECT_PER_CLUSTER + 0x01
BPB_FATS		equ	BPB_RESERVED_SECT    + 0x02
BPB_ROOT_ENTRIES	equ	BPB_FATS             + 0x01
BPB_TOTAL_SECT		equ	BPB_ROOT_ENTRIES     + 0x02
BPB_DEV_DESC		equ	BPB_TOTAL_SECT       + 0x02
BPB_SECT_PER_FAT	equ	BPB_DEV_DESC         + 0x01
BPB_SECT_PER_TRACK	equ	BPB_SECT_PER_FAT     + 0x02
BPB_HEADS		equ	BPB_SECT_PER_TRACK   + 0x02
BPB_HIDDEN_SECT		equ	BPB_HEADS	     + 0x02
BPB_SECT_EXT		equ	BPB_HIDDEN_SECT      + 0x04
BPB_DEV_ID		equ	BPB_SECT_EXT	     + 0x04
BPB_FLAGS		equ	BPB_DEV_ID	     + 0x01
BPB_SIGNATURE		equ	BPB_FLAGS	     + 0x01
BPB_VOL_ID		equ	BPB_SIGNATURE	     + 0x01
BPB_VOL_NAME		equ	BPB_VOL_ID	     + 0x04
BPB_FS_TYPE		equ	BPB_VOL_NAME	     + 0x0b

bits 16
org 0x0

; el SOLO (loader.asm) nos cargo en 07E0:0000 (direccion lineal 7E00)
inicio:
	call	cBorrarPantalla
	; vemos si nos pasaron el numero "magico" de instalacion:
	cmp	ax, 0xf1f1
	je	.instalar 
	; vemos si nos pasaron el numero "magico" de desinstalacion:
	cmp	ax, 0xf2f2
	je	.desinstalar ; si saltamos directo a instalar, 
			     ; NASM se queja (error: short jump is out of range)
	; no nos pasaron nada, saltar al sodium:
.sodium:
	jmp	sodium
.instalar:
	jmp 	jInstalar
.desinstalar:
	jmp	jDesinstalar

cEsperarTecla:
	push	ax
	mov 	ah,0
	int 	16h
	pop	ax
	ret	

cBorrarPantalla:
	push	ax
	mov	ax, 0x03
	int	10h
	pop	ax
	ret
	
jInstalar:
	; cartelito entrada
	mov	si, strComienzoInstalacion
	call    cImprimir
	; esperamos una tecla
	call	cEsperarTecla
	; leemos el mbr
	mov	si, strLeyendoMBR
	call    cImprimir
	call	leerMbr
	; vemos si es el nuestro:
	call	mbrYaInstalado
	cmp	ax, 1
	jne	.buscarid13h
	mov	si, strMbrYaInstalado
	jmp	error
.buscarid13h
	mov	si, strMbrNoInstalado
	call	cImprimir
	; buscamos la particion para backupear el mbr
	mov	si, strBuscandoPartId13h
	call	cImprimir
	mov	byte [dbPartId], 13h
	call	buscarid
	cmp	ax, 1
	je	.backupMBR
	mov	si, strPartId13hNotFound
	jmp	error
.backupMBR:
	; comenzamos el backup del MBR
	mov	si, strBeginBackupMBR
	call	cImprimir
	call	backupMBR
	mov	si, strEndBackupMBR
	call	cImprimir
	; ahora empezamos a levantar cosas del floppy, inicializamos
	; los valores de BPB para FAT12:
	call	cCargarParamsFAT12
	; cargamos el root dir del floppy
	mov	si, strCargandoRootDirFloppy
	call	cImprimir
	call	cargarRootDirFloppy
	; buscamos el archivo strNombreMBR en el root dir que cargamos
	mov	si, strBuscandoFileMbr
	call	cImprimir
	mov	si, strNombreMBR
	call	buscarFileEnRootDir
	cmp	ax, 1
	je	.cargarFAT
	mov	si, strErrorFileMbrNF
	jmp	error
.cargarFAT
	; cargamos la FAT del floppy
	mov	si, strCargandoFatFloppy
	call	cImprimir
	call	cCargarFatFloppy
.leerfileMbr
	; leemos loader.bin
	mov	si, strLeyendoFileMBR
	call	cImprimir
	push	FILE_MBR_EN_MEM
	pop	es
	xor	bx, bx
	call	cargarFile
.instalarMbr
	; instalamos nuestro MBR
	mov	si, strInstalandoMBR
	call	cImprimir
	call	instalarMbr
.buscarPart16h
	; buscamos la particion 16h para instalar el PBR + sodium
	mov	si, strBuscandoPartId16h
	call	cImprimir
	mov	byte [dbPartId], 16h
	call	buscarid
	cmp	ax, 1
	je	.buscarHDDSolo
	mov	si, strPartId16hNotFound
	jmp	error
.buscarHDDSolo
	; buscamos el archivo strNombreHDDSOLO en el root dir que cargamos
	mov	si, strBuscandoFileHDDSolo
	call	cImprimir
	mov	si, strNombreHDDSOLO
	call	buscarFileEnRootDir
	cmp	ax, 1
	je	.leerHDDSolo
	mov	si, strErrorFileHDDSoloNF
	jmp	error
.leerHDDSolo:
	; leemos hddsolo.bin
	mov	byte [dbIdDispositivo], 0x00
	mov	si, strLeyendoFileHDDSolo
	call	cImprimir
	push	FILE_PBR_EN_MEM
	pop	es
	xor	bx, bx
	call	cargarFile
	; leemos el pbr (para obtener el BPB)
	mov	si, strLeyendoPBR
	call    cImprimir
	call	leerPbr
	; le anexamos la BPB a nuestra imagen de PBR y escribimos eso
	; en el arranque de la particion
	mov	si, strInstalandoHDDSolo
	call	cImprimir
	call	instalarPbr
.buscarSODIUM
	; buscamos el archivo strNombreSODIUM en el root dir que cargamos
	mov	si, strBuscandoFileSODIUM
	call	cImprimir
	mov	si, strNombreSODIUM
	call	buscarFileEnRootDir
	cmp	ax, 1
	je	.leerSODIUM
	mov	si, strErrorFileSODIUMNF
	jmp	error
.leerSODIUM:
	; leemos sodium.sys
	mov	byte [dbIdDispositivo], 0x00
	mov	si, strLeyendoFileSODIUM
	call	cImprimir
	push	SODIUM_EN_MEM
	pop	es
	xor	bx, bx
	call	cargarFile

	; despues de haber levantado todo del disco, pasamos a la segunda etapa:
	mov	si, strSegundaEtapa
	call	cImprimir
	call	cEsperarTecla

.cargarFat16	
	; despues de haber levantado SODIUM.SYS en memoria, no nos queda
	; mas que guardarlo en el disco, por lo cual al diskette no accedemos
	; mas, asi que cargamos los valores de la BPB para FAT16
	call	cCargarParamsFAT16
	; cargar la fat en si:
	mov	si, strCargandoFatHDD
	call	cImprimir
	call	cCargarFat16
	; armamos la concatenacion de clusters
	mov	si, strArmandoListaClusters
	call	cImprimir
	call	cArmarListaClusters
	; mostramos el inicio de la FAT (asumiendo que se guardo ahi)
	push	FAT_EN_MEM
	pop	es
	xor	di, di
	mov	cx, 0x40
	call 	printHexaBlock
	; cargamos root dir
	mov	si, strCargandoRootDirHDD
	call	cImprimir
	call	cCargarRootDirHDD
	; generamos la entrada en el root_dir
	mov	si, strGenerandoEntradaRootDir
	call	cImprimir
	call	cGenerarEntradaRootDir
	; mostramos como quedo el root dir:
	push	ROOT_DIR_EN_MEM
	pop	es
	xor	di, di
	mov	cx, 0x40
	call 	printHexaBlock
	; bajamos la fat de vuelta al disco
	mov	si, strEscribiendoFAT16
	call	cImprimir
	call	cEscribirFat16
	; bajamos el root dir
	mov	si, strEscribiendoRootDirHDD
	call	cImprimir
	call	cEscribirRootDirHDD
	; escribimos sodium.sys
	mov	byte [dbIdDispositivo], 0x80
	mov	si, strEscribiendoSODIUM
	call	cImprimir
	push	SODIUM_EN_MEM
	pop	es
	xor	bx, bx
	call	escribirFile
	
	; finalizada instalacion:
	mov	si, strInstalacionFinalizada
	jmp	fin

error:
	push	si
	mov	si, strError
	call	cImprimir
	pop	si
	jmp	fin

fin:
	call	cImprimir
	mov	si, strReiniciar
	call	cImprimir
	; esperamos una tecla
	call	cEsperarTecla
	; con int 19h forzamos al BIOS a iniciar el proceso de booteo nuevamente
	int	19h

jDesinstalar:
	mov	si, strComienzoDesinstalacion
	call	cImprimir
	; esperamos una tecla
	call	cEsperarTecla
	; leemos el mbr
	mov	si, strLeyendoMBR
	call    cImprimir
	call	leerMbr
	; vemos si es el nuestro:
	call	mbrYaInstalado
	cmp	ax, 1
	je	.buscarid13h
	mov	si, strMbrNoInstalado
	jmp	error
.buscarid13h:
	mov	si, strMbrYaInstalado
	call	cImprimir
	; buscamos la particion que contiene la copia del MBR
	mov	si, strBuscandoPartId13h
	call	cImprimir
	mov	byte [dbPartId], 13h
	call	buscarid
	cmp	ax, 1
	je	.restaurarMbr
	mov	si, strPartId13hNotFound
	jmp	error
.restaurarMbr
	mov	si, strRestaurandoMbr
	call	cImprimir
	call	restaurarMbr
.buscarPart16h
	; buscamos la particion 16h para eliminar el SODIUM.SYS
	mov	si, strBuscandoPartId16hDesinstalar
	call	cImprimir
	mov	byte [dbPartId], 16h
	call	buscarid
	cmp	ax, 1
	je	.leerPBR
	mov	si, strPartId16hNotFoundDesinstalar
	jmp	error
.leerPBR
	; leemos el pbr (para obtener el BPB)
	mov	si, strLeyendoPBR
	call    cImprimir
	call	leerPbr
	; cargamos los valores de la BPB para FAT16, para poder encontrar el root dir
	call	cCargarParamsFAT16
	; cargamos root dir
	mov	si, strCargandoRootDirHDD
	call	cImprimir
	call	cCargarRootDirHDD
	; eliminamos la entrada en el root_dir
	mov	si, strEliminandoEntradaRootDir
	call	cImprimir
	call	cEliminarEntradaRootDir
	; mostramos como quedo el root dir:
	push	ROOT_DIR_EN_MEM
	pop	es
	xor	di, di
	mov	cx, 0x40
	call 	printHexaBlock
	; bajamos el root dir
	mov	si, strEscribiendoRootDirHDD
	call	cImprimir
	call	cEscribirRootDirHDD

	; finalizada desinstalacion:
	mov	si, strDesinstalacionFinalizada
	jmp	fin

leerMbr:
	pusha
	mov	byte [dbIdDispositivo], 0x80 ; leer del disco
        mov     dl, byte [dbIdDispositivo]
        mov     dh, 0x00		; byte [dbCabeza]
        mov     ch, 0x00		; byte [dbPista]
        mov     cl, 0x01		; byte [dbSector]
	push	MBR_EN_MEM
	pop	es
	xor	bx, bx
	call	leerSectores
	popa
	ret

leerPbr:
	pusha
	mov	byte [dbIdDispositivo], 0x80 ; leer del disco
        mov     dl, byte [dbIdDispositivo]
	push	MBR_EN_MEM
	pop	es
	mov	di, word [dwSelectorParticion]
	mov	dh, byte [es:di+01]
	mov     cx, [es:di+02]      
	push	PBR_EN_MEM
	pop	es
	xor	bx, bx
	call	leerSectores
	popa
	ret

restaurarMbr:
	pusha
	mov	byte [dbIdDispositivo], 0x80 ; leer del disco
        mov     dl, byte [dbIdDispositivo]
	push	MBR_EN_MEM
	pop	es
	mov	di, word [dwSelectorParticion]
	mov	dh, byte [es:di+01]
	mov     cx, [es:di+02]      
	xor	bx, bx
	call	leerSectores
	mov	byte [dbIdDispositivo], 0x80 ; escribir en el disco
        mov     dl, byte [dbIdDispositivo]
        mov     dh, 0x00		; byte [dbCabeza]
        mov     ch, 0x00		; byte [dbPista]
        mov     cl, 0x01		; byte [dbSector]
	push	MBR_EN_MEM
	pop	es
	xor	bx, bx
	call	escribirSectores
	popa
	ret

leerSectores:
.main
        mov     di, 0x0005              ; 5 reintentos antes de dar error

.sector_loop
        mov     ah, 0x02                ; voy a leer
        mov     al, 0x01                ; cant de sectores = 1
        int     0x13                    ; leer!
        jnc     .lectura_ok             ; si no hubo carry --> lectura exitosa

	call	cImprimirWord
        xor     ax, ax
        int     0x13                    ; si hubo error se resetea la disketera
        dec     di                      ; se decrementa el contador de intentos
        jnz     .sector_loop            ; y si no me pase, reintento la lectura

        int	0x18                    ; sino bootea --> mejorar manejo de
                                        ; errores!!

.lectura_ok
        mov     si, strPunto            ; imprimir un punto cada vez que se lee
        call 	cImprimir

	ret

escribirSectores:
.main
        mov     di, 0x0005              ; 5 reintentos antes de dar error

.sector_loop
        mov     ah, 0x03                ; voy a escribir
        mov     al, 0x01                ; cant de sectores = 1
        int     0x13                    ; escribir!
        jnc     .lectura_ok             ; si no hubo carry --> lectura exitosa

	call	cImprimirWord
        xor     ax, ax
        int     0x13                    ; si hubo error se resetea la disketera
        dec     di                      ; se decrementa el contador de intentos
        jnz     .sector_loop            ; y si no me pase, reintento la lectura

        int 0x18                        ; sino bootea --> mejorar manejo de
                                        ; errores!!

.lectura_ok
        mov     si, strPunto            ; imprimir un punto cada vez que se lee
        call 	cImprimir

	ret

mbrYaInstalado:
	push	MBR_EN_MEM
	pop	es
	xor	ax, ax
	cmp	word [es:MBR_MN_OFFSET], MBR_MAGICNUMBER
	jne	.ret
	mov	ax, 1
.ret:
	ret

buscarid:
	push	cx

	xor    	cx, cx
	push	MBR_EN_MEM
	pop	es
	mov    	di, BEGIN_PT

.ptentry:
	mov	si, strNewLine
	call	cImprimir
	
	mov    	ax, cx
	call 	cImprimirByte
	mov   	si, strTab
	call 	cImprimir
	
	mov 	al, byte [es:di + PART_ID_OFFSET]
	cmp	al, byte [dbPartId]
	je	.encontrado
	call 	cImprimirByte
	
	inc    	cx
	add	di, PT_ENTRY_SIZE
	cmp    	cx, 4
	jnge   	.ptentry

.noencontrado:
	xor	ax, ax
	jmp	.ret
.encontrado:
	mov	word [dwSelectorParticion], di
	call 	cImprimirByte
	mov	ax, 1
.ret:
	pop	cx
	ret

; ya tenemos a DI apuntando a la entrada en la tabla de particiones que corresponde
; a la particion de backup de MBR (id 13h), y tenemos el MBR original en [MBR_EN_MEM].
; tenemos que escribir en el primer sector de esa particion el MBR (la restauracion es lo
; mismo pero al reves: MBR en el arranque de la particion -> arranque del disco)
backupMBR:
	pusha
	mov	byte [dbIdDispositivo], 0x80 ; escribir en el disco
	push	MBR_EN_MEM
	pop	es
        mov     dl, byte [dbIdDispositivo]
	mov	di, [dwSelectorParticion]
	mov	dh, byte [es:di+01]
	mov     cx, [es:di+02]      
	xor	bx, bx
	call	escribirSectores
	popa
	ret

cargarRootDirFloppy:
	; CX = tamanio del directorio raiz (donde se busca el loader)
	; CX = ( dwEntradasRaiz * 32 bytes ) / bytesPorSector
	; Nota: cada entrada tiene 32 bytes
	xor	ax, ax
	xor	dx, dx
	mov	ax, 32
	mov	bx, 0x00E0
	mul	bx	; word [dwEntradasRaiz]
	mov	bx, 0x0200
	div	bx	; word [dwBytesPorSector]
	xchg	ax, cx
	
	; AX = direccion del directorio raiz
	; AX = dbTotalFATs * dwSectoresPorFat + dwSectoresReservados
	mov	ax, 0x0002	; byte [dbTotalFATs]
	mov	bx, 0x0009
	mul	bx		; byte [dwSectoresPorFat]
	add	ax, 0x0001	; word [dwSectoresReservados]
	
	; dwSectorInicialDatos = base del directorio raiz + tamanio del directorio raiz
	; dwSectorInicialDatos = sector inicial de los datos
	mov	word [dwSectorInicialDatos], ax
	add	word [dwSectorInicialDatos], cx
	
	push	ROOT_DIR_EN_MEM
	pop	es
	mov	byte [dbIdDispositivo], 0x00 ; primer diskettera
	xor	bx, bx
	call	cLeerSectores
	ret

cCargarFatFloppy:
	; ahora si podemos proceder a cargar la FAT en memoria
	; CX = cantidad total de sectores de la FAT (cant de sectores a leer)
	xor	ax, ax
	mov	al, 0x02	; byte [dbTotalFATs]
	mov	bx, 0x0009
	mul	bx		; word [dwSectoresPorFat]
	mov	cx, ax

	; se calcula la ubicacion de la FAT y se guarda en AX
	mov	ax, 0x0001 	; word [dwSectoresReservados]

	; se lee la FAT en FAT_EN_MEM:0000
	push	FAT_EN_MEM
	pop	es
	xor	bx, bx
	call	cLeerSectores
	ret

; levanta el file que arranca en dwCluster
cargarFile:
	push	bx

.cargar:
	mov	ax, word [dwCluster]		; cluster a leer
	pop	bx				; buffer de lectura para el 
						; cluster

	call	cCluster2Lba			; convertir la direccion en LBA

	xor	cx, cx
	mov	cl, 0x01			; byte [dbSectoresPorCluster]	; cantidad de sectores a leer
	call	cLeerSectores

	push	bx

	; calcular el proximo cluster = 3/2 * cluster_actual
	mov	bx, word [dwCluster]
	mov	ax, word [dwCluster]

	shl	bx, 1
	add	bx, ax
	shr	bx, 1
	and	ax, 1
	push	es	; salvo ES
	push	FAT_EN_MEM
	pop	es
	mov	ax, [es:bx]
	jz	.cluster_par
.cluster_impar
	shr	ax, 4
.cluster_par
	pop	es	; recupero ES
	and	ax, 0000111111111111b		; obtengo los ultimos 12 digitos

	mov	word [dwCluster], ax		; si el cluster nuevo es 0x0FF0
	cmp	ax, 0x0FF0			; llegue al fin de archivo

	jb	.cargar
	pop	bx				; ajustamos el stack
	ret

escribirFile:
	push	bx

.escribir:
	mov	ax, word [dwCluster]		; cluster a leer
	pop	bx				; buffer de lectura para el 
						; cluster

	call	cCluster2Lba			; convertir la direccion en LBA

	xor	cx, cx
	mov	cl, byte [dbSectoresPorCluster]	; cantidad de sectores a leer
	call	cEscribirSectores

	push bx
	
	; calcular el proximo cluster
	mov	bx, word [dwCluster]
	shl	bx, 1
	push	es	; salvo ES
	push	FAT_EN_MEM
	pop	es
	mov	ax, [es:bx]
	mov	word [dwCluster], ax
	pop	es	; recupero ES
	cmp	ax, 0xFFFF			; llegue al fin de archivo

	jb	.escribir

	pop	bx
	ret
	
; rutina para instalar el MBR
instalarMbr:
	; primero copiamos el codigo de nuestro MBR a la imagen original, 
	; para volver a escribirla en el disco con la tabla de particiones
	; sin modificar pero con el codigo cambiado
	push	ds	; salvamos ds
	push	FILE_MBR_EN_MEM
	pop	ds
	push	MBR_EN_MEM
	pop	es
	xor	si, si
	xor	di, di
	mov     cx, 446	; los 446 bytes de codigo antes de llegar a la tabla
			; de particiones (incluyendo el numero magico 0xDED0)
	repnz   movsb
	pop	ds	; recuperamos ds

	; escribimos la imagen de nuevo en el disco
	mov	byte [dbIdDispositivo], 0x80 ; escribir en el disco
        mov     dl, byte [dbIdDispositivo]
	mov	dh, 0x00
	mov	ch, 0x00
	mov     cl, 0x01 
	push	MBR_EN_MEM
	pop	es
	xor	bx, bx
	call	escribirSectores
	ret

instalarPbr:
	push	ds	; salvamos ds
	push	PBR_EN_MEM
	pop	ds
	push	FILE_PBR_EN_MEM
	pop	es
	xor	si, si
	xor	di, di
	mov     cx, 62	; tamaño del BPB
	repnz   movsb
	pop	ds	; recuperamos ds

	mov	byte [dbIdDispositivo], 0x80 ; escribir en el disco
	mov     dl, byte [dbIdDispositivo]
	push	MBR_EN_MEM
	pop	es
	mov	di, word [dwSelectorParticion]
	mov	dh, byte [es:di+01]
	mov     cx, [es:di+02]
	push	FILE_PBR_EN_MEM
	pop	es
	xor	bx, bx
	call	escribirSectores
	ret

cCargarParamsFAT12:
	mov	word [dwBytesPorSector],     0x0200
	mov	word [dwSectoresPorPista],   0x0012
	mov	word [dwNroCabezas],         0x0002
	mov	byte [dbSectoresPorCluster], 0x0001
	ret

cCargarParamsFAT16:
	push	PBR_EN_MEM
	pop	es
	mov	ax, word [es:BPB_BYTES_PER_SECT]
	mov	word [dwBytesPorSector], ax
	mov	ax, word [es:BPB_SECT_PER_TRACK]
	mov	word [dwSectoresPorPista], ax
	mov	ax, word [es:BPB_HEADS]
	mov	word [dwNroCabezas], ax
	mov	al, byte [ES:BPB_SECT_PER_CLUSTER]
	mov	byte [dbSectoresPorCluster], al
	ret

; imprimir CX bytes del area de datos que arranca en ES:DI
printHexaBlock:
	pusha

	mov	si, strNewLine
	call	cImprimir
.loop:
	mov	al, byte [es:di]
	call	cImprimirByte
	inc	di
	loop	.loop	

	popa
	ret

;/**
; * buscarFileEnRootDir:
; * busca el nombre de archivo dado en SI, en el root dir cargado en la 
; * zona apuntada por AX, si lo encuentra devuelve 1 en ax, deja DI apuntando
; * a la entrada que contiene el archivo dentro del root dir, y guarda el primer 
; * cluster en la variable dwCluster; sino devuelve 0 en ax
; */
buscarFileEnRootDir:
	push	cx

	push	ROOT_DIR_EN_MEM
	pop	es

	; buscar dentro del directorio raiz el nombre del kernel
	mov	cx, 0x00E0; word [dwEntradasRaiz]		; hay [dwEntradasRaiz] entradas 
						; en el directorio
	xor	di, di
	xor	ax, ax
.loop
	push	cx
	mov	cx, 0x000B			; cada entrada tiene 11 
						; caracteres
	push	si
	push	di				; guardo la posicion inicial
	rep	cmpsb				; comparo SI con DI de a un byte

	pop	di				; recupero DI
	pop	si
	je	.encontrado			; si encontro el archivo lo 
						; cargo
	pop	cx				; si no lo encontro, recupero 
						; CX e incremento DI para que 
	add	di, 0x0020			; apunte a la siguiente entrada
						; en la tabla
	loop	.loop

	jmp	.ret				; no encontramos nada, volvemos directo

.encontrado:
	pop	cx				; ajustamos la pila
	mov	ax, 1
	mov	dx, word [es:di + 0x001A]	; en DI teniamos el inicio de 
						; la entrada en la tabla de 
						; directorios de [strNombreSO]
						; y dentro de dicha entrada
						; a un offset de 0x001A esta el
						; cluster inicial del mismo
	mov	word [dwCluster], dx		; que es almacenado en la 
						; variable auxiliar [dwCluster]
	mov	dx, word [es:di + 0x001C]	; tamaño del archivo
	mov	word [dwSize], dx		; lo guardamos en la variable dwSize

.ret:
	pop	cx				; recuperamos cx
	ret

;/*************************************************************************** 
; * cLeerSectores
; *    lee [CX] sectores a partir de [AX] y los almacena en la ubicacion de
; *    memoria apuntada por ES:BX
; ***************************************************************************/
cLeerSectores:
.main
	mov	di, 0x0005		; 5 reintentos antes de dar error
.sector_loop
	push	ax
	push	bx
	push	cx
	call	cLba2Chs
	call	cConvertCilindroSector

	mov	ah, 0x02		; voy a leer
	mov	al, 0x01		; cant de sectores = 1
        mov     dx, word [dwCabeza]
        shl     dx, 8
	mov	dl, byte [dbIdDispositivo]
	int	0x13			; leer!
	jnc	.lectura_ok		; si no hubo carry --> lectura exitosa

	xor	ax, ax
	int	0x13			; si hubo error se resetea la disketera
	dec	di			; se decrementa el contador de intentos
	pop	cx
	pop	bx
	pop	ax
	jnz	.sector_loop		; y si no me pase, reintento la lectura

	int 0x18			; sino bootea --> mejorar manejo de 
					; errores!!

.lectura_ok
	mov	si, strPunto		; imprimir un punto cada vez que se lee
	call	cImprimir
	pop	cx
	pop	bx
	pop	ax
	add	bx, word [dwBytesPorSector]	; incrementar el buffer de lectura
	inc	ax			; apuntar al siguiente sector
	loop	.main			; leer el siguiente sector

	ret				; no hay mas sectores para leer (CX=0)

;/*************************************************************************** 
; * cEscribirSectores
; *    escribe [CX] sectores a partir de [AX] leidos de la ubicacion de
; *    memoria apuntada por ES:BX
; ***************************************************************************/
cEscribirSectores:
.main
	mov	di, 0x0005		; 5 reintentos antes de dar error
.sector_loop
	push	ax
	push	bx
	push	cx
	call	cLba2Chs
	call	cConvertCilindroSector

	mov	ah, 0x03		; voy a escribir
	mov	al, 0x01		; cant de sectores = 1
        mov     dx, word [dwCabeza]
        shl     dx, 8
	mov	dl, byte [dbIdDispositivo]
	int	0x13			; leer!
	jnc	.lectura_ok		; si no hubo carry --> lectura exitosa

	xor	ax, ax
	int	0x13			; si hubo error se resetea el dispositivo
	dec	di			; se decrementa el contador de intentos
	pop	cx
	pop	bx
	pop	ax
	jnz	.sector_loop		; y si no me pase, reintento la lectura

	int 0x18			; sino bootea --> mejorar manejo de 
					; errores!!

.lectura_ok
	mov	si, strPunto		; imprimir un punto cada vez que se lee
	call	cImprimir
	pop	cx
	pop	bx
	pop	ax
	add	bx, word [dwBytesPorSector]	; incrementar el buffer de lectura
	inc	ax			; apuntar al siguiente sector
	loop	.main			; leer el siguiente sector

	ret				; no hay mas sectores para leer (CX=0)

;/*************************************************************************** 
; * cCluster2Lba
; *    convierte un cluster FAT en una direccion LBA (Linear Block Addresing)
; *    ( por ejemplo, LBA #1 = cyl: 0 / head: 0 / sector: 1 )
; *    LBA = ( cluster - 2 ) * dbSectoresPorCluster
; *
; *    LBA = ( cluster - 2 ) * dbSectoresPorCluster
; * IN:  AX = numero de cluster
; * OUT: AX = LBA #
; ***************************************************************************/
cCluster2Lba:
	dec	ax				; AX = AX - 2
	dec	ax

	xor	cx, cx
	mov	cl, byte [dbSectoresPorCluster]		; AX = AX * dbSectoresPorCluster
	mul	cx
	add	ax, word [dwSectorInicialDatos]		; AX = AX + dwSectorInicialDatos

	ret

;/*************************************************************************** 
; * cLba2Chs
; *    convierte una direccion LBA en CHS ( Cyl/Head/Sect)
; * donde:
; *    dbSector = (sectorLogico / dwSectoresPorPista) + 1
; *    dbCabeza = (sectorLogico / dwSectoresPorPista) MOD dwNroCabezas
; *    dbPista = sectorLogico / (dwSectoresPorPista * cantCabezas)
; ***************************************************************************/
cLba2Chs:
	push	bx
	xor	dx, dx
	div	word [dwSectoresPorPista]
	inc	dx
	mov	word [dwSector], dx

	xor	dx, dx
	div	word [dwNroCabezas]
	mov	word [dwCabeza], dx
	mov	word [dwCilindro], ax
	pop	bx

	ret

;/***************************************************************************
; * cConvertCilindroSector
; * Convierte el Cilindro y el Sector para ajustarlo al registro CX
; * IN:  Variables dwPista y dwSector
; * OUT: Registro CX con valores correspondientes.
; ***************************************************************************/
cConvertCilindroSector:
        push	ax
	mov	ax, word [dwCilindro]
	shr	ax, 2
	and	ax, 0x00C0
	mov	cx, ax
	mov	ax, word [dwCilindro]
	shl	ax, 8
	or	cx, ax
	mov	ax, word [dwSector]
	and	ax, 0x003F
	or	cx, ax
	pop	ax
	ret

; calcula cuantos clusters van a hacer falta para guardar [dwSize] bytes en la particion
; actual, usando la informacion del sector de BPB dentro de la PBR, devolviendo en AX
cClustersParaArchivo:
	push	PBR_EN_MEM
	pop	es
	xor	ax, ax
	mov	al, byte [es:BPB_SECT_PER_CLUSTER]
	mul	word [es:BPB_BYTES_PER_SECT]
	xchg	ax, cx
	mov	ax, word [dwSize]
	div	cx
	cmp	dx, 0x0000
	je	.nomasclusters
	inc	ax	; hace falta un cluster mas
.nomasclusters:
	ret

cCargarFat16:
	push	PBR_EN_MEM
	pop	es
	; ax: donde arranca la FAT (hidden + reservados)
	mov	ax, word [es:BPB_HIDDEN_SECT]
	xor	bx, bx
	mov	bl, byte [es:BPB_RESERVED_SECT]
	add	ax, bx
	; cx: cuantos sectores leer (sectores por fat)
	mov	cx, word [es:BPB_SECT_PER_FAT]
	; adonde guardar lo leido:
	push	FAT_EN_MEM
	pop	es
	xor	bx, bx
	mov	byte [dbIdDispositivo], 0x80
	call	cLeerSectores
	jmp	.ret
.checkFAT.1:
	cmp	word [es:0000], 0xfff8
	je	.checkFAT.2
	mov	si, strFatInvalida
	jmp	error
.checkFAT.2:
	cmp	word [es:0002], 0xffff
	je	.ret
	mov	si, strFatInvalida
	jmp	error
.ret:
	ret

; - la fat arranca con F8FF FFFF, por lo tanto empezamos a buscar clusters libres
; despues de pasar dichos bytes (4)
; - cada elemento es de 2 bytes, y es una lista enlazada, asi que la idea es buscar
; un elemento libre (0x0000), y guardar su pos en el puntero al siguiente del anterior
; - dos casos particulares: el primer cluster (no hay cluster anterior para escribir)
; y el ultimo (se pone FFFF como puntero al siguiente)
cArmarListaClusters:
	; cClustersParaArchivo calcula cuantos clusters requiere SODIUM.SYS en la partcion
	; destino, teniendo en cuenta el tamaño de los clusters en dicha partcion
	call	cClustersParaArchivo
	xchg	ax, cx 	; guardamos en CX la cantidad de clusters que tenemos que guardar
			; (perfecto para usarlo en la operacion "loop")
	; nos ubicamos en la FAT:
	push	FAT_EN_MEM
	pop	es
	; inicializamos los indices
	mov	di, 0x0002	; lo vamos a usar para indexar la FAT e ir buscando clusters libres
				; lo iniciamos en uno menos del primero (0x0004) porque se incrementa
				; al principio de la iteracion
	mov	bx, 0x0001	; lo vamos a usar para ir llevando el numero de cluster
				; tambien deberia inicializarse en uno mas (0x0002), pero dejamos en uno
				; para que se incremente dentro de la iteracion
	xor	ax, ax		; lo vamos a usar para guardar el cluster anterior
				; (adonde debemos anotar el siguiente cluster que encontremos)
.buscarinicial:
	add	di, 0x0002
	inc	bx
	cmp	word [es:di], 0x0000
	je	.inicialencontrado
	jmp	.buscarinicial
.inicialencontrado:
	mov	word [dwCluster], bx
	mov	ax, di
	dec	cx		; decrementamos porque ya tenemos uno
.buscarsiguiente:
	add	di, 0x0002
	inc	bx
	cmp	word [es:di], 0x0000
	je	.siguienteencontrado
	jmp	.buscarsiguiente
.siguienteencontrado:
	push	di
	mov	di, ax
	mov	word [es:di], bx
	pop	di
	mov	ax, di
	loop	.buscarsiguiente
	mov	di, ax
	mov	word [es:di], 0xffff
	
	ret	

; /* escribe las dos copias de la fat (una atras de la otra) */
cEscribirFat16:
	push	PBR_EN_MEM
	pop	es
	mov	ax, word [es:BPB_HIDDEN_SECT]
	xor	bx, bx
	mov	bl, byte [es:BPB_RESERVED_SECT]
	add	ax, bx
	mov	cx, word [es:BPB_SECT_PER_FAT]
	push	FAT_EN_MEM
	pop	es
	xor	bx, bx
	mov	byte [dbIdDispositivo], 0x80
	call	cEscribirSectores
	push	PBR_EN_MEM
	pop	es
	mov	ax, word [es:BPB_HIDDEN_SECT]
	xor	bx, bx
	mov	bl, byte [es:BPB_RESERVED_SECT]
	add	ax, bx
	mov	cx, word [es:BPB_SECT_PER_FAT]
	add	ax, cx	; pasamos la primer fat, quedamos parados en la segunda
	push	FAT_EN_MEM
	pop	es
	xor	bx, bx
	mov	byte [dbIdDispositivo], 0x80
	call	cEscribirSectores
	ret

cCargarRootDirHDD:
	push	PBR_EN_MEM
	pop	es
	; ax = de donde leer: (hidden + reservados + fats * sect_por_fat)
	mov	ax, word [es:BPB_HIDDEN_SECT]
	xor	bx, bx
	mov	bl, byte [es:BPB_RESERVED_SECT]
	add	ax, bx		; ax = hidden + reserved (1)
	xchg	ax, cx		; cx = ax
	xor	ax, ax
	mov	al, byte [es:BPB_FATS]
	mul	word [es:BPB_SECT_PER_FAT]	; ax = fats * sect_por_fat (2)
	add	ax, cx		; ax = (1) + (2)
	; cx = cuanto leer: (total entradas * 32 / bytes por sector)
	xchg	ax, bx		; bx = ax
	mov	ax, 0x0020
	mul	word [es:BPB_ROOT_ENTRIES]
	div	word [es:BPB_BYTES_PER_SECT]
	xchg	ax, cx		; cx = ax
	xchg	bx, ax		; ax = bx
	; sector de inicio de datos: inicio del rootdir + tamaño del rootdir
	mov	word [dwSectorInicialDatos], ax
	add	word [dwSectorInicialDatos], cx
	push	ROOT_DIR_EN_MEM
	pop	es
	xor	bx, bx
	mov	byte [dbIdDispositivo], 0x80
	call	cLeerSectores
	ret

cGenerarEntradaRootDir:
	push	ROOT_DIR_EN_MEM
	pop	es
	xor	di, di
.buscarentradalibre:
	cmp	byte [es:di], 0x00
	je	.encontrada
	add	di, 0x0020	
	jmp	.buscarentradalibre
.encontrada
	; movsb mueve de DS:SI a ES:DI. ES:DI ya lo tenemos apuntado
	; a donde queremos copiar (la entrada del root dir). falta
	; apuntar SI al nombre de archivo
	mov	si, strNombreSODIUM
	; copiamos 11 bytes:
	mov	cx, 0x000b	; nombre de archivo: 8 + 3 
	push	di	; salvamos di porque movsb lo corre
	repnz	movsb
	pop	di
	; cargamos cluster inicial y tamaño del archivo:
	mov	ax, [dwCluster]
	mov	word [es:di + 0x001A], ax
	mov	ax, [dwSize]
	mov	word [es:di + 0x001C], ax
		
	ret

cEliminarEntradaRootDir:
	push	ROOT_DIR_EN_MEM
	pop	es
	xor	di, di
.buscar.sodium:
	; movsb mueve de DS:SI a ES:DI. ES:DI ya lo tenemos apuntado
	; a donde queremos copiar (la entrada del root dir). falta
	; apuntar SI al nombre de archivo
	mov	si, strNombreSODIUM
	; comparamos 11 bytes:
	mov	cx, 0x000b	; nombre de archivo: 8 + 3 
	push	di	; salvamos di porque movsb lo corre
	rep	cmpsb
	pop	di
	je	.encontrada
	add	di, 0x0020	
	jmp	.buscar.sodium
.encontrada
	mov	byte [es:di], 0x00
		
	ret

cEscribirRootDirHDD:
	push	PBR_EN_MEM
	pop	es
	; ax = donde escribir: (hidden + reservados + fats * sect_por_fat)
	mov	ax, word [es:BPB_HIDDEN_SECT]
	xor	bx, bx
	mov	bl, byte [es:BPB_RESERVED_SECT]
	add	ax, bx		; ax = hidden + reserved (1)
	xchg	ax, cx		; cx = ax
	xor	ax, ax
	mov	al, byte [es:BPB_FATS]
	mul	word [es:BPB_SECT_PER_FAT]	; ax = fats * sect_por_fat (2)
	add	ax, cx		; ax = (1) + (2)
	; cx = cuanto leer: (total entradas * 32 / bytes por sector)
	xchg	ax, bx		; bx = ax
	mov	ax, 0x0020
	mul	word [es:BPB_ROOT_ENTRIES]
	div	word [es:BPB_BYTES_PER_SECT]
	xchg	ax, cx		; cx = ax
	xchg	bx, ax		; ax = bx
	push	ROOT_DIR_EN_MEM
	pop	es
	xor	bx, bx
	mov	byte [dbIdDispositivo], 0x80
	call	cEscribirSectores
	ret

; /*************************************************************************** 
;  * cImprimir:
;  *    imprime la cadena que se le pasa en SI hasta encontrar un '0'
;  ***************************************************************************/
jVolverImprimir:
cImprimir:
	lodsb 
	or	al, al
	jz	jFin
	mov	ah, 0x0E
	mov	bh, 0x00
	mov	bl, 0x07
	int	0x10
	jmp	jVolverImprimir
jFin
	ret

cImprimirByte:
	pusha
	call 	cByteAHexaString
	mov	si, strTmpByteAscStr
	call 	cImprimir
	popa
	ret
	
cImprimirWord:
	pusha
	call 	cWordAHexaString
	mov	si, strTmpWordAscStr
	call 	cImprimir
	popa
	ret
	
cNibbleAHexaChar:
	cmp	ax,0x0a
	jge	.asciiA
	add	ax,0x30
	jmp	.end
.asciiA:
	sub	ax,0x0a
	add	ax,0x61
.end:
	ret

cByteAHexaString:
	mov	bx,ax
	and	ax,0x0f
	call 	cNibbleAHexaChar
	mov	byte [strTmpByteAscStr+1], al
	mov	ax,bx
	shr	ax,4
	and	ax,0x0f
	call 	cNibbleAHexaChar
	mov	byte [strTmpByteAscStr+0], al
	ret
	
cWordAHexaString:
	mov	bx,ax
	
	shr	ax,0
	and	ax,0x0f
	call 	cNibbleAHexaChar
	mov	byte [strTmpWordAscStr+3], al
	
	mov	ax,bx
	shr	ax,4
	and	ax,0x0f
	call 	cNibbleAHexaChar
	mov	byte [strTmpWordAscStr+2], al
	
	mov	ax,bx
	shr	ax,8
	and	ax,0x0f
	call 	cNibbleAHexaChar
	mov	byte [strTmpWordAscStr+1], al
	
	mov	ax,bx
	shr	ax,12
	and	ax,0x0f
	call 	cNibbleAHexaChar
	mov	byte [strTmpWordAscStr+0], al
	ret

; "variables":
	dbPartId			db 0x00
	dbIdDispositivo 		db 0x80
	dwSelectorParticion		dw 0x0000

	; base (en cant de sectores) de donde comienzan los datos
	dwSectorInicialDatos		dw 0x0000

	; auxiliar para guardar el cluster de inicio del file en el
	; que estamos trabajando
	dwCluster			dw 0x0000
	; aca guardamos el tamaño de dicho file
	dwSize				dw 0x0000

	; auxiliares que me indican lo que se va leyendo
	dwSector			dw 0x0000
	dwCabeza			dw 0x0000
	dwCilindro			dw 0x0000

	; auxiliares para guardar los parametros que se encuentran en la
	; BPB (y poder adaptar las rutinas de conversion a FAT12/FAT16)
	dwBytesPorSector    		dw 0x0000
	dwSectoresPorPista  		dw 0x0000
	dwNroCabezas        		dw 0x0000
	dbSectoresPorCluster		db 0x00
	

; "constantes":
	strMensaje			db "(probamos de pasar primero por aca,apreta cualquier tecla)",10,13,0
	strComienzoInstalacion		db 10,13,"Comenzando la instalacion (presione cualquier tecla para continuar)",0
	strComienzoDesinstalacion	db 10,13,"Comenzando la desinstalacion (presione cualquier tecla para continuar)",0
	strLeyendoMBR			db 10,13,"Leyendo el MBR (Master BOOT Record) instalado: ",0
	strLeyendoPBR			db 10,13,"Leyendo el PBR instalado: ",0
	strMbrYaInstalado		db 10,13,"Nuestro MBR esta instalado en el disco",0
	strMbrNoInstalado		db 10,13,"Nuestro MBR no esta instalado en el disco",0
	strBuscandoPartId13h		db 10,13,"Buscando particion 13h para realizar el backup/restauracion del MBR...",0
	strPartId13hNotFound		db 10,13,"Error: PartId 13h no encontrada, imposible salvar/restaurar el MBR original",0
	strBuscandoPartId16h		db 10,13,"Buscando particion 16h para realizar la instalacion del SODIUM...",0
	strBuscandoPartId16hDesinstalar	db 10,13,"Buscando particion 16h para realizar la desinstalacion del SODIUM...",0
	strPartId16hNotFound		db 10,13,"Error: PartId 16h no encontrada, imposible instalar el SODIUM",0
	strPartId16hNotFoundDesinstalar	db 10,13,"Error: PartId 16h no encontrada, no se procedera a desinstalar el SODIUM",0
	strBeginBackupMBR		db 10,13,"Salvando el MBR original: ",0
	strEndBackupMBR			db 10,13,"El MBR original fue salvado en la particion destinada a tal fin",0
	strCargandoRootDirFloppy	db 10,13,"Cargando root_dir del diskette: ",0
	strCargandoRootDirHDD		db 10,13,"Cargando root_dir de la particion destino: ",0
	strBuscandoFileMbr		db 10,13,"Buscando la imagen de MBR a instalar...",0
	strErrorFileMbrNF		db 10,13,"Error: Imagen de MBR a instalar no encontrada en el diskette",0
	strBuscandoFileHDDSolo		db 10,13,"Buscando la imagen de PBR (Partition BOOT Record) a instalar...",0
	strErrorFileHDDSoloNF		db 10,13,"Error: Imagen de PBR a instalar no encontrada en el diskette",0
	strBuscandoFileSODIUM		db 10,13,"Buscando el SODIUM para instalar en la particion destino...",0
	strErrorFileSODIUMNF		db 10,13,"Error: Imagen del kernel del SODIUM no encontrada en el diskette",0
	strCargandoFatFloppy		db 10,13,"Cargando FAT del diskette: ",0
	strCargandoFatHDD		db 10,13,"Cargando FAT de la particion destino: ",0
	strLeyendoFileMBR		db 10,13,"Leyendo imagen del MBR a instalar: ",0
	strInstalandoMBR		db 10,13,"Instalando nuestro MBR: ",0
	strLeyendoFileHDDSolo		db 10,13,"Leyendo imagen del PBR a instalar: ",0
	strInstalandoHDDSolo		db 10,13,"Instalando nuestro PBR: ",0
	strLeyendoFileSODIUM		db 10,13,"Leyendo la copia del SODIUM: ",0
	strInstalacionFinalizada	db 10,13,"La instalacion ha concluido",0
	strRestaurandoMbr		db 10,13,"Restaurando el MBR original: ",0
	strDesinstalacionFinalizada	db 10,13,"La desinstalacion ha concluido",0
	strReiniciar			db 10,13,"[Presione cualquier tecla para reiniciar]",0
	strFatInvalida			db 10,13,"Error: La FAT cargada del disco rigido es invalida (no arranca con 0xF8FFFFFF)", 0
	strArmandoListaClusters		db 10,13,"Armando la concatenacion de clusters...",0
	strGenerandoEntradaRootDir	db 10,13,"Generando la entrada en el root_dir...",0
	strEliminandoEntradaRootDir	db 10,13,"Buscando la entrada en el root_dir para eliminar SODIUM.SYS...",0
	strSegundaEtapa			db 10,13,"Procediendo a la siguiente etapa de la instalacion (presione cualquier tecla para continuar)",0
	strEscribiendoFAT16		db 10,13,"Escrbiendo la FAT de vuelta al disco:",0
	strEscribiendoRootDirHDD	db 10,13,"Escrbiendo el root_dir de vuelta al disco:",0
	strEscribiendoSODIUM		db 10,13,"Escribiendo la imagen del SODIUM en el disco: ",0

	strPunto			db "#", 0
	strNewLine			db 10, 13, 0
	strTab				db 9, 0
	strTmpByteAscStr		db "00  ", 0
	strTmpWordAscStr		db "0000 ", 0

	strError			db 10, 13, "ERROR! Abortando: ", 0

	strNombreSODIUM			db "SODIUM  SYS", 0
	strNombreMBR			db "MBR     BIN", 0
	strNombreHDDSOLO		db "PBR     BIN", 0

sodium:
