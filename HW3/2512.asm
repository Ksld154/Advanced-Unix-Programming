mov eax, [0x600000] 
mov ebx, 0x5
mul ebx                ; edx:eax = var1 * 5
mov edx, 0x0            ; truncate the overflowed part (edx)

mov ebx, [0x600004]     
sub ebx, 0x3 
div ebx                ; eax = eax / (var2-3)

mov [0x600008], eax

done:
