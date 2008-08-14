STACK_POINTER		equ     0x7C00
CARGA_FAT		equ     0x0500
COMIENZO_LOADER		equ     0x07E0
PART_LBA_BEGIN_OFFSET	equ	0x08

[bits 16]
org 0x7C00

	jmp short comienzo	;salto al inicio del bootstrap propiamente dicho
	nop
	; datos en la PBR para compatibilidad con FAT (16)
	strOemId		db	"SoloBoot"
	dwBytesPorSector	dw	0x0000			; cada sector es de 512
											; bytes
	dbSectoresPorCluster	db	0x00			; un sector por cluster
	dwSectoresReservados	dw	0x0000			; un sector para el boot
											; strap
	dbTotalFATs		db	0x00			; la posta y un backup
	dwEntradasRaiz		dw	0x0000			; 224 entradas en la 
											; tabla raiz
	dwTotalSectores		dw	0x0000 			; si la cant. de 
											; sectores es menor 
											; a 65535 van aca, sino
   											; va cero
	dbDescriptor		db	0x00			; dbDescriptor del 
											; dispositivo(?)
	dwSectoresPorFat	dw	0x0000			; sectores por fat
	dwSectoresPorPista	dw	0x0000			; sectores por pista
	dwNroCabezas		dw	0x0000			; numero de cabezas
	ddSectoresPbr		dd	0x00000000		; numero de sectores 
											; ocultos
	ddNroSectores		dd	0x00000000		; cuando los sectores 
											; son mayores a 65535 
											; entonces 
											; dwTotalSectores = 0 
											; y aca va dicho valor
	dbNroDispositivo	db	0x00			; numero de dispositivo 
								; fisico
	dbFlags			db	0x00			; no se usa, reservado
	dbSignature		db	0x00			; firma...
	ddIdVolumen		dd	0x00000000		; aca va fruta
	strNombreVolumen	db	"SODIUM BOOT"
	strTipoFS		db	"FAT16   "

comienzo:
	cli				; deshabilitar interrupciones
	xor	ax, ax
	mov	ss, ax			; seteo el SS

	mov	sp, STACK_POINTER	; seteo el SP

	sti						; habilitar interrupciones

	push	cs
	pop		ds				; seteo DS

	push	cs
	pop		es				; seteo ES

	push	cs
	pop		fs				; seteo FS

	push	cs
	pop		gs				; seteo GS


   	; borrar la pantalla (se realiza al setear el modo de video)
   	; en particular el modo 0x03 es texto, 25 filas X 80 columnas
	mov		ax, 0x3
	int		0x10

	mov	bp, si		; para no perder lo q me dejo el bootloader en SI
	mov	si, strMsgCarga	; imprimir mensaje
	call cImprimir

cargar:
	; BX = tamanio del directorio raiz (donde se busca el loader)
	; BX = ( dwEntradasRaiz * 32 bytes ) / bytesPorSector
	; Nota: cada entrada tiene 32 bytes
	xor	ax, ax
	xor	dx, dx
	mov	ax, 32
	mul	word [dwEntradasRaiz]
	div	word [dwBytesPorSector]
	xor	ebx, ebx
	mov	bx, ax

	; EAX = direccion del directorio raiz
	; EAX = dbTotalFATs * dwSectoresPorFat + dwSectoresReservados + Direccion PBR
	xor	eax, eax
	mov	al, byte [dbTotalFATs]
	mul	byte [dwSectoresPorFat]
	add	ax, word [dwSectoresReservados]
	add	eax, dword [ddSectoresPbr]

	; dwSectorInicialDatos = base del directorio raiz + tamanio del directorio raiz
	; dwSectorInicialDatos = sector inicial de los datos
	mov	dword [ddSectorInicialDatos], eax
	add	dword [ddSectorInicialDatos], ebx

	; leer el directorio raiz en ES:CARGA_FAT = 0x0000:CARGA_FAT
	mov	bx, CARGA_FAT
	mov	cx, 0x0001
	call	cLeerSectores

	; buscar dentro del directorio raiz el nombre del kernel
	mov	cx, word [dwEntradasRaiz]		; hay [dwEntradasRaiz] entradas 
						; en el directorio
	mov	di, CARGA_FAT			; se leyo en ES:CARGA_FAT
.loop
	push	cx
	mov	cx, 0x000B			; cada entrada tiene 11 
						; caracteres
	mov	si, strNombreSO		; nombre a comparar con 
						; c/entrada
	push	di				; guardo la posicion inicial
	rep	cmpsb				; comparo SI con DI de a un byte

	pop	di				; recupero DI
	je	cargar_fat			; si encontro el archivo lo 
						; cargo
	pop	cx				; si no lo encontro, recupero 
						; CX e incremento DI para que 
	add	di, 0x0020			; apunte a la siguiente entrada
						; en la tabla
	loop	.loop

	jmp	error_carga			; si llegue hasta aca es 

error_carga1:
	jmp	error_carga

; antes de cargar la FAT necesitamos guardar la posicion del archivo
cargar_fat:
	mov	dx, word [di + 0x001A]		; en DI teniamos el inicio de 
						; la entrada en la tabla de 
						; directorios de [strNombreSO]
						; y dentro de dicha entrada
						; a un offset de 0x001A esta el
						; cluster inicial del mismo
	mov	word [dwCluster], dx		; que es almacenado en la 
						; variable auxiliar [dwCluster]

	; ahora si podemos proceder a cargar la FAT en memoria
	; CX = cantidad total de sectores de la FAT (cant de sectores a leer)
	mov	cx, 0x0001

	; se calcula la ubicacion de la FAT y se guarda en AX
	xor	eax, eax
	mov	ax, word [dwSectoresReservados]
	add	eax, dword [ddSectoresPbr]

	; se lee la FAT en ES:CARGA_FAT
	mov	bx, CARGA_FAT
	call	cLeerSectores


	; en este punto ya tenemos la FAT en memoria
	; se lee la imagen en COMIENZO_LOADER:0x0000
	mov     ax, COMIENZO_LOADER
	push    ax
	pop     es
	xor     bx, bx
	push    bx

cargar_kernel:
	mov	ax, word [dwCluster]		; cluster a leer
	pop	bx				; buffer de lectura para el 
						; cluster

	call	cCluster2Lba			; convertir la direccion en LBA

	xor	cx, cx
	mov	cl, byte [dbSectoresPorCluster]	; cantidad de sectores a leer
	call cLeerSectores

	push bx
	
	; calcular el proximo cluster = 3/2 * cluster_actual
	mov	bx, word [dwCluster]
	shl	bx, 1
	mov	ax, [CARGA_FAT + BX]
	mov	word [dwCluster], ax
	cmp	ax, 0xFFFF			; llegue al fin de archivo

	jb	cargar_kernel

	mov	ax,COMIENZO_LOADER	; Set segment registers
	push	ax
	pop	ds
	push	ax
	pop	es
	push	ax
	pop	fs
	push	ax
	pop	gs

	; se escribe al port de la diskettera para parar el motor
	; y que no siga girando al pedo
	;xor	ax, ax		; al = 0 -> comando para parar el motor
	;mov	dx, 0x3F2	; port 0x3F2 -> diskettera
	;out	dx, al		; la escritura propiamente dicha

	jmp 	COMIENZO_LOADER:0x0000

error_carga:
	mov	si, strMsgError
	call	cImprimir
	xor	ah, ah
	int	0x16
	int	0x19

;/*************************************************************************** 
; * -----------------------------Subrutinas----------------------------------
; ***************************************************************************/

;/*************************************************************************** 
; * cLeerSectores
; *    lee [CX] sectores a partir de [AX] y los almacena en la ubicacion de
; *    memoria apuntada por ES:BX
; ***************************************************************************/
cLeerSectores:
.main
	mov	di, 0x0005		; 5 reintentos antes de dar error
.sector_loop
	push	eax
	push	bx
	push	cx
	call	cLba2Chs
	call	jConvertCilindroSector
	mov	ah, 0x02		; voy a leer
	mov	al, 0x01		; cant de sectores = 1
	mov	dx, word [dwCabeza]
	shl	dx, 8
	mov	dl, byte [dbNroDispositivo]
	int	0x13			; leer!
	jnc	.lectura_ok		; si no hubo carry --> lectura exitosa

	xor	ax, ax
	int	0x13			; si hubo error se resetea la disketera
	dec	di			; se decrementa el contador de intentos
	pop	cx
	pop	bx
	pop	eax
	jnz	.sector_loop		; y si no me pase, reintento la lectura

	int 0x18			; sino bootea --> mejorar manejo de 
					; errores!!

.lectura_ok
	mov	si, strPunto		; imprimir un punto cada vez que se lee
	call cImprimir
	pop	cx
	pop	bx
	pop	eax
	add	bx, word [dwBytesPorSector]	; incrementar el buffer de lectura
	inc	eax			; apuntar al siguiente sector
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
	and	eax, 0x0000FFFF
	xor	cx, cx
	mov	cl, byte [dbSectoresPorCluster]	; AX = AX * dbSectoresPorCluster
	mul	cx
	add	eax, dword [ddSectorInicialDatos]		; AX = AX + ddSectorInicialDatos

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
	push	ebx
	mov	ebx, eax
	shr	ebx, 16
	mov	dx, bx
	div	word [dwSectoresPorPista]
	inc	dx
	mov	word [dwSector], dx

	xor	dx, dx
	div	word [dwNroCabezas]
	mov	word [dwCabeza], dx
	mov	word [dwCilindro], ax
	pop	ebx
	ret

;/*************************************************************************** 
; * cImprimir:
; *    imprime la cadena que se le pasa en SI hasta encontrar un '0'
; ***************************************************************************/
cImprimir:
	lodsb 
	or	al, al
	jz	.end
	mov	ah, 0x0E
	mov	bh, 0x00
	mov	bl, 0x07
	int	0x10
	jmp	cImprimir
.end:
	ret


;/***************************************************************************
; * jConvertCilindroSector
; * Convierte el Cilindro y el Sector para ajustarlo al registro CX
; * IN:  Variables dwCilindro y dwSector
; * OUT: Registro CX con valores correspondientes.
; ***************************************************************************/
jConvertCilindroSector:
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

;/*************************************************************************** 
; * -----------------------------Variables-----------------------------------
; ***************************************************************************/
	; auxiliares que me indican lo que se va leyendo
	dwSector		dw	0x0000
	dwCabeza		dw	0x0000
	dwCilindro		dw	0x0000

	; base (en cant de sectores) de donde comienzan los datos
	ddSectorInicialDatos	dd	0x00000000

	; cluster que se va a leer
	dwCluster		dw	0x0000

	; nombre del kernel (formato 8+3)
	strNombreSO	db	"SODIUM  SYS"

	strMsgCarga	db	"Cargando SODIUM", 0
	strPunto		db	".", 0
	strMsgError	db	 10, 13, "Error", 0

 ; se rellena con ceros hasta 2 bytes antes de terminar el sector
 times 510-($-$$) db 0
 ; los ultimos dos bytes del sector se setean con los valores 0xAA55
 dw 0xAA55
