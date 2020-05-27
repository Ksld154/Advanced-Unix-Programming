mov eax, [0x600000] 
mov ecx, [0x600004] 
neg ecx                 
imul ecx                ; edx:eax = var1 * (-var2)
cdq                     ; sign extension to eax

mov ecx, [0x600008]
sub ecx, ebx
idiv ecx                ; edx:eax = (var1*(-var2)) / (var3%ebx)

mov [0x600008], eax

done:
