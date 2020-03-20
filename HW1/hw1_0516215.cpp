#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <regex.h>
#include <unistd.h>
#include <iostream>
#include <vector>
using namespace std;

const string NET_FILE = "/proc/net/";


struct connInfo {
    char* localAddr;
    char* remoteAddr;
    char* inode;
};

struct procInfo {
    const char* pid;
    const char* procName;
    const char* inode; 
};

vector <struct connInfo> connList;
vector <struct procInfo> inodeList;

int parseConnEntry(char *);
string parseProcName(struct procInfo inodeEntry);

int readConnection(string connType, const string NET_FILE){

    const string connPath = NET_FILE + connType;
    printf("%s\n", connPath.c_str());

    FILE    *fp;
    char    *line = NULL;
    size_t  n = 0; 

    fp = fopen(connPath.c_str(), "r");
    if (fp == NULL){
        // exit(EXIT_FAILURE);
    }

    int idx = 0;
    while(getline(&line, &n, fp) != -1){
        if (idx >= 1){
            parseConnEntry(line);
        }
        idx++;
    }

    return 0;
}

int scanProcess(){

    regex_t regex;
    int regexRes = regcomp(&regex, "^[0-9]+$", REG_EXTENDED);
    if(regexRes) {
        fprintf(stderr, "Could not compile regex\n");
        // exit(EXIT_FAILURE);
    }
    
    DIR *dir_ptr = opendir("/proc/");
    if(dir_ptr == NULL){
        printf("Fail to open /proc\n");
        // exit(EXIT_FAILURE);
    }


    struct dirent *dirent_ptr; readdir(dir_ptr);  
    int dfd = dirfd(dir_ptr);
    
    vector <string> pidFolders;

    // scan for pidFolders
    while( (dirent_ptr = readdir(dir_ptr))  != NULL){

        struct stat statbuf;
        if(fstatat(dfd, dirent_ptr->d_name, &statbuf, 0) == -1) {
            printf("fstatat call error!\n");
        }

        // it is a folder
        else if(S_ISDIR(statbuf.st_mode)) {
            
            int regResult = regexec(&regex, dirent_ptr->d_name, 0, NULL, 0);
            if(!regResult){
                pidFolders.push_back(dirent_ptr->d_name);
            }
        }
    }
    closedir(dir_ptr);

    // scan fd folder inside every pidFolders
    for (size_t i = 0; i < pidFolders.size(); i++){

        string fdPath = "/proc/" + pidFolders[i] + "/fd"; 
        printf("\n%s\n", fdPath.c_str());
        
        DIR *dir_ptr = opendir(fdPath.c_str());
        if(dir_ptr == NULL){
            printf("Fail to open %s\n", fdPath.c_str());
            // exit(EXIT_FAILURE);
        }

        struct dirent *dirent_ptr; readdir(dir_ptr);  

        while( (dirent_ptr = readdir(dir_ptr)) != NULL) {

            if(strcmp("..", dirent_ptr->d_name) != 0){

                string linkPath = fdPath + "/" + dirent_ptr->d_name;
                char buf[512];

                // use fd to access symbolic link
                int linkfd = readlink(linkPath.c_str(), buf, sizeof(buf));
                if (linkfd == -1){
                    printf("Read process link failed\n");
                    // exit(EXIT_FAILURE);
                }

                
                string fdEntry(buf);
                struct procInfo inodeEntry;

                // parse inode of each fd
                if (fdEntry.find("socket:[") != string::npos){
                    // cout << fdEntry << endl; 
                    string sub1 = fdEntry.substr(0, fdEntry.find("]", 0));
                    string inode = sub1.substr(fdEntry.find("[", 0)+1);

                    // cout << inode << endl;
                    inodeEntry.inode = inode.c_str();
                    inodeEntry.pid = pidFolders[i].c_str();
                    string procName = parseProcName(inodeEntry);
                    inodeEntry.procName = procName.c_str();

                    printf("%s %s %s\n", inodeEntry.pid, inodeEntry.inode, inodeEntry.procName);

                    inodeList.push_back(inodeEntry);
                }
            }
        }
        closedir(dir_ptr);
    }

    return 0;
}

int parseConnEntry(char *line){

    struct connInfo conn;
    char* substr = strtok(line, " ");
    
    int idx = 0;
    while(substr != NULL){

        if(idx == 1){
            conn.localAddr = substr;
        }
        else if(idx == 2){
            conn.remoteAddr = substr;
        }
        else if(idx == 9){
            conn.inode = substr;
        }

        substr = strtok(NULL, " ");
        idx++;
    }

    printf("%s %s %s\n", conn.localAddr, conn.remoteAddr, conn.inode);
    connList.push_back(conn);

    return 0;
}

string parseProcName(struct procInfo inodeEntry){
    string cmdFile = "/proc/" + string(inodeEntry.pid) + "/cmdline";
    string procName;

    FILE*   fd;
    char    *line = NULL;
    size_t  n = 0; 
    
    fd = fopen(cmdFile.c_str(), "r");
    if (fd == NULL){
        // exit(EXIT_FAILURE);
    }

    while(getline(&line, &n, fd) != -1){
        
        // printf("%s\n", line);
        procName = line;
    }

    return procName;
}

int main(){

    readConnection("tcp", NET_FILE);
    scanProcess();

    cout << inodeList.size() << endl;
    // for (int i = 0; i < inodeList.size(); i++){
    //     printf("%s %s %s\n", inodeList[i].pid, inodeList[i].inode, inodeList[i].procName);
    // }

    return 0;

}