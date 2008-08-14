global inb
global inw
global outb
global outw

;/**
; * inb: Esta funcion lee un byte desde un puerto que se pasa como parametro
; *
; * @param puerto puerto a ser leido.
; *
; * @return resultado_lectura Devuelve un byte del resultado de la lectura en
; *                           el puerto.
; */
align 4
inb:
	push	ebp
	mov	ebp, esp

	push	edx
	mov	edx, [ebp+8]   ;recupero el primer y unico argumento
	xor	eax, eax
	xor	al, al
	in	al, dx         ;realizo la operacion de lectura sobre el 
	                       ;puerto seleccionado
	pop	edx

	mov	esp, ebp       ;restauro el esp
	pop	ebp            ;recupero el ebp
	ret

;/**
; * inw: Esta funcion lee un word desde un puerto que se pasa como parametro
; *
; * @param puerto puerto a ser leido.
; *
; * @return resultado_lectura Devuelve un word del resultado de la lectura en
; *                           el puerto.
; */
align 4
inw:
	push	ebp
	mov	ebp, esp

	push	edx
	mov	edx, [ebp+8]   ;recupero el primer y unico argumento
	xor	eax, eax
	in	ax, dx         ;realizo la operacion de lectura sobre el 
	                       ;puerto seleccionado
	pop	edx

	mov	esp, ebp       ;restauro el esp
	pop	ebp            ;recupero el ebp
	ret

;/**
; * outb: Esta funcion escribe un byte que se pasa como parametro a un puerto
; * que se pasa tambien como parametro
; *
; * @param valor byte que se le va a escribir al puerto.
; *
; * @param puerto puerto a ser leido.
; */
align 4
outb:
	push	ebp
	mov	ebp, esp

	push	edx

	mov	eax, [ebp+8] 		; recupero el primer argumento (valor)
	mov	edx, [ebp+12]		; recupero el segundo argumento (puerto)

	out	dx, al                ;realizo la operacion de escritura sobre
                                      ;el puerto seleccionado
	pop	edx

	mov	esp, ebp              ;restauro el esp
	pop	ebp                   ;recupero el ebp
	ret

;/**
; * outw: Esta funcion escribe un word que se pasa como parametro a un puerto
; * que se pasa tambien como parametro
; *
; * @param valor word que se le va a escribir al puerto.
; *
; * @param puerto puerto a ser leido.
; */
align 4
outw:
	push	ebp
	mov	ebp, esp

	push	edx

	mov	eax, [ebp+8]          ;recupero el primer argumento (valor)
	mov	edx, [ebp+12]         ;recupero el segundo argumento (puerto)

	out	dx, ax                ;realizo la operacion de escritura sobre
	                              ;el puerto seleccionado

	pop	edx

	mov	esp, ebp              ;restauro el esp
	pop	ebp                   ;recupero el ebp
	ret
