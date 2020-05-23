mov eax, [0x600000] 
mov ebx, 0x5
neg ebx                 ; ebx = -5
imul ebx                ; edx:eax = var1 * (-5)
mov [0x60000c], eax     ; var4 = var1 * (-5)

mov eax, [0x600004]
neg eax                 ; eax = -var2
cdq                     ; sign extension:  edx:eax = eax
mov ebx, [0x600008]
idiv ebx                ; eax = edx:eax / ebx  (eax = -var2 / var3)
mov ecx, edx            ; ecx = -var2 % var3

mov eax, [0x60000c]
cdq
idiv ecx                ; eax = edx:eax / ecx       ( eax = (var1*(-5)) / (-var2%var3) )
mov [0x60000c], eax     ; var4 =  (var1*(-5)) / (-var2%var3)


done:
