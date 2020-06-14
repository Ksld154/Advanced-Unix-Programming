#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/user.h>
#include <assert.h>

char* filePath = "/mnt/d/Homework/Advanced-Unix-Programming/HW4/no_more_traps";
char* txtFile  = "/mnt/d/Homework/Advanced-Unix-Programming/HW4/no_more_traps.txt";


void errquit(const char* msg) {
    perror(msg);
    exit(-1);
}

unsigned char readTxt_byte(FILE* fp) {
    if(fp == NULL) {
        errquit("Read txt file error!");
        return -1;
    }
    else {
        char buf[2];
        fread(buf, sizeof(char), 2, fp);
        
        unsigned char byte_code = strtol(buf, NULL, 16);
        return byte_code;
    }
}


int main () {

    // FILE* fp = fopen(txtFile, "r");
    // while(!feof(fp)) {
    //     unsigned char byte_code = readTxt_byte(fp);
    //     printf("%u\n", byte_code);
    // }




    pid_t child;
    child = fork();

    if(child < 0) {
        errquit("Fork error!");
    }
    else if(child == 0) {
        if(ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
            errquit("ptrace@child");
        }

        char *arg[] = {filePath, NULL};
        execvp(filePath, arg);
        errquit("execvp");
    }
    else {
        int wait_status = 0;
        if(waitpid(child, &wait_status, 0) < 0) {
            errquit("waitpid");
        }    
        ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_EXITKILL);
        ptrace(PTRACE_CONT, child, 0, 0);

        
        // PEEKTEXT, POKETEXT...
        // 1. find where 0xcc is
        // 2. replace it with readTxt_byte(fp)
        while(WIFSTOPPED(wait_status)) {
            // printf("wait: %d\n", WSTOPSIG(wait_status));
            
            // 1. find where 0xcc is
            struct user_regs_struct regs;
            ptrace(PTRACE_GETREGS, child, NULL, &regs);
            printf("%llX\n", regs.rip);

            // 2. get original code
            long original_code =  ptrace(PTRACE_PEEKTEXT, child, regs.rip, NULL);
            printf("%lX\n", original_code);


            // 3. replace it with readTxt_byte(fp)
            // unsigned char code_byte = readTxt_byte(fp);

            if(waitpid(child, &wait_status, 0) < 0) {
                errquit("waitpid");
            }

        }
        printf("23");

    }
    return 0;
}