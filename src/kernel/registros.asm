global wFnGetSS
global wFnGetES
global wFnGetCS
global wFnGetDS
global wFnGetFS
global wFnGetGS
global wFnGetESP
global vFnLTR
global uiFnSTR

;/*global load_page_directory
;**global set_cr0_for_paging*/

;/**
; * wFnGetSS: Funcion que se encarga de obtener el descriptor del segmento de
; * de pila (ss) y devolverlo en eax.
; *
; * @return ss valor del descriptor del segmento de pila
; */
ALIGN 4
wFnGetSS:
	xor	eax, eax
	mov	ax, ss
	ret

;/**
; * wFnGetES: Funcion que se encarga de obtener el descriptor del segmento 
; * auxiliar es y devolverlo en eax.
; *
; * @return es valor del descriptor del segmento de pila
; */
ALIGN 4
wFnGetES:
	xor	eax, eax
	mov	ax, es
	ret

;/**
; * wFnGetCS: Funcion que se encarga de obtener el descriptor del segmento 
; * codigo cs y devolverlo en eax.
; *
; * @return cs valor del descriptor del segmento de codigo
; */
ALIGN 4
wFnGetCS:
	xor	eax, eax
	mov	ax, cs
	ret

;/**
; * wFnGetDS: Funcion que se encarga de obtener el descriptor del segmento 
; * de datos (ds) y devolverlo en eax.
; *
; * @return ds valor del descriptor del segmento de datos
; */
ALIGN 4
wFnGetDS:
	xor	eax, eax
	mov	ax, ds
	ret

;/**
; * wFnGetFS: Funcion que se encarga de obtener el descriptor del segmento 
; * auxiliar fs y devolverlo en eax.
; *
; * @return fs valor del descriptor del segmento de pila
; */
ALIGN 4
wFnGetFS:
	xor	eax, eax
	mov	ax, fs
	ret

;/**
; * wFnGetGS: Funcion que se encarga de obtener el descriptor del segmento 
; * auxiliar gs y devolverlo en eax.
; *
; * @return gs valor del descriptor del segmento de pila
; */
ALIGN 4
wFnGetGS:
	xor	eax, eax
	mov	ax, gs
	ret

;/**
; * wFnGetESP: Funcion que se encarga de obtener el valor del stack pointer.
; *
; * @return esp valor del stack pointer. 
; */
ALIGN 4
wFnGetESP:
	xor	eax, eax
	mov	eax, esp
	ret

	
	
;/**
;*ALIGN 4
;*load_page_directory
;*	push	ebp
;*	mov		ebp, esp
;*
;*	mov		eax, [ ebp + 8 ]
;*	and		eax, 0xfffff000    ;Se supone que esta
;*                                           ;alinea estaria al pedo esto.
;*	mov		cr3, eax
;*
;*	mov		esp, ebp
;*	pop		ebp	
;*ret
;*
;*ALIGN 4
;*set_cr0_for_paging:
;*	mov	eax, cr0
;*	or	eax, 1 << 31
;*	mov	cr0, eax	
;*        jmp	near .near		; just for fun
;*.near
;*ret


ALIGN 4
vFnLTR:
	push eax
	mov eax, [esp + 8]
	ltr ax
	pop eax
ret

ALIGN 4
uiFnSTR:
	xor eax, eax
	str ax
ret

