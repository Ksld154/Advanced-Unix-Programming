main1:
    
    mov rdi, 16                          ; n
    mov rax, 0                          ; res
    mov rdx, 0
    mov ecx, 0                          ; counter

    ; call recur
    call r
    jmp fin


r:
    case0:
        cmp rdi, 0                          ; if (n <= 0) {
        jne case1                           ;     return 0;    
        mov rax, 0                          ; }
        ret                          
    
    case1:
        cmp rdi, 1                          ; else if (n == 1) {
        jne case2                           ;     return 1;
        mov rax, 1                          ; }
        ret 

    case2:

        jmp recur_cal
        ret



recur_cal:
    inc ecx 
    
    ; 2*r(n-1)
    push rdi                ; save n
    sub rdi, 1

    call r                          
    mov rbx, 2
    mul rbx
    
    pop rdi                 ; retrive n for calculating later's r(n-2)
    push rax                ; save 2*r(n-1)

    ; 3*r(n-2)
    sub rdi, 2

    call r
    mov rbx, 3
    mul rbx

    ; 2*r(n-1) + 3*r(n-2)
    pop rdx
    add rax, rdx        
    
    ret 

fin:

done:
