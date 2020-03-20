#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <regex.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

const string NET_FILE = "/proc/net/";


struct connInfo {
    int connType;
    string localAddr;
    string remoteAddr;
    string inode;
};

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

string ipConvert(string ipAddrWithPort, int connType){
    
    string ipAddr = ipAddrWithPort.substr(0, ipAddrWithPort.find(":", 0));
    string port = ipAddrWithPort.substr(ipAddrWithPort.find(":", 0));
    string readableIP;

    int portInt;
    sscanf(port.c_str(), ":%d", &portInt);
    
    if(connType == 0 || connType == 2){ // ipv4
        
        char dstIP[INET_ADDRSTRLEN];
        struct in_addr addr;

        sscanf(ipAddr.c_str(), "%X", &addr.s_addr);

        if( inet_ntop(AF_INET, &addr, dstIP, INET_ADDRSTRLEN) == NULL ) {
            printf("IP convert failed!\n");
            return "";
        }
        port = ":" + to_string(portInt);
        readableIP = dstIP + port;
    } 
    
    // ipv6
    else if(connType == 1 || connType == 3) { 

        char dstIP[INET6_ADDRSTRLEN];
        struct in6_addr addr;
        
        sscanf(ipAddr.c_str(), "%08X%08X%08X%08X", &addr.__in6_u.__u6_addr32[0], &addr.__in6_u.__u6_addr32[1], \
        &addr.__in6_u.__u6_addr32[2], &addr.__in6_u.__u6_addr32[3]);
        
        if( inet_ntop(AF_INET6, &addr, dstIP, INET6_ADDRSTRLEN) == NULL){
            printf("IP convert failed!\n");
            return "";
        }
        port = ":" + to_string(portInt);
        readableIP = dstIP + port;
    }

    // cout << readableIP << endl;
    return readableIP;
}

void outputResult(){

    for(size_t i = 0; i < connList.size(); i++){
        connList[i].localAddr  = ipConvert(connList[i].localAddr, connList[i].connType);
        connList[i].remoteAddr = ipConvert(connList[i].remoteAddr, connList[i].connType); 
    }

    //TODO: struct sort by connType!!! =. SKIP


    bool udpStartFlag = false;


    printf("\nList of TCP connections: \n");
    printf("%-5s %-40s %-40s %s\n", "Proto", "Local Address", "Remote Address", "pid/Program names and arguments");

    for(size_t i = 0; i < connList.size(); i++){
        for(size_t j = 0; j < inodeList.size(); j++){
            
            if(connList[i].connType >= 2 && !udpStartFlag){
                udpStartFlag = true;
                printf("\nList of UDP connections: \n");
                printf("%-5s %-40s %-40s %s\n", "Proto", "Local Address", "Remote Address", "pid/Program names and arguments");
            }

            if (connList[i].inode == inodeList[j].inode){
                
                string transProto;
                if(connList[i].connType == 0)
                    transProto = "tcp";
                else if (connList[i].connType == 1)
                    transProto = "tcp6";
                else if (connList[i].connType == 2)
                    transProto = "udp";
                else if (connList[i].connType == 3)
                    transProto = "udp6";

                printf("%-5s %-40s %-40s %s %s\n", transProto.c_str(), connList[i].localAddr.c_str(), connList[i].remoteAddr.c_str(), inodeList[j].pid.c_str(), inodeList[j].procName.c_str());
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
    readConnection("tcp6", NET_FILE);
    readConnection("udp", NET_FILE);
    readConnection("udp6", NET_FILE);


    scanProcess();

    outputResult();

    return 0;
}