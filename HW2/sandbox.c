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
};

struct option_arg handleOptions(int argc, char *argv[]) {

    struct option_arg oArg;

    int c;
    const char *optFormat=":p:d:";
    // char soPath[FILENAME_MAX];
    // char baseDir[FILENAME_MAX];


    while( (c = getopt(argc, argv, optFormat)) != -1) {
        switch (c) {
            case 'p':
                strncpy(oArg.soPath, optarg, sizeof(optarg));
                // oArg.soPath = optarg;
                printf("*.so path: %s\n", oArg.soPath);
                break;
            case 'd':
                strncpy(oArg.baseDir, optarg, sizeof(optarg));
                // oArg.baseDir = optarg;
                printf("base  dir: %s\n", oArg.baseDir);
                break;
            case ':':
                printf("Addition arguments required for option %c \n", optopt);
                break;
            case '?':
                printf("Invalid option -- '%c' \n", optopt);
                break;
        }
    }


    for (int i = optind; i < argc; i++) {
        printf("Non-option argument %s\n", argv[i]);
    }

    oArg.optind = optind;
    if(strcmp(oArg.soPath, "") == 0)
        strncpy(oArg.soPath, "./sandbox.so", sizeof("./sandbox.so"));
    if(strcmp(oArg.baseDir, "") == 0)
        strncpy(oArg.baseDir, "./", 2);

    return oArg;
}




int main(int argc, char *argv[]) {


    struct option_arg opt_arg = handleOptions(argc, argv);

    // printf("%d\n", opt_arg.optind);
    printf("%s %s\n", opt_arg.soPath, opt_arg.baseDir);

    char *cmd[argc+1];

    int i = 0;
    for(int j = opt_arg.optind; j < argc; j++) {
        cmd[i] = argv[j];
        // printf("%s\n", cmd[i]);
        i++;
    }
    
    setenv("LD_PRELOAD", opt_arg.soPath, 1);
    setenv("MY_BASEDIR", opt_arg.baseDir, 1);
    execvp(cmd[0], cmd);


    return 0;
}