main1:
    
    mov rdi, 4                          ; n
    mov rax, 0                          ; res
    mov rdx, 0
    mov ecx, 0                          ; counter

    call recur
    jmp fin


recur:
    inc ecx 
    
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
        ; 2*recur(n-1)                      ; else {
        ; mov rsi, rdi                      ;     return 2*recur(n-1) + 3*recur(n-2)
        push rdi                            ; }
        sub rdi, 1                          
        call recur
        mov rbx, 2
        mul rbx
        push rax


        ; 3*recur(n-2)
        pop rdx
        pop rdi

        sub rdi, 2
        call recur
        mov rbx, 3
        mul rbx


        ; mov rdi, rsi

        ; 2*recur(n-1) + 3*recur(n-2)
        add rdx, rax        
        mov rax, rdx
        
        ret 

fin:

done: