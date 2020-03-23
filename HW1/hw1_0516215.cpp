#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <regex.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <regex>
using namespace std;

const string NET_FILE = "/proc/net/";

struct connInfo {
    int connType;
    string connStr;
    string localAddr;
    string remoteAddr;
    string inode;
};

struct procInfo {
    string pid;
    string procName;
    string inode; 
};

struct optResult {
    bool tcp_flag;
    bool udp_flag;
    bool filter_flag;
    regex_t regex_str;
};

vector <struct connInfo> connList;
vector <struct procInfo> inodeList;

struct connInfo parseConnEntry(char *line, int connType);
string ipConvert(string ipAddrWithPort, int connType);
string parseProcName(struct procInfo);

struct optResult handleOptions(int, char**);
int  scanConnection(string connType);
int  scanProcess();
void outputResult(struct optResult opt_res);

struct connInfo parseConnEntry(char *line, int connType, string connStr){

    struct connInfo conn;
    conn.connType = connType;
    conn.connStr = connStr;

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

    // printf("%s %s %s\n", conn.localAddr.c_str(), conn.remoteAddr.c_str(), conn.inode.c_str());
    return conn;
}

string ipConvert(string ipAddrWithPort, int connType){
    
    string ipAddr = ipAddrWithPort.substr(0, ipAddrWithPort.find(":", 0));
    string port = ipAddrWithPort.substr(ipAddrWithPort.find(":", 0));
    string readableIP;

    // sscanf: convert string port(hex format) to int port
    // to_string: convert int to string, using decimal format
    int portInt;
    sscanf(port.c_str(), ":%X", &portInt);
    port = ":" + to_string(portInt);

    // ipv4
    char ipBuf[INET6_ADDRSTRLEN];
    if(connType == 0 || connType == 2){ 
        
        // convert string ip addr(hex) to int ip addr
        struct in_addr addr;
        sscanf(ipAddr.c_str(), "%X", &addr.s_addr);

        // convert int ip addr to human-readable format
        if( inet_ntop(AF_INET, &addr, ipBuf, INET_ADDRSTRLEN) == NULL ) {
            printf("IP convert failed!\n");
            return "";
        }

    } 
    
    // ipv6
    else if(connType == 1 || connType == 3) { 

        struct in6_addr addr;
        sscanf(ipAddr.c_str(), "%08X%08X%08X%08X", &addr.__in6_u.__u6_addr32[0], &addr.__in6_u.__u6_addr32[1], \
        &addr.__in6_u.__u6_addr32[2], &addr.__in6_u.__u6_addr32[3]);
        
        if( inet_ntop(AF_INET6, &addr, ipBuf, INET6_ADDRSTRLEN) == NULL){
            printf("IP convert failed!\n");
            return "";
        }
    }

    readableIP = ipBuf + port;
    return readableIP;
}

string parseProcName(struct procInfo inodeEntry){
    string commFile = "/proc/" + string(inodeEntry.pid) + "/comm";
    string cmdFile  = "/proc/" + string(inodeEntry.pid) + "/cmdline";
    string procName;
    string procArg;

    FILE*   fd;
    char    *line = NULL;
    size_t  n = 0; 
    
    // Get Process Name
    fd = fopen(commFile.c_str(), "r");
    if (fd == NULL){
        printf("[ERROR] failed to open %s\n", cmdFile.c_str());
        return "";
    }
    while(getline(&line, &n, fd) != -1){
        procName = line;
    }
    fclose(fd);

    // remove trailing '\n'
    if(procName.length() > 0){
        procName.pop_back(); 
    }

    // Get process arguments
    fd = fopen(cmdFile.c_str(), "r");
    if (fd == NULL){
        printf("[ERROR] failed to open %s\n", cmdFile.c_str());
        return "";
    }
    while(getline(&line, &n, fd) != -1){
        procArg = line;
    }
    fclose(fd);

    // change separater of each arg from '\0' to ' '(space), 
    // and discard first arg(i.e. proc path)
    std::replace(procArg.begin(), procArg.end(), '\0', ' ');
    if (procArg.find(" ") == string::npos){
        procArg = "";
    }
    else {
        procArg = procArg.substr(procArg.find(" ", 0)+1);
    }

    return procName + " " + procArg;
}


struct optResult handleOptions(int argc, char *argv[]){

    regex_t regEmpty;
    struct optResult opt_res{0, 0, 0, "", regEmpty};

    int c;
    const char *optFormat = "tu";
    struct option opts[] = {
        {"tcp", 0, NULL, 't'},
        {"udp", 0, NULL, 'u'}
    };

    while( (c = getopt_long(argc, argv, optFormat, opts, NULL)) != -1) {
        switch (c) {
            case 't':
                opt_res.tcp_flag = 1;
                break;
            case 'u':
                opt_res.udp_flag = 1;
                break;
            default:
                printf("Not Valid option! Will ignore it.\n");
        }
    }

    // We got a filter rule
    if(optind != argc){

        // First: concat argv by " ", to get regex_fllter 
        string filter_concat;
        for(int i = optind; i < argc; i++){
            filter_concat += argv[i];
            filter_concat += " ";
        }
        // remove trailing ' '
        if(filter_concat.length() > 0){
            filter_concat.pop_back(); 
        }

        // compile regexp rule
        regex_t regex;
        int regexRes = regcomp(&regex, filter_concat.c_str(), REG_EXTENDED);
        if(regexRes) {
            fprintf(stdout, "Could not compile regex\n");
            exit(EXIT_FAILURE);
            return opt_res;
        }

        opt_res.filter_flag = 1;
        opt_res.regex_str = regex;
    }

    return opt_res;
}

int scanConnection(string connType){

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
    

    FILE    *fp;
    char    *line = NULL;
    size_t  n = 0;

    // Open /proc/net/{tcp, tcp6, udp, udp6}, to get connection infos
    const string connPath = NET_FILE + connType;
    fp = fopen(connPath.c_str(), "r");
    if (fp == NULL){
        printf("Fail to open connection file %s\n", connPath.c_str());
        return -1;
    }

    // build connList table
    int idx = 0;
    while(getline(&line, &n, fp) != -1){
        struct connInfo connEntry;
        if (idx >= 1){
            connEntry = parseConnEntry(line, conn, connType);
            connList.push_back(connEntry);
        }
        idx++;
    }

    return 0;
}

int scanProcess(){

    // regexp for PID folder
    regex_t regex;
    int regexRes = regcomp(&regex, "^[0-9]+$", REG_EXTENDED);
    if(regexRes) {
        fprintf(stderr, "Could not compile regex\n");
    }
    
    // regexp for "socket:[inode]" or [0000]:inode
    std::regex socket_pattern("^socket:\\[([0-9]+)\\]");
    std::regex socket_pattern2("^\\[0000\\]:([0-9]+)");




    DIR *dir_ptr = opendir("/proc/");
    if(dir_ptr == NULL){
        printf("Fail to open /proc\n");
    }
    
    // scan for "pidFolders"
    struct dirent *dirent_ptr; readdir(dir_ptr);  
    int dfd = dirfd(dir_ptr);
    vector <string> pidFolders;
    while( (dirent_ptr = readdir(dir_ptr))  != NULL){

        struct stat statbuf;
        if(fstatat(dfd, dirent_ptr->d_name, &statbuf, 0) == -1) {
            printf("fstatat call error!\n");
        }

        // it is a folder && folder name is PID 
        else if(S_ISDIR(statbuf.st_mode)) {
            int regResult = regexec(&regex, dirent_ptr->d_name, 0, NULL, 0);
            if(!regResult){
                pidFolders.push_back(dirent_ptr->d_name);
            }
        }
    }
    closedir(dir_ptr);

    // scan for "fd folder" inside pidFolders
    for (size_t i = 0; i < pidFolders.size(); i++){

        string fdPath = "/proc/" + pidFolders[i] + "/fd"; 
        
        // a non-root user may not open fd folders that belong to other users
        DIR *dir_ptr = opendir(fdPath.c_str());
        if(dir_ptr == NULL){
            continue;
        }

        // Scan for every "symbolic link" inside /proc/${PID}/fd
        struct dirent *dirent_ptr;
        while( (dirent_ptr = readdir(dir_ptr)) != NULL) {
            
            if( (strcmp("..", dirent_ptr->d_name) != 0) && (strcmp(".", dirent_ptr->d_name) != 0) ){

                string linkPath = fdPath + "/" + dirent_ptr->d_name;
                char buf[512];

                // use fd to access symbolic link
                int linkfd = readlink(linkPath.c_str(), buf, sizeof(buf));
                if (linkfd == -1){
                    printf("Read process link failed\n");
                }

                // TODO: use bettey way to parse inode of each fd
                string fdEntry(buf);
                std::smatch match_groups;
                struct procInfo inodeEntry;

                if(regex_search(fdEntry, match_groups, socket_pattern) || regex_search(fdEntry, match_groups, socket_pattern2)){
                    
                    string inode = match_groups.str(1);
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

void outputResult(struct optResult opt_res){

    for(size_t i = 0; i < connList.size(); i++){
        connList[i].localAddr  = ipConvert(connList[i].localAddr, connList[i].connType);
        connList[i].remoteAddr = ipConvert(connList[i].remoteAddr, connList[i].connType); 
    }

    bool tcpTitleFlag = false;
    bool udpTitleFlag = false;

    for(size_t i = 0; i < connList.size(); i++){
        for(size_t j = 0; j < inodeList.size(); j++){

            // matched netstat entry
            if (connList[i].inode == inodeList[j].inode){
                if(connList[i].connType < 2 && !tcpTitleFlag){
                    tcpTitleFlag = true;
                    printf("\nList of TCP connections: \n");
                    printf("%-5s %-40s %-40s %s\n", "Proto", "Local Address", "Remote Address", "PID / Program names and arguments");
                }

                if(connList[i].connType >= 2 && !udpTitleFlag){
                    udpTitleFlag = true;
                    printf("\nList of UDP connections: \n");
                    printf("%-5s %-40s %-40s %s\n", "Proto", "Local Address", "Remote Address", "PID / Program names and arguments");
                }

                // filter feature is turned on
                if(opt_res.filter_flag){
                    // regex filter
                    int regRes = regexec(&opt_res.regex_str, inodeList[j].procName.c_str(), 0, NULL, 0);
                    if(!regRes) {
                        printf("%-5s %-40s %-40s %s / %s\n", connList[i].connStr.c_str(), connList[i].localAddr.c_str(), connList[i].remoteAddr.c_str(), inodeList[j].pid.c_str(), inodeList[j].procName.c_str());
                    }
                }
                else{
                    printf("%-5s %-40s %-40s %s / %s\n", connList[i].connStr.c_str(), connList[i].localAddr.c_str(), connList[i].remoteAddr.c_str(), inodeList[j].pid.c_str(), inodeList[j].procName.c_str());
                }
            }
        }
    }
}


int main(int argc, char *argv[]){
    
    struct optResult opt_res = handleOptions(argc, argv);

    if(opt_res.tcp_flag){
        scanConnection("tcp");
        scanConnection("tcp6"); 
    }
    if(opt_res.udp_flag){
        scanConnection("udp");
        scanConnection("udp6");
    }
    if(!opt_res.tcp_flag && !opt_res.udp_flag){
        scanConnection("tcp");
        scanConnection("tcp6");
        scanConnection("udp");
        scanConnection("udp6");
    } 
    
    scanProcess();
    outputResult(opt_res);

    return 0;
}