#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <assert.h>

char* filePath = "/mnt/d/Homework/Advanced-Unix-Programming/HW4/syscall";


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
        long long syscall_counter = 0LL;
        int wait_status;
        if(waitpid(child, &wait_status, 0) < 0) {
            errquit("waitpid");
        }
        ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_EXITKILL|PTRACE_O_TRACESYSGOOD);
        
        while (WIFSTOPPED(wait_status)) {
            syscall_counter++;
            if(ptrace(PTRACE_SYSCALL, child, 0, 0) != 0) {
                errquit("ptrace@parent");
            }
            if(waitpid(child, &wait_status, 0) < 0) {
                errquit("waitpid");
            }
            
            // if(WSTOPSIG(wait_status) & 0x80) {
            //     // syscall stop
            //     syscall_counter++;
            //     printf("i: %lld\n", syscall_counter);
            // }
        }
        printf("syscall\n");
        // fprintf(stdout, "## %lld syscall(s) executed\n", syscall_counter);
        fprintf(stdout, "## %lld syscall(s) executed\n", syscall_counter/2);
    }
    return 0;
}
