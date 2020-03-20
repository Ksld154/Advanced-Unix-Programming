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
    int connType;
    string localAddr;
    string remoteAddr;
    string inode;
};

// struct procInfo {
//     const char* pid;
//     const char* procName;
//     const char* inode; 
// };

struct procInfo {
    string pid;
    string procName;
    string inode; 
};

vector <struct connInfo> connList;
vector <struct procInfo> inodeList;

// struct ConnInfo parseConnEntry(char *, int);
string parseProcName(struct procInfo);

struct connInfo parseConnEntry(char *line, int connType){

    struct connInfo conn;
    conn.connType = connType;

    int idx = 0;
    char* substr = strtok(line, " ");
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

    printf("%s %s %s\n", conn.localAddr.c_str(), conn.remoteAddr.c_str(), conn.inode.c_str());
    // connList.push_back(conn);
    return conn;
}

int readConnection(string connType, const string NET_FILE){

    int conn = -1;
    if(connType == "tcp"){
        conn = 0;
    }
    else if(connType == "tcp6"){
        conn = 1;
    }
    else if(connType == "udp"){
        conn = 2;
    }
    else if(connType == "udp6"){
        conn = 3;
    }

    const string connPath = NET_FILE + connType;
    printf("%s\n", connPath.c_str());

    FILE    *fp;
    char    *line = NULL;
    size_t  n = 0; 
    fp = fopen(connPath.c_str(), "r");
    if (fp == NULL){
        exit(EXIT_FAILURE);
    }

    int idx = 0;
    while(getline(&line, &n, fp) != -1){
        struct connInfo connEntry;
        if (idx >= 1){
            connEntry = parseConnEntry(line, conn);
            connList.push_back(connEntry);
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
    
    // scan for pidFolders
    vector <string> pidFolders;
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
        // printf("\n%s\n", fdPath.c_str());
        
        DIR *dir_ptr = opendir(fdPath.c_str());
        if(dir_ptr == NULL){
            printf("Fail to open %s\n", fdPath.c_str());
            // exit(EXIT_FAILURE);
        }

        struct dirent *dirent_ptr;
        while( (dirent_ptr = readdir(dir_ptr)) != NULL) {

            if( (strcmp("..", dirent_ptr->d_name) != 0) && (strcmp(".", dirent_ptr->d_name) != 0) ){

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

                    // inodeEntry.inode = inode.c_str();
                    // inodeEntry.pid = pidFolders[i].c_str();
                    // string procName = parseProcName(inodeEntry);
                    // inodeEntry.procName = procName.c_str();

                    inodeEntry.inode = inode;
                    inodeEntry.pid = pidFolders[i];
                    inodeEntry.procName = parseProcName(inodeEntry);


                    // printf("%s %s %s\n", inodeEntry.pid.c_str(), inodeEntry.inode.c_str(), inodeEntry.procName.c_str());
                    inodeList.push_back(inodeEntry);
                }
            }
        }
        closedir(dir_ptr);
    }
    return 0;
}

void outputResult(){

    for(size_t i = 0; i < connList.size(); i++){
        for(size_t j = 0; j < inodeList.size(); j++){
            
            if (connList[i].inode == inodeList[j].inode){
                printf("%d %s %s %s %s\n", connList[i].connType, connList[i].localAddr.c_str(), connList[i].remoteAddr.c_str(), inodeList[j].pid.c_str(), inodeList[j].procName.c_str());
            }
        }
    }
}

string parseProcName(struct procInfo inodeEntry){
    string cmdFile = "/proc/" + string(inodeEntry.pid) + "/cmdline";
    string procName;

    FILE*   fd;
    char    *line = NULL;
    size_t  n = 0; 
    
    fd = fopen(cmdFile.c_str(), "r");
    if (fd == NULL){
        printf("[ERROR] failed to open %s\n", cmdFile.c_str());
        return "";
        // exit(EXIT_FAILURE);
    }

    while(getline(&line, &n, fd) != -1){
        
        procName = line;
    }

    fclose(fd);
    return procName;
}

int main(){

    readConnection("tcp", NET_FILE);

    scanProcess();

    outputResult();
    // cout << inodeList.size() << endl;
    // for (size_t i = 0; i < inodeList.size(); i++){
    //     printf("%s %s %s\n", inodeList[i].pid.c_str(), inodeList[i].inode.c_str(), inodeList[i].procName.c_str());

    //     // printf("%s %s\n", inodeList[i].pid, inodeList[i].inode);
    // }

    return 0;

}