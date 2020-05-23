mov eax, [0x600000]
mov ebx, eax
mov ecx, eax
mov edx, eax
sal eax, 4  ;16
sal ebx, 3  ;8
sal ecx, 1  ;2
add eax, ebx
add eax, ecx
mov [0x600004], eax
done:
