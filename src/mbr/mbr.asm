; defincion de constantes:
MBR_EN_MEM		equ	0x7C00
MBR_AUX_AREA		equ	0x0600
STR_AUX_AREA		equ	0x0500

BEGIN_PT                equ     0x01BE
PART_BOOT_FLAG_OFFSET   equ     0x00
PART_CHS_BEGIN_OFFSET   equ     0x01
PART_ID_OFFSET          equ     0x04
PART_CHS_END_OFFSET     equ     0x05
PART_LBA_BEGIN_OFFSET   equ     0x08
PART_SIZE_OFFSET        equ     0x0C
PT_ENTRY_SIZE           equ     0x10

ASCII_1			equ	0x31
ASCII_4			equ	0x34
ASCII_SPC		equ	0x20

INT_13_RETRIES		equ	0x0005
BOOT_SIGN_OFFSET	equ	0x1FE
BOOT_SIGNATURE		equ	0xAA55
	
[bits 16]
;Por compatibilidad con versiones mas antiguas de nasm, se cambia el uso de la etiqueta por un valor inmediato
org	0x600;MBR_AUX_AREA		; para que calcule las posiciones relativas
				; a esta posicion (0x0600) en lugar de (0x7C00)
	
start:
	cli                  
	xor     ax, ax         
	mov     ss, ax
	mov     sp, MBR_EN_MEM       
	mov     si, sp         
	push    ax
	pop     es            
	push    ax
	pop     ds            
	sti                   

	cld                   

	; teniendo seteado si en la posicion original del MBR,
	; apuntamos di a la posicion auxiliar para hacer el
	; traslado
	mov     di, MBR_AUX_AREA      
	mov     cx, 0x0100	; copiar 256 words (512 bytes) 
	repnz   movsw

	; hacemos el salto a la copia:
	; NOTA: si no ponemos base 0x0000 salta a cualquier lado!!!
	; (hay que ver el tema de setear CS)
	jmp     0x0000:new_location
	
new_location: 
	; borrar la pantalla (se realiza al setear el modo de video)
        ; en particular el modo 0x03 es texto, 25 filas X 80 columnas
	mov	ax, 0x03
	int	0x10
	
choose:
	mov	si, strMensajeSeleccion
	call	print_string

	mov	di, MBR_AUX_AREA + BEGIN_PT
	mov	cx, 0x01	; inicializamos el contador

.print.entry:
	mov	si, STR_AUX_AREA
	; cargamos el contador:
	mov	ax, cx
	call	byte_2_str
	; cargamos el flag de particion activa:
	mov	al, byte [di]
	call	byte_2_str
	; cargamos el id de la particion:
	mov	al, byte [di+PART_ID_OFFSET]
	call	byte_2_str	
	; imprimos la linea:
	mov	si, STR_AUX_AREA
	call	print_string
.print.type.desc:
	; esto se puede optimizar poniendo en SI el string
	; que corresponde y saltando directo a .print.type
	cmp	byte [di+PART_ID_OFFSET], 0x04
	je	.print.type.04
	cmp	byte [di+PART_ID_OFFSET], 0x06
	je	.print.type.06
	cmp	byte [di+PART_ID_OFFSET], 0x13
	je	.print.type.13
	cmp	byte [di+PART_ID_OFFSET], 0x16
	je	.print.type.16
	jmp	.print.unknown
.print.type.04:
	mov	si, strPartType04
	jmp	.print.type
.print.type.06:
	mov	si, strPartType06
	jmp	.print.type
.print.type.13:
	mov	si, strPartType13
	jmp	.print.type
.print.type.16:
	mov	si, strPartType16
	jmp	.print.type
.print.unknown:
	mov	si, strPartTypeUnknown
.print.type:
	call	print_string
.new.line:
	; mandamos new line:
	mov	si, strNewLine
	call	print_string

	; saltamos a la siguiente entrada...
	add	di, PT_ENTRY_SIZE

	inc	cx
	cmp	cx, 0x04
	; ...si no estamos parados en la ultima
	jna	.print.entry	
	
read_key:
	; leemos del teclado	
	xor	ax, ax
	int	0x16
	xor	ah, ah
	; vemos que se haya ingresado 1-4:
	cmp	al, ASCII_1
	jb	read_key	; menor a '1', de vuelta...
	cmp	al, ASCII_4
	ja	read_key	; mayor a '4', de vuelta...

	; calculamos el offset dentro de la tabla de particiones,
	; haciendo: SI = (TECLA-31h)*PT_ENTRY_SIZE
	sub	ax, ASCII_1
	; esto se puede optimizar haciendo un shift de 4 en lugar
	; de multiplicar por 16
	mov	bx, PT_ENTRY_SIZE
	mul	bx
	mov	si, ax

read_pbr:	
	; ahora le agregamos como base el inicio de la tabla de particiones:
	add     si, MBR_AUX_AREA + BEGIN_PT 
	; cargamos DX y CX para la INT 13h, y guardamos SI para
	; recuperarlo despues	
	;mov     dx, [si]
	; esto es para que ignore las particiones que no estan marcadas como activas
	mov	dl, 0x80
	mov	dh, byte [si+01]
	mov     cx, [si+02]      
	mov     bp, si           

	; cargamos la cantidad de reintentos
	mov     di, INT_13_RETRIES  
	
.retry.int13: 
	mov     bx, MBR_EN_MEM	; destino 
	mov	ah, 0x02	; servicio 0x02 (leer sectores)
	mov	al, 0x01	; leer un sector
	push    di             
	int     0x13              
	pop     di              
	jnb     check_sig        
	xor     ax, ax		; servicio 0x00 (resetear el dispositivo)
	int     0x13		 
	dec     di              ; un intento menos
	jnz     .retry.int13
	mov     si, strErrorCarga 
	jmp     0x0000:show_error_retry 
	
check_sig:
	mov     si, strFirmaInvalida
	mov     di, MBR_EN_MEM + BOOT_SIGN_OFFSET
	cmp     word [di], BOOT_SIGNATURE 
	jnz     show_error_retry

do_boot:
	mov     si, bp              	; restauramos SI
	jmp     0x0000:MBR_EN_MEM	; saltamos al PBR cargado, con SI apuntando
					; a la entrada correspodiente en la tabla 
					; de particiones

; /** 
;  * rutina para imprimir una cadena: 
;  */
print_string:            
	lodsb                   ; sacamos un byte de DS:SI y lo cargamos
				; en AL
	cmp     al, 0x00        ; chequeamos fin de string
	jz      .end
	push    si 
	mov     bx, 0x0007	; seteamos modo 
	mov     ah, 0x0e	; servicio 0x0E (imprimir un byte)
	int     0x10 
	pop     si 
	jmp     0x0000:print_string 
.end:   
	ret

nibble_2_ascii:
        cmp     al, 0x0a
        jge     .asciiA
        add     al, 0x30
        jmp     .end
.asciiA:
        sub     al, 0x0a
        add     al, 0x61
.end:
        ret

byte_2_str:
	; esto se puede optimizar haciendo que nibble_2_ascii guarde
	; directo en [SI]
        mov     ah, al		; guardo el byte original
        shr     al, 4		; tomo el nibble alto
        and     al, 0x0f
        call 	nibble_2_ascii
        mov     byte [si], al	; lo guardamos en donde este si
	inc	si		; movemos si a la siguiente posicion
	xchg	ah, al		; recuperamos el original
        and     ax, 0x0f	; tomo el nibble bajo
        call	nibble_2_ascii
        mov     byte [si], al	
	inc	si
	; agregamos un espacio:
	mov	byte [si], ASCII_SPC
	inc	si
        ret


; /* errores */ 	
show_error_retry:
	call	print_string
	jmp	read_key

; /**
;  * mensajes:
;  */
	strMensajeSeleccion	db	"Seleccione la particion: (1-4)", 10, 13, \
					"N  Ac Id", 10, 13, 0
	strFirmaInvalida	db	"ERROR: Firma invalida", 10, 13, 0
	strErrorCarga		db	"ERROR: No se puede cargar el PBR", 10, 13, 0

        strPartType04    	db	"FAT16.1", 0 ;"DOS 3.0+ 16-bit FAT"
	strPartType06		db	"FAT16", 0 ;"DOS 3.31+ 16-bit FAT"
	strPartType13		db	"BkpMBR", 0 ;"BackupMBR"
	strPartType16		db	"Sodium", 0 ;"SODIUM (Hidden DOS 16-bit FAT >=32M)"
	strPartTypeUnknown	db	"Unknown", 0 ;"SODIUM (Hidden DOS 16-bit FAT >=32M)"

	strNewLine		db	10, 13, 0

	; rellenamos hasta completar los primeros 446 bytes
	; (sirve para chequear que no ocupe mas que eso asi no 
	; pisamos la tabla de particiones)
	times 444-($-$$) db 0	; tenemos q completar 446 bytes, asi que a ese numero le restamos
				; lo que ya usamos y los 2 bytes del magic number a continuacion, y el
				; resultado lo llenamos con ceros
	dw	0xDED0		; magic number para reconocer nuestro MBR
