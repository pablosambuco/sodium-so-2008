global vFnIniciarIDT_Asm
global vFnRemapearPIC_Asm
global vFnIniciarTimer_Asm
global vFnHandlerGenerico_Asm
global vFnHandlerTimer_Asm
global vFnHandlerTeclado_Asm
global vFnExcepcionCPU7_Asm
global vFnExcepcionCPU13_Asm
global vFnExcepcionCPU16_Asm
global vFnHandlerSyscall_Asm

extern vFnHandlerGenerico 	;Definido en system.c
extern vFnHandlerTimer	    ;Definido en system.c
extern vFnHandlerTeclado	;Definido en system.c
extern lFnHandlerSyscall	;Definido en system.c
extern vFnExcepcionCPU7 	;Definido en system.c
extern vFnExcepcionCPU13	;Definido en system.c
extern vFnExcepcionCPU16	;Definido en system.c

;****************************
;* Definicion de constantes *
;****************************
PIC1		equ	0x21	;Direccion Base del PIC1
PIC2		equ	0xA1	;Direccion Base del PIC2
PIC1_IMR	equ	0x21	;Direccion del IMR del PIC1
PIC2_IMR	equ	0xA1	;Direccion del IMR del PIC2
ICW1		equ	00010001b 	;A7,A6,A5=000, D4=1, LTIM=Edge Trigered
					;ADI=Interval of 8, SNGL=Cascade,
					;IC4=Needed
ICW3_PIC1	equ	00000100b	;En la IR2 esta como esclavo el PIC2
ICW3_PIC2	equ	00000010b	;El PIC2 esta en la IR2 del PIC1
ICW4		equ	00000001b	;Not special fully nested mode,
					;Non buffered mode, Normal EOI, 8086
OCW2		equ	0x20		;Comando EOI no especifico

TIMER_1		equ	0x40		;Timer 1
TIMER_CTRL_REG	equ	0x43		;Registro de control
CTRL_WORD	equ	00110100b	;Counter=0, Write LSB and then MSB, Mode=2,
					;BCD=false



;****************************
;* Definicion de MACROS     *
;****************************


;*******************************************************************************
;* MACRO: ENTRADA_STACK_KRNL                                               
;* Descripcion: Esta macro se encarga de manejar el cambio de stack desde 
;		cualquier proceso que ejecute una syscall o rutina de atención
;		que esté definida dentro del código del kernel.
; 	 	El CS se cambia automáticamente al CS del kernel porque se usa
;		el que está definido en el INTERRUPT GATE, pero al no haber
;		cambio en el nivel de privilegio (porque todavía todo corre
;		en nivel 0) el cambio de stack se tiene que hacer manualmente
;               C y activa las interrupciones                               
;* Parametros: Ninguno
;******************************************************************************

%macro	ENTRADA_STACK_KRNL 0
 	push	ds 		;todo esto se guarda en el stack original
 	push	es
 	push	fs
 	push	gs

 	push	edi
 	push	esi

	pushf
	
 	mov	si, ss		;Importante: Aquí tomo el SS y lo guardo en SI
				;como variable auxiliar. Este registro lo mantengo intacto hasta 
				;configurar el nuevo stack, así pusheo dentro de él 
				;el valor original de ss
	
	mov	edi, 0x10
 	mov	ds, di 		;cambiamos todos los selectores de datos
 	mov	es, di 		;para que apunten al segmento de datos del kernel
 	mov	fs, di
 	mov	gs, di

;-------------------------------(Aquí seteamos el nuevo stack del kernel)

 	mov	ss, di	 	;luego de esto ya se trabaja con el stack del kernel
	mov	edi, esp	;aquí guardo temporalmente el valor del ESP del viejo stack
				;para poder volver a él posteriormente.

	mov	esp, 0x200000
	push	eax		;guardamos EAX y EBX porque los vamos a pisar para calcular
	push	ebx		;la posicion del stack

	;Ahora tengo que elegir la posición del stack "descartable" que se usa en el 
	;Espacio de direccionamiento del kernel. Usamos la memoria desde 2MB hacia abajo.
	;El espacio reservado para cada tarea es de 1024bytes, y la posición asignada
	;se calcula usando el TR de cada proceso.
	;Como el TR más bajo es el 0x20 (que apunta a la TSS del shell), lo que hacemos
	;es tomar el TR actual y le restamos 0x20. Con esto obtenemos un entero positivo
	;que toma intervalos de a 8 (los TSS se ubican en la GDT, y cada TSS Descriptor
	;ocupa 8 bytes)

	;El problema es que la GDT tenderá a crecer bastante, y los índices también.. eso 
	;puede causar que el TR sea demasiado alto y que el stack (2mb-256*(TR-20h) comience
	;a pisar la memoria más baja...
	
	;Osea que lo correcto sería usar el indice en la estructura stuTSS...

	mov	eax, 0x100	;Posición del stack: (0x200000 - 0x08) - (256* (TR-0x20)),
	xor	ebx,ebx
	str	bx ;ebx		;Siendo TR múltiplo de 8, me queda un stack de 1024 bytes
	sub	ebx, 0x20	;para cada proceso.
	mul	bx
	mov	ebx, 0x200000-0x08
	sub	ebx, eax

 	mov	esp, ebx	;pasamos el valor calculado a ESP

 	push	esi     	;aquí apilamos, en el stack nuevo, el SS del stack 
				;original
 	push	edi
	push	ebp
	mov	ebp, ebx
	
	mov	eax, [0x200000-4] ;rescatamos "manualmente" eax y ebx del stack anterior
	mov	ebx, [0x200000-8] ;(antes de calcular el definitivo)
				
;-------------------------------(a partir de ahora pusheamos en el stack del kernel)
%endmacro ;FIN ENTRADA_STACK_KRNL


%macro SALIDA_STACK_KRNL 0
;-------------------------------(Aquí restauramos el stack del proceso)
	pop	ebp
 	pop	edi    		;aquí restauramos el stack pointer original
 	pop	esi		;y el SS original
	mov	ss, esi
 	mov	esp, edi
;-------------------------------(hasta aquí era el stack del kernel)

 	popf

 	pop	esi		;Esto ya se restaura del stack original!
 	pop	edi
	
 	pop	gs
 	pop	fs
 	pop	es
 	pop	ds
%endmacro ;FIN SALIDA_STACK_KRNL





;****************************************************************************
;* Funcion: vFnIniciarIDT_Asm                                                      *
;* Descripcion: Esta funcion carga en el IDTR los datos de la IDT creada en *
;*              C y activa las interrupciones                               *
;****************************************************************************
align 4
vFnIniciarIDT_Asm:
	push	ebp
	mov	ebp, esp
	push	eax  ;Salvo el contenido de eax

	mov	ax, [ebp + 8]
	mov	word [dwIDTLimite], ax
	mov	eax, [ebp + 12]
	mov	dword [ddIDTBase], eax

	lidt	[IDT]
	
	pop	eax  ;Recupero el contenido de eax
	mov	esp, ebp              ;restauro el esp
	pop	ebp                   ;recupero el ebp
	ret


;****************************************************************************
;* Funcion: vFnRemapearPIC_Asm                                                      *
;* Descripcion: Esta funcion remapea el PIC de interrupciones para que las  *
;*              IRQ esten a partir de la int 0x20                           *
;****************************************************************************
vFnRemapearPIC_Asm:
	push	ebp
	mov	ebp, esp

	;Cambio las bases de los PICs
	;PIC1: de 0x08 a 0x0F --> 0x20 a 0x27
	;PIC2: de 0x70 a 0x77 --> 0x28 a 0x2F

	;PIC1
	mov 	al, ICW1
	out 	PIC1 - 1, al
	call 	cDelay8259
	mov 	al, 0x20	;Nueva base de intr para PIC1
	out 	PIC1, al
	call 	cDelay8259
	mov 	al, ICW3_PIC1
	out 	PIC1, al
	call 	cDelay8259
	mov 	al, ICW4
	out 	PIC1, al

	;PIC2
	mov 	al, ICW1
	out 	PIC2 - 1, al
	call 	cDelay8259
	mov 	al, 0x28	;Nueva base de intr para PIC2
	out 	PIC2, al
	call 	cDelay8259
	mov 	al, ICW3_PIC2
	out 	PIC2, al
	call 	cDelay8259
	mov 	al, ICW4
	out 	PIC2, al

	;Habilito Interrupcion del timer (int 0x20)
	;Habilito Interrupcion del teclado (int 0x21)
	mov 	al, 0xFC	;Mascara de enmascaramiento de interrupciones
	out 	PIC1, al

	mov	esp, ebp              ;restauro el esp
	pop	ebp                   ;recupero el ebp
	ret

cDelay8259:
	nop
	nop
	nop
	nop
	ret

;****************************************************************************
;* Funcion: vFnIniciarTimer_Asm                                                    *
;* Descripcion: Esta funcion programa el timer para que trabaje en modo 2   *
;*              (rate generator)                                            *
;****************************************************************************
vFnIniciarTimer_Asm:
	push	ebp
	mov	ebp, esp

	push	eax
	push	ebx

	mov	al, CTRL_WORD
	out	TIMER_CTRL_REG, al
	call 	cDelay8254

	mov	bx, [ebp + 8]
	mov	al, bl  			; Envia el byte
	out	TIMER_1, al		; menos significativo

	mov	al, bh			; Envia el byte
	out	TIMER_1, al		; más significativo

	pop	ebx
	pop	eax

	mov	esp, ebp              ;restauro el esp
	pop	ebp                   ;recupero el ebp
	ret


cDelay8254:
	nop
	nop
	nop
	nop
	ret


;****************************************************************************
;* Funcion: vFnHandlerGenerico_Asm                                             *
;* Descripcion: Handler generico para las interrupciones que llama al       *
;*              handler generico en C. Esta ISR se interpone en la llamada  *
;*              a la ISR de C debido a que cuando se retorna desde una      *
;*              ISR es necesario retornar con IRET, y el GCC no dispone de  *
;*              esta facilidad.                                             *
;* Parametros: Ninguno						    *
;* Valor devuelto: Ninguno						    *
;* Ultima Modificacion: 1-10-2003					    *
;****************************************************************************
vFnHandlerGenerico_Asm:

	pushad
	
	ENTRADA_STACK_KRNL

	;Llamada al Handler generico en C
	call	vFnHandlerGenerico

	;Limpio el PIC para que se pueda volver a disparar
	mov	eax, 0x20
	out	PIC1 - 1, al

	SALIDA_STACK_KRNL

	popad

	iret

;****************************************************************************
;* Funcion: vFnHandlerTimer_Asm                                                *
;* Descripcion: Handler para el manejo del Timer que llama al               *
;*              handler del timer en C. Esta ISR se interpone en la llamada *
;*              a la ISR de C debido a que cuando se retorna desde una      *
;*              ISR es necesario retornar con IRET, y el GCC no dispone de  *
;*              esta facilidad.                                             *
;* Parametros: Ninguno							    *
;* Valor devuelto: Ninguno						    *
;* Ultima Modificacion: 9-11-2006					    *
;****************************************************************************
vFnHandlerTimer_Asm:

	pushad

	ENTRADA_STACK_KRNL	

	;Limpio el PIC para que se pueda volver a disparar. En este caso 
	;PRIMERO limpio el PIC, porque si no lo hago antes de hacer el context
	;switch, después no lo hace nadie!
	;De todos modos no hace mucho daño en el sentido que igualmente
	;SEGURO que no vamos a recibir una interrupción, porque hasta
	;que no ejecutemos el iret, el IF sigue bajo.

	;Importante: ¿Y como se siguen ejecutando las interrupciones de reloj
	;	si no hacemos iret?
	;	Fácil: cuando se cambia el contexto a otra tarea, en ella
	;	seguro que el flag IF está o estará alto en breve, y cuando
	;	nos toque ejecutar a nosotros nuevamente, vamos a hacer
	;	iret exclusivamente para nuestro contexto.
	
 	mov	eax, 0x20
	out	PIC1 - 1, al

	call vFnHandlerTimer

	;NADIE VA A LLEGAR HASTA ACÁ HASTA QUE SE HAYAN ATENDIDO A TODOS LOS
	;DEMAS PROCESOS DE LA COLA DE LISTOS!!!, SALVO QUE SÓLO 
	;HAYA UN PROCESO ACTIVO Y NO SE HAGA UN CONTEXT SWITCH, EN ESE CASO
	;SEGUIMOS NOSOTROS... POR ESO ES IMPORTANTE QUE NADIE NOS DESTRUYA
	;EL STACK HASTA QUE LO DESCARTEMOS NOSOTROS... (VER COMENTARIOS EN
	;LA MACRO ENTRADA_STACK_KRNL)
	SALIDA_STACK_KRNL
	
	popad
	
	iret


;****************************************************************************
;* Funcion: vFnHandlerTeclado_Asm                                             *
;* Descripcion: Handler para el manejo del teclado que llama al handler     *
;*              del teclado en C. Esta ISR se interpone en la llamada       *
;*              a la ISR de C debido a que cuando se retorna desde una      *
;*              ISR es necesario retornar con IRET, y el GCC no dispone de  *
;*              esta facilidad.                                             *
;* Parametros: Ninguno							    *
;* Valor devuelto: Ninguno						    *
;* Ultima Modificacion: 8-11-2006					    *
;****************************************************************************
vFnHandlerTeclado_Asm:

	pushad
	
	ENTRADA_STACK_KRNL

	call vFnHandlerTeclado
	
	;Limpio el PIC para que se pueda volver a disparar
	mov	eax, 0x20
	out	PIC1 - 1, al
	
	SALIDA_STACK_KRNL

	popad
	
	iret


;****************************************************************************
;* Funcion: vFnExcepcionCPU7_Asm                                            *
;* Descripcion: Handler para el manejo de la excepcion 7 (Coprocesador)     *
;*              Ya que es ISR es necesario retornar con IRET, y el GCC no   *
;*		        dispone de esta facilidad.                                  *
;* Parametros: Ninguno							                            *
;* Valor devuelto: Ninguno						                            *
;* Ultima Modificacion: 11-07-2008	    			                        *
;****************************************************************************
vFnExcepcionCPU7_Asm:

	pushad
	
	ENTRADA_STACK_KRNL

	call vFnExcepcionCPU7

	SALIDA_STACK_KRNL

	popad
	
	iret


;****************************************************************************
;* Funcion: vFnExcepcionCPU13_Asm                                           *
;* Descripcion: Handler para el manejo de la excepcion 13 (Error de         *
;*              Proteccion General (Triple Fault)                           *
;*              Ya que es ISR es necesario retornar con IRET, y el GCC no   *
;*              dispone de esta facilidad.                                  *
;* Parametros: Ninguno							                            *
;* Valor devuelto: Ninguno						                            *
;* Ultima Modificacion: 07-11-2008					                        *
;****************************************************************************
vFnExcepcionCPU13_Asm:

    add esp, 0x04           ;Se recupera el codigo de error (sera 0 o el indice
                            ;en la GDT de la TSS del proceso que fallo)
    pushad

    ENTRADA_STACK_KRNL
    
    call vFnExcepcionCPU13

    SALIDA_STACK_KRNL

    popad

    iret


;****************************************************************************
;* Funcion: vFnExcepcionCPU16_Asm                                           *
;* Descripcion: Handler para el manejo de la excepcion 16 (Coprocesador)    *
;*              Ya que es ISR es necesario retornar con IRET, y el GCC no   *
;*		        dispone de esta facilidad.                                  *
;* Parametros: Ninguno							                            *
;* Valor devuelto: Ninguno						                            *
;* Ultima Modificacion: 11-07-2008					                        *
;****************************************************************************
vFnExcepcionCPU16_Asm:

	pushad
	
	ENTRADA_STACK_KRNL

    call vFnExcepcionCPU16

	SALIDA_STACK_KRNL

	popad
	
	iret


;****************************************************************************
;* Funcion: vFnHandlerSysCalls_Asm					    *
;****************************************************************************
;* Funcion: vFnHandlerSysCalls_Asm					    *
;* Descripcion: Handler en ASM que llama al handler generico en C.	    *
;* Esta ISR se interpone en la llamada  				    *
;*              a la ISR de C debido a que cuando se retorna desde una      *
;*              ISR es necesario retornar con IRET, y el GCC no dispone de  *
;*              esta facilidad.                                             *
;* Parametros: Ninguno							    *
;* Valor devuelto: Ninguno						    *
;* Ultima Modificacion: 8-11-2006					    *
;****************************************************************************
vFnHandlerSyscall_Asm:

	ENTRADA_STACK_KRNL

	push	edx	;estos vienen a ser los parámetros de la syscall,
	push	ecx	;al menos de prueba, realmente van más
	push	ebx
	push	eax

	call lFnHandlerSyscall

	;Prestar atención que el handler en C nos devuelve el resultado 
	;de la syscall en eax! 

	add	esp, 16		;con esto balanceo el stack, por 4 los registros
				;que mandamos con anterioridad: 
				;y así respetamos la convención CDECL

	SALIDA_STACK_KRNL

	iret


;**********************
;* Defincion del IDTR *
;**********************

IDT
        dwIDTLimite	dw	0x0000
	ddIDTBase	dd	0x00000000

