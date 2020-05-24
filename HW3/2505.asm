mov bx, ax                              ; bx is for keeping track of original ax value in the iteration

; fill the buf from bottom (because it's easier to get LSB, rather than MSB)
mov ecx, 15
loop1:                                  ; for(int i=15; i >= 0; i--){} 
    cmp ecx, 0
    jl loop1Exit

    and ax, 0x1                         ; get current LSB
    add ax, 0x30                        ; turn int into ASCII char
    mov BYTE PTR[0x600000+ecx], al      ; buf[i] = ax

    shr bx, 0x1                         ; shift right 1 bit, so that we got new LSB for new iteration
    mov ax, bx

    sub ecx, 1
    jmp loop1

loop1Exit:

done:
