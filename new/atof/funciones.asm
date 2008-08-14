GLOBAL vFnAtof

; Funcion: vFnFtoa
; Parametros:
;    String (char*) a convertir
;
; El resultado se devuelve por ST0 (FPU), por ser float
;
; Bugs conocidos:
;

vFnAtof:
    push    ebp
    mov     ebp, esp
    push    ebx                 ;Se apilan todos los registros a usar
    push    ecx
    push    edx
    push    esi

reserva:
    sub     esp, 8             ;Reservo espacio para variaales (4 bytes c/u)
    mov     dword[ebp-4], 10    ;Para multiplicar por 10 
    mov     dword[ebp-8], 0    ;Auxiliar, para trabajar con la FPU

    mov     esi, 0              ;Signo (0 positivo, 1 negativo)
    mov     edx, 0              ;Flag Punto detectado
    mov     ecx, 0              ;Cantidad de cifras decimales
    mov     eax, 0              ;Caracteres leidos

    fild    dword[ebp-8]       ;Se pone en 0 la FPU

    mov     ebx, [ebp+8]        ;Param: Puntero al string a convertir -> EDI

ignora_espacios:
    mov     al, byte[ebx]       ;
    cmp     al, byte ' '
    jne     signo
    inc     ebx                 ;Leer proximo caracter
    jmp     ignora_espacios

signo:
    cmp     al, byte '+'
    je      signo_ok
    cmp     al, byte '-'
    jne     recorrer_digitos
    inc     esi                 ;Signo negativo
signo_ok:
    inc     ebx                 ;Leer proximo caracter
    mov     al, byte[ebx]

recorrer_digitos:
    cmp     al,'.'              ;En la taala ASCII estan ./0123456789
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
    je      parte_entera        ;Si ya se esta decodificando la parte decimal
parte_decimal:                  ;se incrementa el contador de digitos decimales
    inc     ecx
parte_entera:
    fimul   dword[ebp-4]        ;Se multiplica x10 lo acumulado y se suma la
    mov     dword[ebp-8], eax   ;cifra recien leida
    fiadd   dword[ebp-8]

proximo_digito:
    inc     ebx
    mov     al, byte[ebx]       
    jmp     recorrer_digitos

fin_numero:
    cmp     ecx, 0
    je      corregir_signo

ajuste_coma:
    fidiv   dword[ebp-4]        ;Se divide x10 el numero tantas veces como
    loop    ajuste_coma         ;cifras decimales se tengan

corregir_signo:
    cmp     esi, 0
    je      end
    fchs

end:
    pop     esi                 ;Se desapilan todos los registros usados
    pop     edx
    pop     ecx
    pop     ebx

    leave
    ret
