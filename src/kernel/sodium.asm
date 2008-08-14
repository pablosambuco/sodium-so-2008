                               ; Indic          G-L   DPL
SELECTOR_CODIGO   equ   0x08   ; 0000 0000 0000 1  0  00
SELECTOR_DATOS    equ   0x10   ; 0000 0000 0001 0  0  00
SELECTOR_PILA     equ   0x10   ; 0000 0000 0001 0  0  00

;/************************************************************************
; * GDTBASE: macro que calcula la direccion base de la GDT en base a los
; *          tamanios del segmento BSS y del nucleo.  Es importante
; *          que dicha tabla este ALINEADA a 4K! para lo cual se le suman
; *          esos cuatro KB y luego con un AND logico se eliminan las
; *          ultimas 12 posiciones (2^12 = 4K)
; *
; * gracias Frank Cornelis!!!
; *
; ************************************************************************/
bits 16
org INSTALADOR_SIZE

%define GDTBASE ((KERNEL_SIZE + BSS_SIZE + 4096 - 1) & 0xFFFFF000)
%define IDTBASE (GDTBASE + 65536)

%macro ESCRIBIR_VARIABLE_KERNEL 2
	push	dword %1		;dirección de la variable
	push	dword %2		;valor a copiar
	call	cCopiarAVariableKernel
	add	sp, 8
%endmacro

%macro LEER_VARIABLE_KERNEL 1
	push	dword %1		;dirección de la variable
	call	cLeerVariableKernel	;el valor se devuelve en eax
	add	sp, 4
%endmacro


Inicio:
	push	cs		; DS = CS
	pop	ds

InfoMemoria:	
	; int 0x12 obtiene la memoria base (por si no son 640 KB)
	int	0x12
	movzx	eax, ax	
	shl	eax, 0xA		; eax *= 1024 //para expresarla en bytes
	; se almacena en una variable (declarada en main.c) el valor
	; obtenido

	;mov	[dword LOW_MEMORY_SIZE_ADDR + kernel], eax
	ESCRIBIR_VARIABLE_KERNEL LOW_MEMORY_SIZE_ADDR, eax

	; El servicio 0x88 de la int 15 nos da cuanta memoria tiene
	; la PC (en KB) -> memoria extendida
	; Primero se setea en cero por si no es posible obtener dicho dato

	;mov	dword [dword BIOS_MEMORY_SIZE_ADDR + kernel], 0
	ESCRIBIR_VARIABLE_KERNEL BIOS_MEMORY_SIZE_ADDR, 0

	mov	ah,88h
        int	15h
	jc	jNoSoportado
		
	; se multiplica por 1024 para expresarla en bytes
	movzx	eax, ax	
	shl	eax,0xA	         ; multiplica por 1024
	add	eax,0x100000     ; suma 1 mb
	; se almacena el valor en la variable correspondiente
	; (declarada en main.c)

	;mov	[dword BIOS_MEMORY_SIZE_ADDR + kernel], eax
	ESCRIBIR_VARIABLE_KERNEL BIOS_MEMORY_SIZE_ADDR, eax

jNoSoportado
	cli			; deshabilitar interrupciones


	mov 	cx,0
	mov 	ax,0b800h
	mov 	es,ax
	mov 	di,0
	mov 	ax,03
	int 	10h    

	mov	ax,1720h
	call 	cLimpiarPantalla

;Menu de ejecucion.

jSwitch1:     
	call 	cImprimirRecuadro
	mov   si, strEjeTitulo
	call 	cImprimirTitulo
	mov 	si, strMLiveSelecc
	call 	cImprimirOpcion1
	mov 	si, strMInstNoSelecc
	call 	cImprimirOpcion2
	
	mov 	ah,0
	int 	16h
	cmp 	al,0dh
	je 	jOpcionLive
Switch2:
	call 	cImprimirRecuadro
	mov   si, strEjeTitulo
	call 	cImprimirTitulo
	mov 	si, strMLiveNoSelecc
	call 	cImprimirOpcion1
	mov 	si, strMInstSelecc
	call 	cImprimirOpcion2
	mov 	ah,0
	int 	16h
	cmp 	al,0dh
	je 	jOpcionInst
	jmp 	jSwitch1

;Menu Asignacion de memoria

jOpcionLive:
	mov	ax,1720h
	call 	cLimpiarPantalla
jSwitch3:     
	call 	cImprimirRecuadro
	mov   si, strMemTitulo
	call 	cImprimirTitulo
	mov 	si, strMPagiSelecc
	call 	cImprimirOpcion1
	mov 	si, strMSegmNoSelecc
	call 	cImprimirOpcion2
	
	mov 	ah,0
	int 	16h
	cmp 	al,0dh
	je 	jOpcModoPaginado
Switch4:
	call 	cImprimirRecuadro
	mov   si, strMemTitulo
	call 	cImprimirTitulo
	mov 	si, strMPagiNoSelecc
	call 	cImprimirOpcion1
	mov 	si, strMSegmSelecc
	call 	cImprimirOpcion2
	mov 	ah,0
	int 	16h
	cmp 	al,0dh
	je 	jOpcModoSegmentado
	jmp 	jSwitch3

;Menu Instalar/Desinstalar

jOpcionInst:
	mov	ax,1720h
	call 	cLimpiarPantalla
jSobreInstalar:     
	call 	cImprimirRecuadro
	mov   si, strMemTitulo
	call 	cImprimirTitulo
	mov 	si, strMInsSelecc
	call 	cImprimirOpcion1
	mov 	si, strMDesNoSelecc
	call 	cImprimirOpcion2
	
	mov 	ah,0
	int 	16h
	cmp 	al,0dh
	je 	jOpcInstalar
SobreDesinstalar:
	call 	cImprimirRecuadro
	call 	cImprimirRecuadro
	mov   si, strMemTitulo
	mov 	si, strMInsNoSelecc
	call 	cImprimirOpcion1
	mov 	si, strMDesSelecc
	call 	cImprimirOpcion2
	mov 	ah,0
	int 	16h
	cmp 	al,0dh
	je 	jOpcDesinstalar
	jmp	jSobreInstalar

jOpcModoPaginado:
	;mov	dword [dword MEMORY_MODE_ADDR + kernel], 1
	ESCRIBIR_VARIABLE_KERNEL MEMORY_MODE_ADDR, 1
	jmp	jFinOpciones

jOpcModoSegmentado:
	;mov	dword [dword MEMORY_MODE_ADDR + kernel], 0
	ESCRIBIR_VARIABLE_KERNEL MEMORY_MODE_ADDR, 0
	jmp	jFinOpciones

jOpcInstalar:
	mov     ax,1720h
	call    cLimpiarPantalla 
	mov     si, strInicioInstalacion
	call    cImprimir
	mov	ax, 0xf1f1
	jmp     Inicio - INSTALADOR_SIZE

jOpcDesinstalar:
	mov     ax,1720h
	call    cLimpiarPantalla 
	mov     si, strInicioDesinstalacion
	call    cImprimir
	mov	ax, 0xf2f2
	jmp     Inicio - INSTALADOR_SIZE

jFinOpciones:
;-------------------------------------------------------------------------------

        mov     eax, cr0                ;Toma el registro de control 0
        and     eax, 0xFFFFFFFB         ;EM = 0
        or      eax, 0x00000022         ;NE = 1, MP = 1
        mov     cr0, eax                ;Guarda el registro de control 0

        fninit                          ;Iniciliza la FPU	
        fnstcw  [esp-4]
        mov     eax, dword[esp-4]
        and     eax, 0xFFC0             ;Se desenmascaran las excepciones
        or      eax, 0x0020             ;Omitir excepciones por precision
        mov     [esp-4], eax            ;de la FPU
        fldcw   [esp-4]

;-------------------------------------------------------------------------------
	mov	ax,0020h
	call cLimpiarPantalla
	

; habilitar la compuerta A20 (gracias Linus Torvalds!)
; extraido de linux version 0.0.1
	call    cEmpty8042
	mov     al,0D1h
	out     64h,al
	call    cEmpty8042
	mov     al,0DFh
	out     60h,al
	call    cEmpty8042

; setear el stack (al final de la memoria base)
	LEER_VARIABLE_KERNEL LOW_MEMORY_SIZE_ADDR
	mov	[ddStackPointer], eax

	push	cs			; DS = CS
	pop	ds	

	push	si
	mov	si, strBienvenida
	call    cImprimir
	pop	si

; se almacenan en las variables declaradas en el kernel
; los respectivos tamanios del kernel y el segmento BSS

	;mov	dword [dword KERNEL_SIZE_ADDR + kernel], KERNEL_SIZE
	;mov	dword [dword BSS_SIZE_ADDR + kernel], BSS_SIZE
	;mov	dword [dword IDT_ADDR + kernel], IDTBASE
	;mov	dword [dword GDT_ADDR + kernel], GDTBASE
	
	ESCRIBIR_VARIABLE_KERNEL 	KERNEL_SIZE_ADDR,	KERNEL_SIZE
	ESCRIBIR_VARIABLE_KERNEL 	BSS_SIZE_ADDR,		BSS_SIZE
	ESCRIBIR_VARIABLE_KERNEL 	IDT_ADDR, 		IDTBASE
	ESCRIBIR_VARIABLE_KERNEL 	GDT_ADDR, 		GDTBASE
	
; se copia el codigo del loader desde 0x07E0:0x0000 a 0x9000:0x0000
	mov	ax, 0x9000
	mov	es, ax			; ES = 0x9000
	xor	di, di			; DI = SI = 0x0
	xor	si, si
	mov	cx, kernel + 3		; tamanio del loader
					; (desde el BOF hasta el label kernel)
	shr	cx, 2			; como se hace un movsd (DOUBLE)
					; se divide el tamanio del loader
					; por cuatro -> 1 double = 4 bytes
	cld				; se pone a cero el bit D
	rep	movsd			; se realiza la copia!

; se setea un stack temporal en 0x7000:0xFFFF
	mov	ax, 0x7000
	mov	ss, ax
	mov	sp, 0xFFFF

	; hasta aca tenemos el codigo en 0x9000:0x0000
	; y una pila seteada, entonces al hacer un jmp se va a setear
	; el segmento CS con dicha direccion (comenzando a ejecutar
	; el codigo recien copiado ahi, a partir de la etiqueta jSegundaEtapa)
	jmp	0x9000:jSegundaEtapa

jSegundaEtapa:

; se copia a partir del label kernel (donde comienza el codigo C de main.c)
; al comienzo de la memoria (direccion 0)
; OJO! DS todavia apunta al segmento viejo!
	mov	esi, kernel			; origen = ESI = kernel
	xor	edi, edi			; destino = EDI = 0x0
	mov	es, di				;se va a copiar a 0x0:0x0

	mov ecx, (KERNEL_SIZE + BSS_SIZE)


;A continuación se copia de a un byte por vez, de DS:SI a ES:DI.
;En modo real no es posible copiar mediante el prefijo rep más de 65536 veces,
;independientemente del uso del prefijo a32. Por ello, para copiar un bloque 
;de memoria mayor a 64kb, o al copiar un bloque que en algún momento 
;trasciende los límites del segmento origen o destino, es necesario 
jCopiarByte: 
;Por compatibilidad con versiones antiguas de nasm, que no generan el opcode correcto para saltar a una instruccion con boundary de 2 bytes al usar a32, se alinea la primer instruccion del bucle a 4 bytes
align 4
	movsb ;copiando el kernel de a un byte por ciclo!!!
	cmp si, 0x0000 ;verifico si SI se paso de los 64Kb
	jnz jComprobarDI ;si no se paso, salto a chequear por el DI (que indica destino)

	;Se incrementa el selector de segmento DS para que apunte a los próximos 64kb
	push ds 
	pop ax
	add ax, 0x1000 
	push ax
	pop ds 
	
jComprobarDI: ;chequeamos ahora que el DI no se pase de 64kb
	cmp di, 0x0000
	jnz jContinuar ;si no se paso, salto a continuar copiando

	;Se incrementa el selector de segmento DS para que apunte a los próximos 64kb
	push es 
	pop ax
	add ax, 0x1000 
	push ax
	pop es 
	
jContinuar:
a32	loop jCopiarByte
		       ;salto al comienzo del bucle, para continuar copiando el kernel
		       ; (use a32 pues le indico que estoy trabajando con ECX en vez de CX)
		       ; el loop decrementa automaticamente ECX/CX y sale cuando CX sea 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		
	; ahora que ya efectuamos el movimiento, podemos setear DS con el
	; valor correcto (0x9000) de donde esta el loader
	push	cs
	pop	ds

        ; se va a setear la GDT!

	; primero se limpian los 64Kb que va a ocupar (llenando con ceros)
	mov	eax, GDTBASE		; obtengo la direccion de la GDT
					; OJO! es la direccion fisica
	shr	eax, 4			; para accederla desde modo real
					; divido por 0x10 = 16
					; ya que al acceder al segmento el
					; hardware lo multiplica por ese
					; valor, transformando una direccion
					; lineal en fisica o real:
					; DIR LINEAL = DIR REAL / 16
	mov	es, ax			; ES = direccion lineal de la GDT
	xor	di, di			; La copia es desde el inicio
	xor	eax, eax		; Se llena con ceros
	mov	cx, 65536 / 4		; Se mueven de a 4 bytes = 1 double por
					; vez
	rep	stosd			; Se procede!

	; ahora se mueve la GDT declarada abajo hasta la ubicacion "posta"
	xor	di, di			; desde la posicion 0 de la GDT
					; (ES todavia esta seteado con la
					; direccion lineal de la GDT!)
	mov	si, inicio_gdt		; source = SI = inicio de la GDT que
					; se declaro abajo

	; tamanio de la GDT (expresado en doubles = 4 bytes)
	mov	cx, (fin_gdt - inicio_gdt + 3) / 4
	rep	movsd			; se procede a moverla!

	; y por fin la cargamos en memoria!
	lgdt	[GDT]

	; ahora que tenemos una GDT posta pasamos a PM
	mov	eax, cr0
	or	eax, 1
	mov	cr0, eax

	; para setear el CS con el selector que queremos debemos hacer
	; un far jmp indicando dicho selector
	; como el selector que usamos para codigo tiene su base en 0x0
	; y previamente movimos el codigo del loader a 0x90000
	; entonces debemos sumarle ese valor a la etiqueta a la cual
	; vamos a efectuar el salto.  Asimismo, el valor que se ubica
	; antes de los ':' es el desplazamiento dentro de la
	; tabla de descriptores de segmento (GDT) expresado en bytes
	jmp 	dword SELECTOR_CODIGO:(jModoProtegido + 0x90000)

; ya estamos en 32 bits!!!!!
BITS 32		
jModoProtegido	
        ; se carga una LDT nula
	xor	ax, ax	
	lldt	ax

	mov	ax, SELECTOR_DATOS
	mov	ds, ax			; DS = selector de datos
	mov	es, ax			; ES = DS
	mov	fs, ax			; FS = DS
	mov	gs, ax			; GS = DS

	mov	ax, SELECTOR_PILA	; se setea el segmento de pila
	mov	ss, ax

	; en la variable ddStackPointer habiamos seteado a donde debia
	; apuntar el ESP, que es al final de la memoria base
	; (nuevamente, el segmento de codigo comienza en 0x0 por lo que
	; debemos usar el offset 0x90000 para referirnos a las variables
	; declaradas en el loader)
	mov	esp, [ddStackPointer + 0x90000]

	; bit reservado por intel (siempre 1)
	push	dword 0x2
	popfd

	; se hace un far jmp al codigo del kernel (main!)
	jmp	dword SELECTOR_CODIGO:MAIN

; /********************************************************************** 
;  * SUBRUTINAS								*
;  **********************************************************************/ 

BITS 16		; las rutinas utilizadas aca se llaman desde modo real!


; /********************************************************************** 
;  * cCopiarAVariableKernel: A medida que el sodium crece, las variables
;  * globales se van alejando de 0x7e00 :P. Hoy, superaron la barrera del 
;  * segmento, así que vamos a buscarlas al infinito y más allá...
;  * Recibe a través del stack: 
;  *	- (dw) Offset de la variable dentro del main.bin (ubicado a partir
;  * de la etiqueta kernel
;  *    - (dw) Valor a copiar
;  **********************************************************************/

cCopiarAVariableKernel:

	push 	bp
	mov 	bp, sp
	push 	eax
	push	ebx
	push	edx
	push	es
	
	;primero calculamos a qué segmento nos tenemos que dirigir, que
	;ya no será el actual...
	;Veo cuántos segmentos enteros (64kb) hacen falta para 
	;llegar. El resto (las monedas) son el offset.

	xor	eax, eax
	xor	ebx, ebx
	xor	edx, edx

	;Tomamos el parámetro del stack que representa la posición de la
	;variable a escribir dentro del main.bin y le sumamos la posición
	;inicial de main.bin (dentro del segmento actual!)
	mov	ebx, dword [bp+8]
	add	ebx, dword kernel
	
	mov	eax, ebx
	shr	eax, 16
	;en ax está la parte alta de ebx, que es la que excede los 64kb
	mov 	dx, 0x1000
	mul	dx	

	mov	dx, cs
	add	ax, dx
	mov	es, ax
	
	mov	edx, [bp+4]

	mov	[es:bx],edx

	pop	es
	pop	edx
	pop	ebx
	pop	eax
	mov	sp, bp
	pop	bp
ret

cLeerVariableKernel:

	push 	bp
	mov 	bp, sp
	push	ebx
	push	edx
	push	es
	
	;primero calculamos a qué segmento nos tenemos que dirigir, que
	;ya no será el actual...
	;Veo cuántos segmentos enteros (64kb) hacen falta para 
	;llegar. El resto (las monedas) son el offset.

	xor	eax, eax
	xor	ebx, ebx
	xor	edx, edx

	;Tomamos el parámetro del stack que representa la posición de la
	;variable a leer dentro del main.bin y le sumamos la posición
	;inicial de main.bin (dentro del segmento actual!)
	mov	ebx, dword [bp+4]
	add	ebx, dword kernel
	
	mov	eax, ebx
	shr	eax, 16
	;en ax está la parte alta de ebx, que es la que excede los 64kb
	mov 	dx, 0x1000
	mul	dx	

	mov	dx, cs ;uso cs de referencia porque se mantiene siempre en la tierra!
			;ds y es se empiezan a mover cuando reubicamos al kernel.
	add	ax, dx
	mov	es, ax
	
	mov	eax, [es:bx]

	pop	es
	pop	edx
	pop	ebx
	mov	sp, bp
	pop	bp
ret



; /********************************************************************** 
;  * cDelay: utilizada para habilitar la compuerta A20
;  * (gracias Linus!)
;  **********************************************************************/
cDelay 
	jmp jNext
jNext
ret

; /********************************************************************** 
;  * Empty8024: utilizada para habilitar la compuerta A20
;  * (gracias Linus!)
;  * Lo que hace es leer el teclado hasta recibir una senial de
;  * reconocimiento por parte del 8042, esto es para que el mismmo no
;  * quede en un estado "intestable"  esperando algun comando o datos.
;  * Muchas gracias Linus!!!!!!!!!!! (sacado de la version 0.0.1 de linux)
;  **********************************************************************/
cEmpty8042
       	call    cDelay
       	in      al, 64h
       	test    al, 1
       	jz      jNoOutput
       	call    cDelay
       	in      al, 60h
       	jmp     cEmpty8042
  jNoOutput
       	test    al, 2
       	jnz     cEmpty8042
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

	
	
;Nombre:	cLimpiarPantalla
;Parametros: 	ah=Caracter
;		al=color
;Descripción:	Rutina para cubrir la pantalla (memoria de video) de un determinado chr y color
cLimpiarPantalla:
	pusha
	mov	bx,0xb800
	mov	es,bx
	xor	edi,edi
	mov	cx,2000
	rep	stosw
	popa
ret



;Nombre:	cImprimirRecuadro
;Parametros: 	bl=Posicion x
;		bh=Longitud x
;		dl=Posicion y
;		dh=Longitud y
;Descripción:	Rutina para dibujar un recuadro en la pantalla (memoria de video) de un determinado chr y color
;Hardcodeado a 	chr(al): espacio 	//0x20h
;		color(ah): verde 	//0x20h
;IMPORTANTE:	Es responsabilidad del programador asignar correctamente los valores correspondientes a los registros
;		utilizados como parámetro. Esta rutina no controla que el recuadro exceda el framebuffer (80x25).

cImprimirRecuadro:
	enter 0,0
	pusha
	mov	ax,0b800h
	mov	es,ax
	xor	di,di
		
	mov	dl,8		;HEIGHT= 7 char.
	mov	di,160 * 4	;TOP = 4 char. 80 caracteres definidos con 2 bytes cada uno
	add	di,  2 * 20	;LEFT = 20 char. multiplico porque cada char se define por 2 bytes

jImprimirNuevaLinea:
	push 	di
	mov	cx,40		;WIDTH= 40 char.
	mov	al,20h		;caracter: 	espacio
	mov	ah,20h		;color: 	verde
	rep	stosw
	pop 	di
	
	add	di,160		;salto a la próxima línea
	
	dec	dl	
	jnz	jImprimirNuevaLinea
	popa
	leave
	ret

cImprimirTitulo:
	enter 0,0
	
	pusha
	mov	dx,si
	mov	ax,0b800h
	mov	es,ax
	xor	di,di
		
	mov	di, 160 * 5	;TOP = 8 char. 80 caracteres definidos con 2 bytes cada uno
	add	di, 2 * 25	;LEFT = 25 char. multiplico porque cada char se define por 2 bytes
jRepetirTitulo:
	mov	al,[si]		;caracter del string
	mov	ah,20h		;color: Byte alto: color texto, Byte bajo: color fondo 	
	mov	[es:di],ax
	inc	si
	add	di,0x2	
	cmp	byte [si],0x0
	jne	jRepetirTitulo	
	popa
	leave
	ret

cImprimirOpcion1:
	enter 0,0
	
	pusha
	mov	dx,si
	mov	ax,0b800h
	mov	es,ax
	xor	di,di
		
	mov	di, 160 * 7	;TOP = 8 char. 80 caracteres definidos con 2 bytes cada uno
	add	di, 2 * 25	;LEFT = 25 char. multiplico porque cada char se define por 2 bytes
jRepetir:
	mov	al,[si]		;caracter del string
	mov	ah,03h		;color: Byte alto: color texto, Byte bajo: color fondo 	
	mov	[es:di],ax
	inc	si
	add	di,0x2	
	cmp	byte [si],0x0
	jne	jRepetir	
	popa
	leave
	ret
	
cImprimirOpcion2:
	enter 0,0
	
	pusha
	mov	dx,si
	mov	ax,0b800h
	mov	es,ax
	xor	di,di
		
	mov	di, 160 * 9	;TOP = 8 char. 80 caracteres definidos con 2 bytes cada uno
	add	di, 2 * 25	;LEFT = 25 char. multiplico porque cada char se define por 2 bytes
jRepetir2:
	mov	al,[si]		;caracter del string
	mov	ah,03h		;color: Byte alto: color texto, Byte bajo: color fondo 	
	mov	[es:di],ax
	inc	si
	add	di,0x2
	cmp	byte [si],0
	jne	jRepetir2
	popa
	leave
	ret



; /********************************************************************** 
;  * FINAL SUBRUTINAS							*
;  **********************************************************************/ 

; definicion de la GDT
inicio_gdt:	
ddDescriptorNulo	dd 0x0, 0x0

;Codigo: LI = 00000000h, LS = FFFFFFFFh
dwDescrCodigo	dw	0xFFFF	  ;lim = xFFFFh
		dw	0x0000	  ;base = xxxx0000h
		db	0x00 	  ;base = xx00xxxxh
		db	10011010b ;Presente | Privilegio 0 | Sistema no | Tipo codigo x/r
		db	11001111b ;Granularidad gruesa | Default 32bits | lim = Fxxxxh
		db	0x00	  ;base = 00xxxxxxh

;Datos: LI = 00000000h, LS = FFFFFFFFh
dwDescrDatos	dw	0xFFFF	  ;lim = xFFFFh
		dw	0x0000	  ;base = xxxx0000h
		db	0x00 	  ;base = xx00xxxxh
		db	10010010b ;Presente | Privilegio 0 | Sistema no | Tipo datos r/w
		db	11001111b ;Granularidad gruesa | Default 32bits | lim = Fxxxxh
		db	0x00	  ;base = 00xxxxxxh	
	
fin_gdt:	


; /********************************************************************** 
;  * declaracion de variables usadas por sodium.asm                     *
;  **********************************************************************/ 
strBienvenida   db  10, 13, "INICIANDO SODIUM...",10,13,0
strInicioInstalacion  db  10, 13, "INICIANDO INSTALACION...",10,13,0
strInicioDesinstalacion db  10, 13, "INICIANDO DESINSTALACION...",10,13,0
strBorrarLinea    db  "                              ",0

strEjeTitulo      db  "        Modo ejecucion:       ",0
strMLiveSelecc    db  "    ->       Live!      <-    ",0
strMLiveNoSelecc  db  "             Live!            ",0
strMInstSelecc    db  "    ->    Instalacion   <-    ",0
strMInstNoSelecc  db  "          Instalacion         ",0



strMemTitulo      db  "    Asignacion de Memoria:    ",0
strMPagiSelecc    db  "    ->   Modo Paginado  <-    ",0
strMPagiNoSelecc  db  "         Modo Paginado        ",0
strMSegmSelecc    db  "    ->  Modo Segmentado <-    ",0
strMSegmNoSelecc  db  "        Modo Segmentado       ",0

strInsTitulo      db  "            Opcion:           ",0
strMInsSelecc     db  "    -> Instalar SODIUM  <-    ",0
strMInsNoSelecc   db  "       Instalar SODIUM        ",0
strMDesSelecc     db  "    ->Desinstalar SODIUM<-    ",0
strMDesNoSelecc   db  "      Desinstalar SODIUM      ",0

GDT
	dwGDTLimit	dw 	0xFFFF 	; GDT limit = maximum	
	ddGDTBase	dd 	GDTBASE	; GDT base	

IDT
	dwIDTLimite	dw	0x07ff
	ddIDTBase	dd	IDTBASE

ddStackPointer		dd 	0

; /********************************************************************** 
;  * fin declaracion de variables usadas por sodium.asm                 *
;  **********************************************************************/ 

bits 32

kernel:			; aca comienza lo mejor!!!
