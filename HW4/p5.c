#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/user.h>
#include <assert.h>

char* filePath = "/home/ksld154/Advanced-Unix-Programming/HW4/no_more_traps";
char* txtFile  = "/home/ksld154/Advanced-Unix-Programming/HW4/no_more_traps.txt";


void errquit(const char* msg) {
    perror(msg);
    exit(-1);
}

unsigned char readTxt_byte(FILE* fp) {
    
    if(feof(fp)) {
        
        exit(-1);
    }
    else if(fp == NULL) {
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

    FILE* fp = fopen(txtFile, "r");
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
        int cnt = 0;
        while(waitpid(child, &wait_status, 0) > 0) {

            if(!WIFSTOPPED(wait_status)) {
                continue;
            }
            printf("signal: %d\n", WEXITSTATUS(wait_status));


            // 1. find where 0xcc is
            struct user_regs_struct regs;
            if((ptrace(PTRACE_GETREGS, child, NULL, &regs)) < 0) {
                errquit("ptrace(get_reg)");
            }
            printf("rip: %llX\n", regs.rip-1);

            
            // 2. get original code
            unsigned char code_byte = readTxt_byte(fp);
            long original_code = ptrace(PTRACE_PEEKTEXT, child, regs.rip-1, NULL);
            long modified_code = (original_code & 0xFFFFFFFFFFFFFF00) | code_byte;

            
            // 3. replace it with readTxt_byte(fp)
            if(ptrace(PTRACE_POKETEXT, child, regs.rip-1, modified_code) < 0) {
                errquit("poketext");
            }
            printf("original code: %lX\n", original_code);
            printf("replaced with: %lX\n", modified_code);


            // 4. run the replaced code
            regs.rip = regs.rip - 1;
            if(ptrace(PTRACE_SETREGS, child, NULL, &regs) < 0) {
                errquit("ptrace(set_reg)");
            }

            printf("ind: %d\n\n", cnt++);
            ptrace(PTRACE_CONT, child, 0, 0);
        }

        printf("Finished!\n");
    }
    return 0;
}