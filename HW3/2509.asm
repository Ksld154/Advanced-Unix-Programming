mov ecx, 0

loop1: 
    cmp ecx, 15
    jge loop1Exit

    ; turn a char to lowercase
    mov ax, [0x600000+ecx]                       
    or ax, 0x20                    
    mov [0x600010+ecx], ax

    inc ecx
    jmp loop1

loop1Exit:

done:
