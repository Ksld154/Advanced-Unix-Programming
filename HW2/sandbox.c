#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

struct option_arg {
    int  optind;
    char soPath[FILENAME_MAX];
    char baseDir[FILENAME_MAX];
    int  errOption;
};

struct option_arg handleOptions(int argc, char *argv[]) {

    struct option_arg oArg;
    oArg.errOption = 0;

    int c;
    const char *optFormat=":p:d:";

    while( (c = getopt(argc, argv, optFormat)) != -1) {
        switch (c) {
            case 'p':
                strncpy(oArg.soPath, optarg, sizeof(optarg));
                break;
            case 'd':
                strncpy(oArg.baseDir, optarg, sizeof(optarg));
                // printf("base  dir: %s\n", oArg.baseDir);
                break;
            case ':':
                printf("Addition arguments required for option %c \n", optopt);
                break;
            case '?':
                printf("Invalid option -- '%c' \n", optopt);
                printf("usage: ./sandbox [-p sopath] [-d basedir] [--] cmd [cmd args ...]\n");
                printf("-p: set the path to sandbox.so, default = ./sandbox.so\n");
                printf("-d: the base directory that is allowed to access, default = .\n");
                printf("--: seperate the arguments for sandbox and for the executed command\n");
                oArg.errOption = 1;
                break;
        }
    }


    oArg.optind = optind;
    if(strcmp(oArg.soPath, "") == 0)
        strncpy(oArg.soPath, "./sandbox.so", sizeof("./sandbox.so"));
    if(strcmp(oArg.baseDir, "") == 0)
        strncpy(oArg.baseDir, "./", 2);

    char realBaseDir[FILENAME_MAX];
    realpath(oArg.baseDir, realBaseDir);
    strncpy(oArg.baseDir, realBaseDir, sizeof(realBaseDir));
    
    return oArg;
}


int main(int argc, char *argv[]) {

    struct option_arg opt_arg = handleOptions(argc, argv);
    if(opt_arg.errOption) {
        printf("[sandbox]: Invaild option detected, will not execute cmd\n");
        return -1;
    }


    char *cmd[argc+1];
    int i = 0;
    for(int j = opt_arg.optind; j < argc; j++) {
        cmd[i] = argv[j];
        i++;
    }
    
    setenv("LD_PRELOAD", opt_arg.soPath, 1);
    setenv("MY_BASEDIR", opt_arg.baseDir, 1);
    execvp(cmd[0], cmd);

    return 0;
}