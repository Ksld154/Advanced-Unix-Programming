mov eax, [0x600000] 
add eax, [0x600004]
mov ebx, [0x600008]     ; val1+val2

mul ebx                 ; (val1+val2) * val3
; mov [0x60000c], edx
mov [0x60000c], eax
done:
