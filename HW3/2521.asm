mov ax, [0x600000]
mov bx, 0x20
not bx
and ax, bx
; xor ax, bx
mov [0x600001], ax
done:
