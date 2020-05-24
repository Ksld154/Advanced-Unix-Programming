cmp eax, 0x0
jge L1
mov DWORD PTR [0x600000], 0x-1
jmp T2
L1:
    mov DWORD PTR [0x600000], 0x1

T2:
    cmp ebx, 0x0
    jge L2
    mov DWORD PTR [0x600004], 0x-1
    jmp T3
    L2:
        mov DWORD PTR [0x600004], 0x1

T3:
    cmp ecx, 0x0
    jge L3
    mov DWORD PTR [0x600008], 0x-1
    jmp T4
    L3:
        mov DWORD PTR [0x600008], 0x1

T4:
    cmp edx, 0x0
    jge L4
    mov DWORD PTR [0x60000c], 0x-1
    jmp T5
    L4:
        mov DWORD PTR [0x60000c], 0x1

T5:

done:
