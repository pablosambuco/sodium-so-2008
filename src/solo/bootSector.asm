STACK_POINTER		equ     0x7C00
CARGA_FAT		equ     0x0500
COMIENZO_LOADER		equ     0x9000

[bits 16]
org 0x7C00

	jmp short jComienzo	;salto al inicio del bootstrap propiamente dicho
	nop
	; datos en la BPB para compatibilidad con FAT (12)
	strOemId				db	"Sodium  "
	dwBytesPorSector				dw	0x0200			; cada sector es de 512
											; bytes
	dbSectoresPorCluster	db	0x01			; un sector por cluster
	dwSectoresReservados	dw	0x0001			; un sector para el boot
											; strap
	dbTotalFATs			db	0x02			; la posta y un backup
	dwEntradasRaiz		dw	0x00E0			; 224 entradas en la 
											; tabla raiz
	dwTotalSectores		dw	0x0B40 			; si la cant. de 
											; sectores es menor 
											; a 65535 van aca, sino
   											; va cero
	dbDescriptor			db	0xF0			; dbDescriptor del 
											; dispositivo(?)
	dwSectoresPorFat		dw	0x09			; sectores por fat
	dwSectoresPorPista	dw	0x12			; sectores por pista
	dwNroCabezas			dw	0x02			; numero de cabezas
	dbSectoresOcultos		dd	0x00000000		; numero de sectores 
											; ocultos
	ddNroSectores		dd	0x00000000		; cuando los sectores 
											; son mayores a 65535 
											; entonces 
											; dwTotalSectores = 0 
											; y aca va dicho valor
	dbNroDispositivo				db	0x00			; numero de dispositivo 
											; fisico
	dbFlags				db	0x00			; no se usa, reservado
	dbSignature			db	0x29			; firma...
	ddIdVolumen			dd	0x0FFFFFFF		; aca va fruta
	strNombreVolumen		db	"SODIUM BOOT"
	strTipoFS				db	"FAT12   "

jComienzo:
	cli						; deshabilitar interrupciones
	xor	ax, ax
	mov	ss, ax				; seteo el SS

	mov	sp, STACK_POINTER	; seteo el SP
	jmp	0x0:cs_en_0
cs_en_0:
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

	mov	si, strMsgCarga	; imprimir mensaje
	call cImprimir

Cargar:
	; CX = tamanio del directorio raiz (donde se busca el loader)
	; CX = ( dwEntradasRaiz * 32 bytes ) / bytesPorSector
	; Nota: cada entrada tiene 32 bytes
	xor	ax, ax
	xor	dx, dx
	mov	ax, 32
	mul	word [dwEntradasRaiz]
	div	word [dwBytesPorSector]
	xchg	ax, cx

	; AX = direccion del directorio raiz
	; AX = dbTotalFATs * dwSectoresPorFat + dwSectoresReservados
	mov	al, byte [dbTotalFATs]
	mul	byte [dwSectoresPorFat]
	add	ax, word [dwSectoresReservados]

	; dwSectorInicialDatos = base del directorio raiz + tamanio del directorio raiz
	; dwSectorInicialDatos = sector inicial de los datos
	mov	word [dwSectorInicialDatos], ax
	add	word [dwSectorInicialDatos], cx

	; leer el directorio raiz en ES:CARGA_FAT = 0x0000:CARGA_FAT
	mov	bx, CARGA_FAT
	call	cLeerSectores

	; buscar dentro del directorio raiz el nombre del kernel
	mov	cx, word [dwEntradasRaiz]		; hay [dwEntradasRaiz] entradas 
						; en el directorio
	mov	di, CARGA_FAT			; se leyo en ES:CARGA_FAT
.jLoop:
	push	cx
	mov	cx, 0x000B			; cada entrada tiene 11 
						; caracteres
	mov	si, strSodiumLoader		; nombre a comparar con 
						; c/entrada
	push	di				; guardo la posicion inicial
	rep	cmpsb				; comparo SI con DI de a un byte

	pop	di				; recupero DI
	je	jCargarFAT			; si encontro el archivo lo 
						; cargo
	pop	cx				; si no lo encontro, recupero 
						; CX e incremento DI para que 
	add	di, 0x0020			; apunte a la siguiente entrada
						; en la tabla
	loop	.jLoop

	call	cErrorCarga		; Mostramos un mensaje de error y damos la 
					; oportunidad a la bios de probar otros 
					; dispositivos de booteo

	jmp	$			; si llegue hasta aca es que la bios no intercept??			; la int 19h


; antes de cargar la FAT necesitamos guardar la posicion del archivo
jCargarFAT:
	mov	dx, word [di + 0x001A]		; en DI teniamos el inicio de 
						; la entrada en la tabla de 
						; directorios del loader.sys
						; y dentro de dicha entrada
						; a un offset de 0x001A esta el
						; cluster inicial del mismo
	mov	word [dwCluster], dx		; que es almacenado en la 
						; variable auxiliar [dwCluster]

	; ahora si podemos proceder a cargar la FAT en memoria
	; CX = cantidad total de sectores de la FAT (cant de sectores a leer)
	xor	ax, ax
	mov	al, byte [dbTotalFATs]
	mul	word [dwSectoresPorFat]
	mov	cx, ax

	; se calcula la ubicacion de la FAT y se guarda en AX
	mov	ax, word [dwSectoresReservados]

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

jCargarKernel:
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
	mov	ax, word [dwCluster]

	shl	bx, 1
	add	bx, ax
	shr	bx, 1
	and	ax, 1
	mov	ax, [CARGA_FAT + BX]
	jz	.jClusterPar 
.ClusterImpar:
	shr	ax, 4
.jClusterPar:
	and	ax, 0000111111111111b		; obtengo los ultimos 12 bits

	mov	word [dwCluster], ax		; si el cluster nuevo es 0x0FF0
	cmp	ax, 0x0FF0			; llegue al fin de archivo

	jb	jCargarKernel

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

cErrorCarga:
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
jBuclePrincipal:
	mov	di, 0x0005		; 5 reintentos antes de dar error
jIntentoLectura:
	push	ax
	push	bx
	push	cx
	call cLba2Chs

	mov	ah, 0x02		; voy a leer
	mov	al, 0x01		; cant de sectores = 1
	mov	ch, byte [dbPista]
	mov	cl, byte [dbSector]
	mov	dh, byte [dbCabeza]
	mov	dl, byte [dbNroDispositivo]
	int	0x13			; leer!
	jnc	jLecturaOk		; si no hubo carry --> lectura exitosa

	xor	ax, ax
	int	0x13			; si hubo error se resetea la disketera
	dec	di			; se decrementa el contador de intentos
	pop	cx
	pop	bx
	pop	ax
	jnz	jIntentoLectura		; y si no me pase, reintento la lectura

	int 0x18			; sino bootea --> mejorar manejo de 
					; errores!!

jLecturaOk:
	mov	si, strPunto		; imprimir un punto cada vez que se lee
	call	cImprimir
	pop	cx
	pop	bx
	pop	ax
	add	bx, word [dwBytesPorSector]	; incrementar el buffer de lectura
	inc	ax			; apuntar al siguiente sector
	loop	jBuclePrincipal		; leer el siguiente sector

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
	mov	cl, byte [dbSectoresPorCluster]	; AX = AX * dbSectoresPorCluster
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
	xor	dx, dx
	div	word [dwSectoresPorPista]
	inc	dl
	mov	byte [dbSector], dl

	xor	dx, dx
	div	word [dwNroCabezas]
	mov	byte [dbCabeza], dl
	mov	byte [dbPista], al

	ret

;/*************************************************************************** 
; * cImprimir:
; *    imprime la cadena que se le pasa en SI hasta encontrar un '0'
; ***************************************************************************/
cImprimir:
	lodsb 
	or	al, al
	jz	jVolver
	mov	ah, 0x0E
	mov	bh, 0x00
	mov	bl, 0x07
	int	0x10
	jmp	cImprimir
jVolver:
	ret

;/*************************************************************************** 
; * -----------------------------Variables-----------------------------------
; ***************************************************************************/
	; auxiliares que me indican lo que se va leyendo
	dbSector		db	0x00
	dbCabeza		db	0x00
	dbPista		db	0x00

	; base (en cant de sectores) de donde comienzan los datos
	dwSectorInicialDatos	dw	0x0000

	; cluster que se va a leer
	dwCluster		dw	0x0000

	; nombre del kernel (formato 8+3)
	strSodiumLoader	db	"LOADER  SYS"

	strMsgCarga	db	"Cargando el SOLO", 0
	strPunto		db	".", 0
	strMsgError	db	 10, 13, "Error", 0

 ; se rellena con ceros hasta 2 bytes antes de terminar el sector
 times 510-($-$$) db 0
 ; los ultimos dos bytes del sector se setean con los valores 0xAA55
 dw 0xAA55
