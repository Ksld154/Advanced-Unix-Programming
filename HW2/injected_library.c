#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>


int chdir(const char *);
int chmod(const char *, mode_t);
int chown(const char *, uid_t, gid_t);
int creat(const char *, mode_t);
int creat64(const char *, mode_t);
FILE* fopen(const char *, const char *);
FILE* fopen64(const char *, const char *);
int link(const char *, const char *);
int mkdir(const char *, mode_t );
int open(const char *, int, ...);
int open64(const char *, int, ...);
int openat(int, const char *, int, ...);
int openat64(int, const char *, int, ...);

DIR *opendir(const char *);
ssize_t readlink(const char *, char *, size_t);
int remove(const char *);
int rename(const char *, const char *);
int rmdir(const char *);
int __xstat(int, const char *, struct stat *);
int __xstat64(int, const char *, struct stat *);
int symlink(const char *, const char *);
int unlink(const char *);

int execl(const char *, const char *, ... /* (char  *) NULL */);
int execlp(const char *, const char *, ... /* (char  *) NULL */);
int execle(const char *, const char *, ... /*, (char *) NULL, char * const envp[] */);
int execv(const char *, char *const argv[]);
int execvp(const char *, char *const argv[]);
int execve(const char *, char *const argv[], char *const envp[]);
int system(const char *);


int is_in_cwd(const char *file) {
    
    // absolute path of BaseDir
    char absBaseDir[FILENAME_MAX];
    strcpy(absBaseDir, getenv("MY_BASEDIR"));


    char *filePath = strdup(file);
    char abs_filePath[FILENAME_MAX] = {0};
    if(strlen(filePath) < 1) {
        printf("[sandbox] Wrong file path format!\n");
        return -1;
    }

    // 1. set the base_dir of filePath
    if(filePath[0] == '.') {
        // use cwd as base_dir of filePath
        getcwd(abs_filePath, sizeof(abs_filePath));
    }
    else if (filePath[0] != '/'){
        printf("[sandbox] Wrong file path format!\n");
        return -1;
    }
    
    // 2. parse relative path to abs. path
    if(strcmp(filePath, "/") != 0) {
        char *ptr;
        int startflag = 0;
        for(ptr = strtok(filePath, "/"); ptr; ptr = strtok(NULL, "/")) {
            // printf("%s\n", ptr);
            if(strcmp(ptr, "..") == 0) {
                // go-back
                for(int i = strlen(abs_filePath)-1; i >= 0; i--){
                    if(abs_filePath[i] == '/') {
                        abs_filePath[i] = '\0';
                        break;
                    }
                }
            } 
            else if (strcmp(ptr, ".") == 0) {
                // skip
                continue;
            }
            else {
                // normal folder/file name, just append it
                strncat(abs_filePath, "/", sizeof("/"));
                strncat(abs_filePath, ptr, strlen(ptr)+1);
            }
            // printf("abs: %s\n", abs_filePath);
        }
    }
    else { 
        strncpy(abs_filePath, "/", sizeof("/"));
    }

    // printf("Abs      cwd: %s\n", absBaseDir);
    // printf("Abs arg path: %s\n", abs_filePath);
    
    // compare absBaseDir and abs_filePath
    int parent_len = strlen(absBaseDir);
    int child_len = strlen(abs_filePath);
    int is_in_cwd = 0;
    if( (strncmp(absBaseDir, abs_filePath, strlen(absBaseDir)) == 0) \
        && ( parent_len <= child_len) \
    ){    
        is_in_cwd = 1;
    }

    return is_in_cwd;
}


static int (*old_chdir)(const char*) = NULL; /* function pointer */
int chdir(const char *path) {
    
    int resp = 0;
    if(old_chdir == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            old_chdir = dlsym(handle, "chdir");
    }

    if(old_chdir != NULL){
        int allowed_path = is_in_cwd(path);
        if(allowed_path != 1) {
            printf("[sandbox] chdir: access to %s is not allowed \n", path);
            errno = EACCES;
            return -1;
        }
        resp = old_chdir(path);
        printf("%s\n", path);
        return resp;
    }
    return -1;
}


static int (*old_chmod)(const char*, mode_t) = NULL; /* function pointer */
int chmod(const char *path, mode_t mode) {
    if(old_chmod == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            old_chmod = dlsym(handle, "chmod");
    }

    if(old_chmod != NULL){
        int allowed_path = is_in_cwd(path);
        if(allowed_path != 1) {
            printf("[sandbox] chmod: access to %s is not allowed \n", path);
            errno = EACCES;
            return -1;
        }
        int resp = old_chmod(path, mode);
        return resp;
    }
    return -1;
}

static int (*real_chown)(const char*, uid_t, gid_t) = NULL; /* function pointer */
int chown(const char *path, uid_t owner, gid_t group) {
    if(real_chown == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_chown = dlsym(handle, "chown");
    }

    if(real_chown != NULL){
        int allowed_path = is_in_cwd(path);
        if(allowed_path != 1) {
            printf("[sandbox] chown: access to %s is not allowed \n", path);
            errno = EACCES;
            return -1;
        }
        int resp = real_chown(path, owner, group);
        return resp;
    }
    return -1;
}

static int (*real_creat)(const char*, mode_t) = NULL; /* function pointer */
int creat(const char *path, mode_t mode) {
    if(real_creat == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_creat = dlsym(handle, "creat");
    }

    if(real_creat != NULL){
        int allowed_path = is_in_cwd(path);
        if(allowed_path != 1) {
            printf("[sandbox] creat: access to %s is not allowed \n", path);
            errno = EACCES;
            return -1;
        }
        int resp = real_creat(path, mode);
        return resp;
    }
    return -1;
}

static int (*real_creat64)(const char*, mode_t) = NULL; /* function pointer */
int creat64(const char *path, mode_t mode) {
    if(real_creat64 == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_creat64 = dlsym(handle, "creat64");
    }

    if(real_creat64 != NULL){
        int allowed_path = is_in_cwd(path);
        if(allowed_path != 1) {
            printf("[sandbox] creat64: access to %s is not allowed \n", path);
            errno = EACCES;
            return -1;
        }
        int resp = real_creat64(path, mode);
        return resp;
    }
    return -1;
}

static FILE* (*real_fopen)(const char*, const char*) = NULL; /* function pointer */
FILE* fopen(const char *path, const char *mode) {
    if(real_fopen == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_fopen = dlsym(handle, "fopen");
    }
    
    if(real_fopen != NULL){
        int allowed_path = is_in_cwd(path);
        if(allowed_path != 1) {
            printf("[sandbox] fopen: access to %s is not allowed \n", path);
            return NULL;
        }
        FILE* resp = real_fopen(path, mode);
        return resp;
    }
    return NULL;
}

static FILE* (*real_fopen64)(const char*, const char*) = NULL; /* function pointer */
FILE* fopen64(const char *path, const char *mode) {
    if(real_fopen64 == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_fopen64 = dlsym(handle, "fopen64");
    }
    
    if(real_fopen64 != NULL){
        int allowed_path = is_in_cwd(path);
        if(allowed_path != 1) {
            printf("[sandbox] fopen64: access to %s is not allowed \n", path);
            return NULL;
        }
        FILE* resp = real_fopen64(path, mode);
        return resp;
    }
    return NULL;
}


static int (*real_link)(const char*, const char*) = NULL; /* function pointer */
int link(const char *path1, const char *path2) {
    if(real_link == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_link = dlsym(handle, "link");
    }
    if(real_link != NULL){
        // int allowed_path1 = is_in_cwd(path1);
        // int allowed_path2 = is_in_cwd(path2);
        if(is_in_cwd(path1) != 1) {
            printf("[sandbox] link: link from %s is not allowed \n", path1);
            errno = EACCES;
            return -1;
        }
        if(is_in_cwd(path2) != 1) {
            printf("[sandbox] link: link to %s is not allowed \n", path2);
            errno = EACCES;
            return -1;
        }
        int resp = real_link(path1, path2);
        return resp;
    }
    return -1;
}

// mkdir
static int (*real_mkdir)(const char*, mode_t) = NULL; /* function pointer */
int mkdir(const char *path, mode_t mode) {
    if(real_mkdir == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_mkdir = dlsym(handle, "mkdir");
    }
    if(real_mkdir != NULL){
        int allowed_path = is_in_cwd(path);
        if(allowed_path != 1) {
            printf("[sandbox] mkdir: access to %s is not allowed \n", path);
            return -1;
        }
        int resp = real_mkdir(path, mode);
        return resp;
    }
    return -1;
}

// open VLA
static int (*real_open)(const char*, int, ...) = NULL; /* function pointer */
int open(const char *path, int flags, ...) {
    va_list args;
    va_start(args, flags);
    mode_t mode = 0;
    mode = va_arg(args, mode_t);

    if(real_open == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_open = dlsym(handle, "open");
    }
    if(real_open != NULL){
        if(is_in_cwd(path) != 1) {
            printf("[sandbox] open: access to %s is not allowed \n", path);
            errno = EACCES;
            return -1;
        }
        int resp = real_open(path, flags, mode);
        return resp;
    }
    return -1;
}

// open64 VLA
static int (*real_open64)(const char*, int, ...) = NULL; /* function pointer */
int open64(const char *path, int flags, ...) {
    va_list args;
    va_start(args, flags);
    mode_t mode = va_arg(args, mode_t);

    if(real_open64 == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_open64 = dlsym(handle, "open64");
    }
    if(real_open64 != NULL){
        if(is_in_cwd(path) != 1) {
            printf("[sandbox] open64: access to %s is not allowed \n", path);
            errno = EACCES;
            return -1;
        }
        
        int resp = real_open64(path, flags, mode);
        return resp;
    }
    return -1;
}

// openat VLA
static int (*real_openat)(int, const char*, int, ...) = NULL; 
int openat(int dirfd, const char *path, int flags, ...) {
    va_list args;
    va_start(args, flags);
    mode_t mode = va_arg(args, mode_t);

    if(real_openat == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_openat = dlsym(handle, "openat");
    }
    if(real_open != NULL){
        if(is_in_cwd(path) != 1) {
            printf("[sandbox] openat: access to %s is not allowed \n", path);
            errno = EACCES;
            return -1;
        }
        
        int resp = real_openat(dirfd, path, flags, mode);
        return resp;
    }
    return -1;
}


// openat64 VLA
static int (*real_openat64)(int, const char*, int, ...) = NULL; 
int openat64(int dirfd, const char *path, int flags, ...) {
    va_list args;
    va_start(args, flags);
    mode_t mode = va_arg(args, mode_t);

    if(real_openat64 == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_openat64 = dlsym(handle, "openat64");
    }
    if(real_openat64 != NULL){
        if(is_in_cwd(path) != 1) {
            printf("[sandbox] openat64: access to %s is not allowed \n", path);
            errno = EACCES;
            return -1;
        }
        
        int resp = real_openat64(dirfd, path, flags, mode);
        return resp;
    }
    return -1;
}


// opendir 
static DIR* (*real_opendir)(const char*) = NULL; /* function pointer */
DIR *opendir(const char *path) {
    if(real_opendir == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_opendir = dlsym(handle, "opendir");
    }
    if(real_opendir != NULL){
        int allowed_path = is_in_cwd(path);
        if(allowed_path != 1) {
            printf("[sandbox] opendir: access to %s is not allowed \n", path);
            return NULL;
        }
        DIR* resp =real_opendir(path);
        return resp;
    }
    return NULL;
}

// readlink
static ssize_t (*real_readlink)(const char*, char*, size_t) = NULL; 
ssize_t readlink(const char *path, char *buf, size_t bufsize) {
    if(real_readlink == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_readlink = dlsym(handle, "readlink");
    }
    if(real_readlink != NULL){
        int allowed_path = is_in_cwd(path);
        if(allowed_path != 1) {
            printf("[sandbox] readlink: access to %s is not allowed \n", path);
            errno = EACCES;
            return -1;
        }
        ssize_t resp =real_readlink(path, buf, bufsize);
        return resp;
    }
    return -1;
}

// remove 
static int (*real_remove)(const char*) = NULL; 
int remove(const char *path) {
    if(real_remove == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_remove = dlsym(handle, "remove");
    }
    if(real_remove != NULL){
        if(is_in_cwd(path) != 1) {
            printf("[sandbox] remove: access to %s is not allowed \n", path);
            errno = EACCES;
            return -1;
        }
        int resp =real_remove(path);
        return resp;
    }
    return -1;
}


// rename
static int (*real_rename)(const char*, const char*) = NULL; 
int rename(const char *oldpath, const char *newpath) {
        if(real_rename == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_rename = dlsym(handle, "rename");
    }
    if(real_rename != NULL){
        if(is_in_cwd(oldpath) != 1 || is_in_cwd(newpath) != 1) {
            printf("[sandbox] rename: rename from %s to %s is not allowed \n", oldpath, newpath);
            errno = EACCES;
            return -1;
        }
        int resp =real_rename(oldpath, newpath);
        return resp;
    }
    return -1;
}


// rmdir 
static int (*real_rmdir)(const char*) = NULL; 
int rmdir(const char *path) {
    if(real_rmdir == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_rmdir = dlsym(handle, "rmdir");
    }
    if(real_rmdir != NULL){
        if(is_in_cwd(path) != 1) {
            printf("[sandbox] rmdir: access to %s is not allowed \n", path);
            errno = EACCES;
            return -1;
        }
        int resp =real_rmdir(path);
        return resp;
    }
    return -1;
}

// __xstat
static int (*real_xstat)(int, const char*, struct stat*) = NULL; 
int __xstat(int ver, const char *path, struct stat *stat_buf) {
    if(real_xstat == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_xstat = dlsym(handle, "__xstat");
    }
    if(real_xstat != NULL){
        if(is_in_cwd(path) != 1) {
            printf("[sandbox] __xstat: access to %s is not allowed \n", path);
            errno = EACCES;
            return -1;
        }
        int resp =real_xstat(ver, path, stat_buf);
        return resp;
    }
    return -1;
}

// __xstat64 
static int (*real_xstat64)(int, const char*, struct stat*) = NULL; 
int __xstat64(int ver, const char *path, struct stat *stat_buf) {
    if(real_xstat64 == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_xstat64 = dlsym(handle, "__xstat");
    }
    if(real_xstat64 != NULL){
        if(is_in_cwd(path) != 1) {
            printf("[sandbox] __xstat64: access to %s is not allowed \n", path);
            errno = EACCES;
            return -1;
        }
        int resp =real_xstat64(ver, path, stat_buf);
        return resp;
    }
    return -1;
}

// symlink 
static int (*real_symlink)(const char*, const char*) = NULL; 
int symlink(const char *target, const char *linkpath) {
    if(real_symlink == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_symlink = dlsym(handle, "symlink");
    }
    if(real_symlink != NULL){
        if(is_in_cwd(target) != 1 || is_in_cwd(linkpath) != 1) {
            printf("[sandbox] symlink: symlink from %s to %s is not allowed \n", target, linkpath);
            errno = EACCES;
            return -1;
        }
        int resp =real_symlink(target, linkpath);
        return resp;
    }
    return -1;
}

// unlink
static int (*real_unlink)(const char*) = NULL; 
int unlink(const char *path) {
    if(real_unlink == NULL) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);
        if(handle != NULL)
            real_unlink = dlsym(handle, "unlink");
    }
    if(real_unlink != NULL){
        if(is_in_cwd(path) != 1) {
            printf("[sandbox] unlink: unlink %s is not allowed \n",path);
            errno = EACCES;
            return -1;
        }
        int resp =real_unlink(path);
        return resp;
    }
    return -1;
}

int execl(const char *path, const char *arg, ... /* (char  *) NULL */) {
    errno = EACCES;
    return -1;
}
int execlp(const char *file, const char *arg, ... /* (char  *) NULL */) {
    errno = EACCES;
    return -1;
}
int execle(const char *path, const char *arg, ... /*, (char *) NULL, char * const envp[] */) {
    errno = EACCES;
    return -1;
}
int execv(const char *path, char *const argv[]) {
    errno = EACCES;
    return -1;
}
int execvp(const char *file, char *const argv[]) {
    errno = EACCES;
    return -1;
}
int execve(const char *path, char *const argv[], char *const envp[]) {
    errno = EACCES;
    return -1;
}
int system(const char *command) {
    errno = EACCES;
    return -1;
}