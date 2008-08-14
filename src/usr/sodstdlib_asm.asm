;ATENCION: Este archivo esta basado en kernel/libk/libk_asm.asm
;Cualquier modificacion que se haga en este archivo debe ser tenida en cuenta
;para ser incluida en el archivo original mencionado y viceversa.
;
;Se opto por aniadir el sufijo _usr a las funciones que aqui figuran para
;diferenciarlas de las disponibles a nivel kernel. Esta decision no se
;relaciona con ninguna limitacion tecnica, solo se hace para evitar
;confusiones a quienes las utilicen.
;
;Puede que muchas de las funciones y partes del codigo de hallen comentadas.
;Esto no quiere decir que no funcionen o sean inapropiadas, es solo que al
;momento de generar este archivo no se tuvo el tiempo necesario para probarlas.

GLOBAL vFnFtoa_usr
GLOBAL fFnAtof_usr

; Funcion: vFnFtoa
; Parametros:
;    Buffer (char*) donde grabar la cadena (512 bytes)
;    Valor a convertir (double, 8 bytes)
;    Cifras decimales (max 198)
;
; Bugs conocidos:
;    - La precision en la parte entera llega aproximadamente a la cifra 15.
;      Tiene un error del orden del 0.0000000000005 %
;      (5 x 10 ^ 2 - cifras_correctas)
;    - Redondeo diferente que sprinft (no es un bug, ya que lo hace mejor :)
;      sprintf a veces redodea hacia arriba y a veces no :?
;           ejemplo: 3.625 (a dos decimales)
;           sprintf:    3.62
;           vFnFtoa:    3.63

vFnFtoa_usr:
    push    ebp
    mov     ebp, esp
    sub     esp, 36             ;Reservo espacio para variables (4 bytes c/u)

    push    ebx                 ;Se apilan todos los registros a usar
    push    ecx
    push    edx

    fnstcw  [ebp-4]             ;Cambio del metodo de redondeo del FPU 
    mov     eax, dword[ebp-4]   ;guardandolo antes en el stack.
    push    eax                 ;   00 = Round to nearest whole number(default)
    or      eax, 0x0C20         ;   01 = Round down, toward -infinity. 
                                ;   10 = Round up, toward +infinity.
                                ;-> 11 = Round toward zero (truncate).
                                ;Tambien deshabilito las interrupciones por
                                ;perdida de precision
    mov     [ebp-4], eax        
    fldcw   [ebp-4]

reserva1:
   ;mov     dword[ebp-4],0      ;Valor absoluto (Parte alta)
   ;mov     dword[ebp-8],0      ;Valor absoluto (Parte baja)
    mov     dword[ebp-12],1000000000    ;Auxiliar1 (P/ dividir por milmillones)
    mov     dword[ebp-16],10            ;Auxiliar2 (P/ dividir por diez)
   ;mov     dword[ebp-20],0     ;AuxiliarFP (Parte alta)
   ;mov     dword[ebp-24],0     ;AuxiliarFP (Parte baja)
    mov     dword[ebp-28],0     ;AcumuladoFP (Parte alta)
    mov     dword[ebp-32],0     ;AcumuladoFP (Parte baja)
    mov     dword[ebp-36],0     ;Signo (en ASCII, 0 para no imprimir nada)

    mov     ebx, [ebp+8]        ;Param: Puntero al buffer de salida -> EBX

signo1:
    mov     eax, [ebp+16]       ;Param: Numero a convertir (HI)-> EAX
    and     eax, 0x80000000     ;Se testea si es positivo o negativo
    jz      positivo
negativo:
    mov     dword[ebp-36],'-'
    jmp     valor
positivo:
    mov     dword[ebp-36], 0

valor:
    fld     qword[ebp+12]       ;Carga el valor en la FPU 
    fldz                        ;Carga CERO en la FPU
    fucompp                     ;Compara el valor con cero
    fnstsw  ax                  
    sahf                        ;Copia los flags de la FPU en EFLAGS
    je      evaluar
    jmp     numero

evaluar:
    mov     eax, [ebp+16]
    and     eax, 0x7FF00000
    cmp     eax, 0x7FF00000
    jne     numero_cero
nan:
    mov     [ebx], dword "nan"
    add     ebx,3
    jmp     fin_cadena

numero_cero:                    ;Podria hacerse un tratamiento especial para 0
numero:

imprimir_signo1:
    mov     eax, dword[ebp-36]  ;Primero se imprime el signo1
    cmp     eax, 0
    je      listo_signo1
    mov     [ebx], al
    inc     ebx   
listo_signo1:

    fld     qword[ebp+12]       ;Calcula el valor absoluto 
    fabs

; Rutina de redondeo alternativa
; Funciona sumando 5 * 10^-(precision_pedida + 1)
; Por temas de redondeo, tiene peores resultados que la rutina usada actualmente
;
;redondeo:
;    mov     dword[ebp-20], 5
;    fild    dword[ebp-20]
;    mov     ecx,[ebp+20]        ;Param. Cantidad de decimales
;    inc     ecx
;calculo_suma_redondeo:
;    fidiv   dword[ebp-16]
;    loop    calculo_suma_redondeo
;    faddp   st1, st0

    fstp    qword[ebp-8]        ;Se guarda el valor absoluto en memoria

exponente:
    mov     eax, [ebp-4]        ;(Parte alta del valor absoluto)
    sar     eax, 20             ;Calcula el exponente 
    sub     eax, 1023

limites:
    cmp     eax, 1024           ;Evalua si el exponente es >= 1024 ????
    jge     infinito            ;y lo considera infinito

    cmp     eax,0               ;Verifica si es mayor a 1
    jge     parte_entera1

    mov     byte[ebx], '0'
    inc     ebx
    jmp     parte_decimal1

infinito:
    mov     [ebx], dword "inf"
    add     ebx,3
    jmp     fin_cadena

parte_entera1:

    mov     ecx, 0
    fld     qword[ebp-8]i       ;Se carga el valor absoluto

apilarmilesdemillones:

    fst     qword[ebp-24]       ;Se apilan los resultados de ir dividiend1o por
    push    dword[ebp-24]       ;1.000.000.000 para separar el numero en bloques
    push    dword[ebp-20]       ;de 9 cifras las cuales entran en regs de 32bits
                                ;Lo ultimo en apilarse seran las cifras mas
                                ;significativas
    inc     ecx

    fidiv   dword[ebp-12]

    fld1                        ;Carga UNO en la FPU
    fucomp  st1                 ;Compara el valor con UNO
    fnstsw  ax                  
;    fwait
    sahf                        ;Copia los flags de la FPU en EFLAGS
    jbe     apilarmilesdemillones

    fistp   dword[ebp-24]       ;Solo para limpiar la FPU

desapilarmilesdemillones_init:

    pop     dword[ebp-20]       ;Se carga el ultimo valor apilado (cifras mas
    pop     dword[ebp-24]       ;significativas)

    fld     qword[ebp-24]
    frndint                     ;Se calcula la parte entera
    fist    dword[ebp-24]

    mov     eax, [ebp-24]
    call    imprimir_entero     ;Y se convierte

    dec     ecx
    jz      listo_parte_entera1

desapilarmilesdemillones:

    fld     qword[ebp-32]       ;Se lleva la cuenta que valores se imprimieron
    faddp   st1, st0            ;acumulandolos
    fimul   dword[ebp-12]       ;y multiplicandolos por 1.000.000.000
    fst     qword[ebp-32]

    pop     dword[ebp-20]
    pop     dword[ebp-24]
    fld     qword[ebp-24]       ;Se desapila otro valor

    fxch
    fsubp   st1, st0            ;Se le resta el valor ya impreso
    frndint

    fild    dword[ebp-12]       ;Y si el valor NO es menor a 1.000.000.000
    fucomp  st1                 ;significa que ya los valores estaran fuera de
    fnstsw  ax                  ;la precision que maneja este algoritmo
    sahf
    jb      precision_perdida

    fist    dword[ebp-24]       ;Si no, se imprimen las 9 cifras
    mov     eax, [ebp-24]
    call    imprimir_entero_9_cifras

    loop    desapilarmilesdemillones    ;Tantas veces como sea necesario

listo_parte_entera1:
    fistp   dword[ebp-24]       ;Solo para limpiar la FPU
    jmp     parte_decimal1


precision_perdida:              ;Si se perdio la precision, no importa que
                                ;cifras se impriman. Elegimos imprimir un 5
                                ;seguido de tantos ceros como correspondan, para
                                ;que el valor este en el medio de los valores
                                ;posibles
    mov     eax, 500000000
    call    imprimir_entero_9_cifras
    mov     eax, 0
    dec     ecx
    jz      decimales_precision_perdida
loop_precision_perdida:
    pop     edx 
    pop     edx 
    call    imprimir_entero_9_cifras
    loop    loop_precision_perdida
decimales_precision_perdida:
    mov     byte[ebx], '.'
    inc     ebx
    mov     ecx,[ebp+20]        ;Param. Cantidad de decimales
    cmp     ecx, 0
    je      listo_loop_decimales_precision_perdida
loop_decimales_precision_perdida:
    mov     byte[ebx], '0'
    inc     ebx
    loop    loop_decimales_precision_perdida
listo_loop_decimales_precision_perdida:

    fistp   dword[ebp-24]       ;Solo para limpiar la FPU
    jmp     fin_cadena


parte_decimal1:

    mov     ecx,[ebp+20]        ;Param. Cantidad de decimales
    inc     ecx

    mov     [ebx], byte'.'      ;Imprimo el punto decimal
    inc     ebx

    fld     qword[ebp-8]
    fist    dword[ebp-24]

imprimir_decimales:
    fild    dword[ebp-24]
    fsubp   st1, st0

    fimul   dword[ebp-16]       ;Se va multiplicando por 10 la parte decimal
    fist    dword[ebp-24]
    mov     eax, [ebp-24]
    add     eax,'0'             ;Convierto el valor a ascii

    mov     [ebx],eax           ;Guardo el valor en el
    inc     ebx                 ;buffer
    loop    imprimir_decimales  ;Y se repite la cantidad de decimales pedida

    fistp   dword[ebp-24]       ;Solo para limpiar la FPU


    dec     ebx                 ;Se carga el ultimo decimal calculado
    mov     al, byte[ebx]       ;(solo para redondear)
redondeo:                       ;Recorro la cadena armada redondeando
    inc     ecx
    dec     ebx
    cmp     al,'5'              ;Si es menor de 5, termine
    jb      fin_redondeo
    mov     al,byte[ebx]
    cmp     al,'.'              ;Si llega al punto decimal, incremento la parte
    je      rehacer_enteros     ;entera, debo re-hacer la parte entera
    inc     al
    cmp     al,'9'              ;Si es 9 o menos, termina
    jle     ultimo_redondeo
    mov     al,'0'
    mov     byte[ebx],al        ;Guarda el valor redondeado
    mov     al,'9'              ;Carga en EAX un valor como bandera
    jmp     redondeo            ;Si redondea a cero, continua
ultimo_redondeo
    mov     byte[ebx],al
fin_redondeo                    ;Vuelve a acomodar el puntero del buffer
    inc     ebx
    loop    fin_redondeo
    jmp     fin_cadena


rehacer_enteros:
    fld     qword[ebp+12]       ;Carga el valor en la FPU
    fabs
    fld1                        ;Se le suma 1
    faddp   st1, st0
    frndint                     ;Y la parte decimal sera = 0
    fstp    qword[ebp+12]
    mov     ebx, [ebp+8]        ;Param: Puntero al buffer de salida -> EBX
    jmp     valor


fin_cadena:
    dec     ebx
    mov     al, byte[ebx]
    cmp     al, '.'             ;Se quita el punto decimal si sobra despues del
    je      caracter_nulo       ;redondeo
    inc     ebx
caracter_nulo:
    mov     byte [ebx], 0       ;Caracter nulo, fin de la cadena
    inc     ebx
    jmp     end1


end1:

    fclex                       ;Sabemos que el codigo funciona bien generando
                                ;errores de precision, asi que limpiamos las
                                ;excepciones de fpu
    pop     eax                 ;Se restaura el modo de operacion de la FPU
    mov     [ebx], eax
    fldcw   [ebx]

    pop     edx                 ;Se desapilan todos los registros usados
    pop     ecx
    pop     ebx

    leave
    ret


;
; Funcion: imprimir_entero (interna)
;
; Convierte un entero a su representacion ASCII
;
; Parametros:
;               EAX     entero a convertir
;               EBX     puntero al array de caracteres destino
;
imprimir_entero:
    push    eax
    push    ecx
    push    edx
    push    esi

    mov     edx, 0
    mov     ecx, 0
    mov     esi, 10

_apilar_enteros_1:
    div     esi
    add     edx,'0'
    push    edx                 ;Divido por 10 y voy apilando los restos
    inc     ecx
    mov     edx,0
    cmp     eax,10
    jae     _apilar_enteros_1   ;Mientras haya cifras
    cmp     eax,0
    je      _desapilar_enteros_1
    add     eax,'0'             ;Y se toma tambien el ultimo resultado
    push    eax                 ;Si no es 0
    inc     ecx
_desapilar_enteros_1:
    pop     eax
    mov     byte[ebx],al
    inc     ebx
    loop    _desapilar_enteros_1

    pop     esi
    pop     edx
    pop     ecx
    pop     eax
    ret

    
;
; Funcion: imprimir_entero_9_cifras (interna)
;
; Convierte un entero a su representacion ASCII, ocupando siempre 9 cifras
; (supone que entra en 9 cifras, y si es menor, rellena con ceros)
;
; Parametros:
;               EAX     entero a convertir
;               EBX     puntero al array de caracteres destino
;
imprimir_entero_9_cifras:
    push    eax
    push    ecx
    push    edx
    push    esi

    mov     edx, 0
    mov     ecx, 8
    mov     esi, 10

_apilar_enteros_2:
    div     esi
    add     edx,'0'
    push    edx                 ;Divido por 10 y voy apilando los restos
    mov     edx,0
    loop    _apilar_enteros_2
    add     eax,'0'             ;Y se toma tambien el ultimo resultado
    push    eax                 ;Si no es 0
    
    mov     ecx, 9
_desapilar_enteros_2:
    pop     eax
    mov     byte[ebx],al
    inc     ebx
    loop    _desapilar_enteros_2

    pop     esi
    pop     edx
    pop     ecx
    pop     eax
    ret


; Funcion: fFnAtof
; Parametros:
;    String (char*) a convertir
;
; El resultado se devuelve por ST0 (FPU), por ser flotante
;
; Bugs conocidos:
;

fFnAtof_usr:
    push    ebp
    mov     ebp, esp
    push    ebx                 ;Se apilan todos los registros a usar
    push    ecx
    push    edx
    push    esi

reserva2:
    sub     esp, 8              ;Reservo espacio para variaales (4 bytes c/u)
    mov     dword[ebp-4], 10    ;Para multiplicar por 10 
    mov     dword[ebp-8], 0     ;Auxiliar, para trabajar con la FPU

    mov     esi, 0              ;Signo (0 positivo, 1 negativo)
    mov     edx, 0              ;Flag Punto detectado
    mov     ecx, 0              ;Cantidad de cifras decimales
    mov     eax, 0              ;Caracteres leidos

    fild    dword[ebp-8]        ;Se pone en 0 la FPU

    mov     ebx, [ebp+8]        ;Param: Puntero al string a convertir -> EDI

ignora_espacios:
    mov     al, byte[ebx]       ;
    cmp     al, byte ' '
    jne     signo2
    inc     ebx                 ;Leer proximo caracter
    jmp     ignora_espacios

signo2:
    cmp     al, byte '+'
    je      signo2_ok
    cmp     al, byte '-'
    jne     recorrer_digitos
    inc     esi                 ;Signo negativo
signo2_ok:
    inc     ebx                 ;Leer proximo caracter
    mov     al, byte[ebx]

recorrer_digitos:
    cmp     al,'.'              ;En el codigo ASCII estan ./0123456789
    jl      fin_numero          ;Se pregunta si es un numero o el punto
    cmp     al,'9'              ;si no, se acabo el numero
    jg      fin_numero
    cmp     al,'/'
    je      fin_numero

    cmp     al,'.'              ;Es punto ?
    jne     es_numero
    cmp     edx,0               ;Ya se encontro otro punto ?
    jne     fin_numero
    inc     edx                 ;Es punto! (el primero que se encontro)
    jmp     proximo_digito

es_numero:
    sub     al, '0'             ;Se decodifica el ASCII
    cmp     edx, 0
    je      parte_entera2        ;Si ya se esta decodificando la parte decimal
parte_decimal2:                  ;se incrementa el contador de digitos decimales
    inc     ecx
parte_entera2:
    fimul   dword[ebp-4]        ;Se multiplica x10 lo acumulado y se suma la
    mov     dword[ebp-8], eax   ;cifra recien leida
    fiadd   dword[ebp-8]

proximo_digito:
    inc     ebx
    mov     al, byte[ebx]       
    jmp     recorrer_digitos

fin_numero:
    cmp     ecx, 0
    je      corregir_signo2

ajuste_coma:
    fidiv   dword[ebp-4]        ;Se divide x10 el numero tantas veces como
    loop    ajuste_coma         ;cifras decimales se tengan

corregir_signo2:
    cmp     esi, 0
    je      end2
    fchs

end2:
    pop     esi                 ;Se desapilan todos los registros usados
    pop     edx
    pop     ecx
    pop     ebx

    leave
    ret
