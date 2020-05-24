
main1:
    mov ecx, -1
    mov edi, 9                      ; ARRAY_LEN-1
    jmp outerLoop

swap:
    mov [0x600000+edx*4], ebx
    mov [0x600000+edx*4+4], eax
    inc edx
    jmp inner_loop

inner_loop:                         ; for(int j = 0; j < ARRAY_LEN-i; j++)
    cmp edx, edi
    jge outerLoop

    mov eax, [0x600000+edx*4]
    mov ebx, [0x600000+edx*4+4]
    cmp eax, ebx                    ; if(arr[j] > arr[j+1]) 
    jg swap                         ;     swap(arr[j], arr[j+1])
                        
    inc edx
    jmp inner_loop


outerLoop:
    cmp ecx, 9
    jge outerExit

    inc ecx
    mov edx, 0                    ; inner loop's j
    jmp inner_loop
    
    sub edi, 1
    jmp outerLoop


outerExit:

done:
