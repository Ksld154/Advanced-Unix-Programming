mov eax, [0x600000] 
mov ebx, 0x5
neg ebx                 ; ebx = -5
imul ebx                ; edx:eax = var1 * (-5)
; mov edx, 0x0            ; truncate the overflowed part (edx)
mov [0x60000c], eax     ; var4 = var1 * (-5)

mov eax, [0x600004]
neg eax                 ; eax = -var2
mov ebx, [0x600008]
idiv ebx                
mov ecx, edx            ; ecx = -var2 % var3
mov edx, 0x0

mov eax, [0x60000c]
idiv ecx
mov edx, 0x0
mov [0x60000c], eax

done:
