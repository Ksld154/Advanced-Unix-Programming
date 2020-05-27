#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <assert.h>

char* filePath = "/mnt/d/Homework/Advanced-Unix-Programming/HW4/traceme";

int main() {
    pid_t child;


    child = fork();
    if (child < 0) {
        printf("Fork error!\n");
        return -1;
    }
    else if(child == 0) {
        if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
            printf("ptrace child error!\n");
            return -1;
        }

        char *arg[] = {filePath, NULL};
        execvp(filePath, arg);
        printf("execvp failed!\n");
    }
    else {
        int status;
        if(waitpid(child, &status, 0) < 0) {
            printf("waitpid error!\n");
            return -1;
        }
        assert(WIFSTOPPED(status));

        ptrace(PTRACE_SETOPTIONS, child, 0, 0);
        ptrace(PTRACE_CONT, child, 0, 0);
        waitpid(child, &status, 0);

        // printf("Ptrace done!\n");
    }


    return 0;
}