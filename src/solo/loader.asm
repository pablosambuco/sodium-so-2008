STACK_POINTER		equ     0x7C00
COMIENZO_SODIUM		equ     0x07E0

;CARGA_FAT		equ     0x2300
;DIR_RAIZ		equ	0x0500
;COMIENZO_LIST_OFF	equ	0X0AD7



[bits 16]
org 0x0

	jmp short jComienzo	;salto al inicio del bootstrap propiamente dicho
	nop
	; datos en la BPB para compatibilidad con FAT (12)
	strOemId		db	"Sodium  "
	dwBytesPorSector	dw	0x0200			; cada sector es de 512
								; bytes
	dbSectoresPorCluster	db	0x01			; un sector por cluster
	dwSectoresReservados	dw	0x0001			; un sector para el boot
								; strap
	dbTotalFATs		db	0x02			; la posta y un backup
	dwEntradasRaiz		dw	0x00E0			; 224 entradas en la 
								; tabla raiz
	dwTotalSectores		dw	0x0B40 			; si la cant. de 
								; sectores es menor 
								; a 65535 van aca, sino
								; va cero
	dbDescriptor		db	0xF0			; dbDescriptor del 
								; dispositivo(?)
	dwSectoresPorFat	dw	0x09			; sectores por fat
	dwSectoresPorPista	dw	0x12			; sectores por pista
	dwNroCabezas		dw	0x02			; numero de cabezas
	dbSectoresOcultos	dd	0x00000000		; numero de sectores 
								; ocultos
	ddNroSectores		dd	0x00000000		; cuando los sectores 
								; son mayores a 65535 
								; entonces 
								; dwTotalSectores = 0 
								; y aca va dicho valor
	dbNroDispositivo	db	0x00			; numero de dispositivo 
								; fisico
	dbFlags			db	0x00			; no se usa, reservado
	dbSignature		db	0x29			; firma...
	ddIdVolumen		dd	0x0FFFFFFF		; aca va fruta
	strNombreVolumen	db	"Carga BOOT"
	strTipoFS		db	"FAT12   "

jComienzo:
	cli			; deshabilitar interrupciones
	xor	ax, ax
	mov	ss, ax		; seteo el SS
	mov	sp, STACK_POINTER	; seteo el SP
	sti			; habilitar interrupciones
	push	cs
	pop	ds		; seteo DS
	push	cs
	pop	es		; seteo ES
	push	cs
	pop	fs		; seteo FS
	push	cs
	pop	gs		; seteo GS


   	; borrar la pantalla (se realiza al setear el modo de video)
   	; en particular el modo 0x03 es texto, 25 filas X 80 columnas
	mov		ax, 0x3
	int		0x10

	mov	si, strMsgCarga	; imprimir mensaje
	call 	cImprimir


	;;; Cargamos la FAT y el directorio raiz justo a continuación del loader.
	;;; Las FAT y el directorio raíz se copian uno a continuación del otro
	;;; para poder reutilizarlos con cada archivo.

	; CX = tamanio del directorio raiz (donde se busca el loader)
	; CX = ( dwEntradasRaiz * 32 bytes ) / bytesPorSector
	; Nota: cada entrada tiene 32 bytes
	xor	ax, ax
	xor	dx, dx
	mov	ax, 32
	mul	word [dwEntradasRaiz]
	mov	[dwTamanioDirRaiz], ax
	div	word [dwBytesPorSector]
	mov	cx, ax

	; AX = sector inicial del directorio raiz
	; AX = dbTotalFATs * dwSectoresPorFat + dwSectoresReservados
	mov	al, byte [dbTotalFATs]
	mul	byte [dwSectoresPorFat]
	mov 	bx, ax
	mul	word [dwBytesPorSector]
	mov	[dwDirInicialDirRaiz], ax ;Guarda que todavía falta sumarle el offset del fin del loader
	add	bx, word [dwSectoresReservados]

	; dwSectorInicialDatos = base del directorio raiz + tamanio del directorio raiz
	mov	word [dwSectorInicialDatos], bx
	add	word [dwSectorInicialDatos], cx


	; Se carga en memoria las Tablas FAT y el directorio raíz durante la misma
	; operación de lectura.
	; AX = Sector inicial a partir del que se comienza a leer.
	; BX = Dirección de memoria a donde se vuelcan los sectores leídos.
	; CX = Cant Sectores a Leer = NroFATs * SectoresPorFAT + EntradasDirRaiz * 32 / BytesPorSector

	; Cargamos la fat y el directorio raíz a continuación de la imagen del loader en memoria. 
	; Tambien alineamos a 4 bytes para optimizar el acceso a memoria.
	mov	bx, FinLoader
	add	bx, 0xF
	and	bx, 0xFFF0
	
	add	[dwDirInicialDirRaiz], bx ;es lo que faltaba para terminar de calcular esta dirección

	mov	[dwDirInicialFAT], bx
	mov	ax, [dwSectoresReservados]
	
	mov	cx, [dwSectorInicialDatos]
	sub	cx, [dwSectoresReservados]

	call	cLeerSectores

; ***************************************************
; Procedemos a la carga de los archivos
; Ello se hara con una funcion (cCargarArchivo)
; le enviamos por parametros los archivos a leer
; ***************************************************
	
; 	Cargamos la lista de archivos, a partir de la cual se
;	buscarán los otros archivos a cargar

	;; Mensajito por pantalla
	mov si, strLista
	call cImprimir

	;; Parámetros para "cCargarArchivo"
	push	cs			;; Segmento destino de la carga: Será el mismo donde esta el loader para facilitar operaciones
	mov	ax, [dwDirInicialDirRaiz]
	add	ax, [dwTamanioDirRaiz]
	mov	[dwDirInicialListado], ax
	push	ax	;; Offset destino de la carga, inmediatamente después del directorio raiz
	push	strNombreListado	

	;; Efectuamos la carga	
	call	cCargarArchivo

	;; Verificamos el resultado de cCargarArchivo
	or	ax,ax
	jz	jCargaListaCont

	;; Si no devolvió 0 hubo problemas
	call	cErrorCarga

jCargaListaCont:
	;; Limpiamos el stack
	add	sp,6

	;; Imprimimos OK
	call	cImprimirOk

	;; SI: Puntero para recorrer la lista de archivos (offset)
	;; (El segmento de la lista de archivos será el mismo que el del loader)
	;;
	;; Recordatorio: Estructura de cada entrada de la lista
	;; [NOMBREAR][EXT][SEGMENTO][OFFSET][TAMANIO]
	;;     8       3      2        2        4    (Bytes)
	;;

	mov	si, [dwDirInicialListado]

jProxArchivo:
	push	si
	mov	cx,11
	mov	di,strFinal
	rep	cmpsb				; comparo SI con DI de a un byte	
	pop	si

	je	jFinListaArch			; termino la lista

	;; Mensajito por pantalla
	call	cImprimirNomArch

	;; Salteo por ahora el nombre de archivo y extension
	add	si,11
	
	;; Preparamos los parametros para la carga

	;; SEGMENTO
	mov	ax,[si]
	push 	ax
	add	si,2
	
	;; OFFSET
	mov	ax,[si]
	push 	ax
	add	si,2

	;; NOMBRE
	mov	ax,si
	sub	ax,15	;hace que ax apunte al inicio de la entrada. 11+2+2
	push	ax

	add	si, 4	;salteo el campo tamanio, que se usa más adelante. 
			;Ahora SI ya apunta al inicio del próximo archivo.

	;; Llamamos a cCargarArchivo
	call	cCargarArchivo
	or	ax,ax
	jz	jContCarga

	;; Si no devolvió 0, mensajito por pantalla y error
	push	si
	mov	si,strNOk
	call	cImprimir
	mov	si,strContPreg
	call	cImprimir
	pop	si

	mov 	ah,0
	int 	16h
	cmp 	al,0dh
	jne	jContCargaConError
	call	cErrorCarga

jContCargaConError:
	add	sp,6
	jmp	jProxArchivo
	
jContCarga:
	add	sp,6
	call	cImprimirOk
	jmp	jProxArchivo


jFinListaArch:
	mov si, strCargandoSO
	call cImprimir
	
	mov	ax,COMIENZO_SODIUM
	mov 	ds, ax
	mov 	es, ax
	mov 	fs, ax
	mov 	gs, ax

	jmp 	COMIENZO_SODIUM:0x0000

cErrorCarga:
	mov	si, strMsgError
	call	cImprimir
	xor	ah, ah
	int	0x16
	int	0x19

;/************************************************************************
; * -----------------------------Subrutinas-------------------------------
; ************************************************************************/

;/************************************************************************
; * ----------------------------------CARGA DE LOS ARCHIVOS---------------
; ;	Se le pasan los parámetros mediante el stack. Parametros:
; ;	
; ;	[BP+4] -> Puntero a nombre de archivo (2 bytes)
; ;	[BP+6] -> Offset destino de la carga (2 bytes)
; ;	[BP+8] -> Segmento destino de la carga (2 bytes)
; ;	
; ;	Devuelve 0 en AX si todo salió bien, o -1 si hubo error
;/*************************************************************************

cCargarArchivo:
	
	push 	bp	;Resguardo el Frame Stack Pointer de la funcion llamadora
	mov	bp,sp	;Seteo el Frame Stack Pointer para esta funcion
	;Resguardo de los registros que se utilizaran (debe hacerlo, por convencion), salvo ax 
	; que sera el del resultado (segun convencion)
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	es

	;Nos aseguramos que ES (modificado durante las cargas), vuelva al segmento donde está el dir raiz
	push	cs
	pop	es
	
	; buscar dentro del directorio raiz el nombre del archivo que nos pasaron
	mov	cx, word [dwEntradasRaiz]	; hay [dwEntradasRaiz] entradas 
						; en el directorio
	mov	di, [dwDirInicialDirRaiz]	; se leyo en ES:DIR_RAIZ
jLoop:
	push	cx
	mov	cx, 11				; cada entrada tiene 11 
						; caracteres
	mov	si, [BP+4]			; nombre a comparar con 
						; c/entrada
	push	di				; guardo la posicion inicial
	rep	cmpsb				; comparo SI con DI de a un byte

	pop	di				; recupero DI
	je	jCargarEntrada			; si encontro el archivo lo 
						; cargo
	pop	cx				; si no lo encontro, recupero 
						; CX e incremento DI para que 
	add	di, 0x0020			; apunte a la siguiente entrada
						; en la tabla
	loop	jLoop

	;;; A este punto no se pudo encontrar el archivo en el dir raiz
	
	mov	si,strNOencontro
	call	cImprimir

	pop	es
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx

	mov	ax,-1
	
	mov	sp,bp			; Leave
	pop 	bp

	ret	



; antes de cargar la FAT necesitamos guardar la posicion del archivo

jCargarEntrada:	
	mov	dx, word [di + 0x001A]		; en DI teniamos el inicio de 
						; la entrada en la tabla de 
						; directorios del archivo,
						; y dentro de dicha entrada
						; a un offset de 0x001A esta el
						; cluster inicial del mismo
	mov	word [dwCluster], dx		; que es almacenado en la 
						; variable auxiliar [dwCluster]
	
	mov     es, [BP+8]   ;; Leemos el segmento de destino pasado por parámetro

	mov	bx, [BP+6]   ;; Leemos el valor de Offset que pasamos por parámetro
	push    bx

jSeguirCargar:
	mov	ax, word [dwCluster]		; cluster a leer
	pop	bx				; buffer de lectura para el 
						; cluster

	call	cCluster2Lba			; convertir la direccion en LBA

	xor	cx, cx
	mov	cl, byte [dbSectoresPorCluster]	; cantidad de sectores a leer
	call cLeerSectores

	push bx
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;Para que lea mas de 64kb

	; Si el offset está en 0 significa que debemos incrementar el segmento.
	cmp bx, 0x0000
	jnz continuar

	; La dirección física se formará por la suma desplazada del segmento y del offset.
	; Como ya recorrimos todos los valores del offset, la próxima dirección será la
	; del segmento actual + 1000h.
	; Segmengto          S   S   S   S   -
	; Offset             -   O   O   O   O
	; Suma              -------------------
	; Dirección física:  X   X   X   X   X
	push es
	pop ax
	add ax, 0x1000
	push ax
	pop es
	
	;lo de a continuacion es para que me imprima un * cuando pase los 64kb
	push bx
	mov si, strAsterisco
	call cImprimir
	pop bx

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
continuar:

	; calcular el proximo cluster = 3/2 * cluster_actual
	mov	ax, word [dwCluster]
	mov	bx, word [dwCluster]
	shl	bx, 1
	add	bx, ax
	shr	bx, 1
	add	bx, [dwDirInicialFAT]
	mov	ax, [bx]

	mov	bx, word [dwCluster]
	and	bx, 1
	jz	jClusterPar 
ClusterImpar:
	shr	ax, 4

jClusterPar:
	and	ax, 0000111111111111b		; obtengo los ultimos 12 digitos

	mov	word [dwCluster], ax		; si el cluster nuevo es 0x0FF0
	cmp	ax, 0x0FF0			; llegue al fin de archivo

	jb	jSeguirCargar

	pop gs
	pop gs

	;Devuelvo a los registros los valores anteriores a la funcion
	pop	es
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx

	;Hago el leave (devuelvo el Stack Frame Pointer de la funcion llamadora)
	mov	sp,bp
	pop	bp

jRetornar:
	xor	ax,ax
	ret

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
	push	bx		; Salvo BX para que la función llamadora
				; no pierda su valor
jcImpCont:
	lodsb 
	or	al, al
	jz	jVolver
	
	mov	ah, 0x0E
	mov	bh, 0x00
	mov	bl, 0x07
	int	0x10
	jmp	jcImpCont
jVolver:
	pop	bx
	ret



;/*************************************************************************** 
; * cImprimirNomArch:
; *    imprime el nombre de archivo apuntado por SI en formato DOS 8.3
; ***************************************************************************/
cImprimirNomArch:
	push	dx
	push	cx
	push	bx
	push	si
	push	di

	;; Imprimo como máximo los 8 primeros caracteres
	mov	cx,0x8	
	
	;; Puntero al texto que corresponde poner después del punto
	mov	di,si
	add	di,0x8

	;; Auspicia de flag para indicarme si ya agregué la extensión o no
	xor	dx,dx

	;; Mientras no sea un espacio imprimo caracter a caracter
jCImpNACont:
	lodsb 
	or	al, al
	jz	jCImpNAContVolver
	cmp	al,' '
	jne	jNoPunto

	
jPunto:
	;; Si llegué acá significa que corresponde poner el punto e imprimir la extensión
	mov	cx,0x4
	mov	al,'.'

	;; Recupero el puntero a la extensión
	mov	si,di

	;; Levanto el flag de extensión agregada
	inc	dx
jNoPunto:
	mov	ah, 0x0E
	mov	bh, 0x00
	mov	bl, 0x07
	int	0x10
	loop	jCImpNACont

	;; Si todavía no agregué la extensión, salto a jPunto para hacerlo
	or	dx,dx
	jz	jPunto

jCImpNAContVolver:
	pop	di
	pop	si
	pop	bx	
	pop	cx
	pop	dx
	ret

cImprimirOk:
	push	si

	mov	si,strOk
	call	cImprimir

	pop	si
	ret	

;/*************************************************************************** 
; * -----------------------------Variables-----------------------------------
; ***************************************************************************/
	; auxiliares que me indican lo que se va leyendo
	dbSector		db	0x00
	dbCabeza		db	0x00
	dbPista			db	0x00

	; base (en cant de sectores) de donde comienzan los datos
	dwSectorInicialDatos	dw	0x0000

	; cluster que se va a leer
	dwCluster		dw	0x0000

	dwDirInicialDirRaiz	dw	0x0000
	dwTamanioDirRaiz	dw	0x0000
	dwDirInicialFAT		dw	0x0000
	dwDirInicialListado	dw	0x0000

	strAsterisco	db	"*", 0 
	strMsgCarga	db	"SOLO", 10,13,"====",10,13,"Cargando informacion del disco ",0
	strPunto	db	".", 0
	strNuevaLinea	db	10,13,0
	strMsgError	db	10, 13, "Fallo Cargador", 0
	strOk		db	"[ HECHO ]",10,13,0
	strNOk		db	"[ MAAAL ]",10,13,0

	strNOencontro	db	" Archivo no encontrado. ", 0
	strEncontro	db	"Encontro el archivo", 0
	
	; Nombre de los archivos
	strNombreListado	db	"LISTADO BIN"
	strFinal		db	"           "
	strLista	db	10,13,"Cargando el listado de archivos:", 0

	strCargandoSO	db	"Cargando el SODIUM.SYS expandido", 0
	strContPreg	db	10,13,"El ultimo archivo no se ha cargado correctamente.",10,13,"Presione ENTER para cancelar la carga del SODIUM o cualquier otra tecla para continuar... ",10,13,0 

FinLoader: 
