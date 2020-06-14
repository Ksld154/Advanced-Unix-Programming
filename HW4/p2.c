#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <assert.h>

char* filePath = "/mnt/d/Homework/Advanced-Unix-Programming/HW4/countme";


void errquit(const char *msg) {
    perror(msg);
    exit(-1);
}

int main() {
    pid_t child;

    child = fork();
    if(child < 0){ 
        errquit("fork");
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
        long long counter = 0LL;
        int wait_status;
        if(waitpid(child, &wait_status, 0) < 0) {
            errquit("waitpid");
        }
        ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_EXITKILL);
        
        while (WIFSTOPPED(wait_status)) {
            counter++;
            printf("i: %lld\n", counter);
            if(ptrace(PTRACE_SINGLESTEP, child, 0, 0) < 0) {
                errquit("ptrace@parent");
            }
            if(waitpid(child, &wait_status, 0) < 0) {
                errquit("waitpid");
            }
        }
        printf("124\n");
        fprintf(stdout, "## %lld instruction(s) executed\n", counter);
    }
    return 0;
}
