mov eax, [0x600000] 
mov ecx, [0x600004] 
neg ecx                 
imul ecx                ; edx:eax = var1 * (-var2)
mov edx, 0x0
; mov edx, eax            ; edx = var1 * (-var2)


mov ecx, [0x600008]
sub ecx, ebx
idiv ecx

mov [0x600008], eax

done:
