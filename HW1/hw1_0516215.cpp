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
    string filter_str;
    regex_t regex_str;
};

vector <struct connInfo> connList;
vector <struct procInfo> inodeList;

struct connInfo parseConnEntry(char *line, int connType);
string ipConvert(string ipAddrWithPort, int connType);
string parseProcName(struct procInfo);
struct optResult handleOptions(int, char**);
int  scanConnection(string connType);
int  scanProcess(uid_t uid);
void outputResult(struct optResult opt_res);
void outputResultWithoutProc(struct optResult opt_res);

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

    // printf("%s %s %s\n", conn.localAddr.c_str(), conn.remoteAddr.c_str(), conn.inode.c_str());
    return conn;
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
    // and discard first arg
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

    if(optind != argc){
        // opt_res.filter_str = argv[optind];

        regex_t regex;
        int regexRes = regcomp(&regex, argv[optind], REG_EXTENDED);
        if(regexRes) {
            fprintf(stderr, "Could not compile regex\n");
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

    const string connPath = NET_FILE + connType;
    // printf("%s\n", connPath.c_str());

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

int scanProcess(uid_t uid){

    regex_t regex;
    int regexRes = regcomp(&regex, "^[0-9]+$", REG_EXTENDED);
    if(regexRes) {
        fprintf(stderr, "Could not compile regex\n");
    }
    
    DIR *dir_ptr = opendir("/proc/");
    if(dir_ptr == NULL){
        printf("Fail to open /proc\n");
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

    // non-root user cannot open fd folders
    if(uid != 0){
        return 1;
    }

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

                string transProto;
                if(connList[i].connType == 0)
                    transProto = "tcp";
                else if (connList[i].connType == 1)
                    transProto = "tcp6";
                else if (connList[i].connType == 2)
                    transProto = "udp";
                else if (connList[i].connType == 3)
                    transProto = "udp6";

                // check if any fields contains the filter_str
                if(opt_res.filter_flag){
                    // if(transProto.find(opt_res.filter_str) != string::npos ||
                    //     connList[i].localAddr.find(opt_res.filter_str) != string::npos ||
                    //     connList[i].remoteAddr.find(opt_res.filter_str) != string::npos ||
                    //     inodeList[j].pid.find(opt_res.filter_str) != string::npos ||
                    //     inodeList[j].procName.find(opt_res.filter_str) != string::npos
                    // ) {
                    //     printf("%-5s %-40s %-40s %s / %s\n", transProto.c_str(), connList[i].localAddr.c_str(), connList[i].remoteAddr.c_str(), inodeList[j].pid.c_str(), inodeList[j].procName.c_str());
                    // }

                    // regex filter
                    int regRes = regexec(&opt_res.regex_str, inodeList[j].procName.c_str(), 0, NULL, 0);
                    if(!regRes) {
                        printf("%-5s %-40s %-40s %s / %s\n", transProto.c_str(), connList[i].localAddr.c_str(), connList[i].remoteAddr.c_str(), inodeList[j].pid.c_str(), inodeList[j].procName.c_str());
                    }
                }
                else{
                    printf("%-5s %-40s %-40s %s / %s\n", transProto.c_str(), connList[i].localAddr.c_str(), connList[i].remoteAddr.c_str(), inodeList[j].pid.c_str(), inodeList[j].procName.c_str());
                }
            }
        }
    }
}

void outputResultWithoutProc(struct optResult opt_res){
    for(size_t i = 0; i < connList.size(); i++){
        connList[i].localAddr  = ipConvert(connList[i].localAddr, connList[i].connType);
        connList[i].remoteAddr = ipConvert(connList[i].remoteAddr, connList[i].connType); 
    }

    bool tcpTitleFlag = false;
    bool udpTitleFlag = false;

    for(size_t i = 0; i < connList.size(); i++){
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

        string transProto;
        if(connList[i].connType == 0)
            transProto = "tcp";
        else if (connList[i].connType == 1)
            transProto = "tcp6";
        else if (connList[i].connType == 2)
            transProto = "udp";
        else if (connList[i].connType == 3)
            transProto = "udp6";

        // filter feature is turned on
        if(opt_res.filter_flag){
            
            // check if any fields contains the filter_str
            if(transProto.find(opt_res.filter_str) != string::npos ||
                connList[i].localAddr.find(opt_res.filter_str) != string::npos ||
                connList[i].remoteAddr.find(opt_res.filter_str) != string::npos
            ) {
                printf("%-5s %-40s %-40s -\n", transProto.c_str(), connList[i].localAddr.c_str(), connList[i].remoteAddr.c_str());
            }
        }
        else {
            printf("%-5s %-40s %-40s -\n", transProto.c_str(), connList[i].localAddr.c_str(), connList[i].remoteAddr.c_str());
        }
    }
}


int main(int argc, char *argv[]){

    uid_t uid = geteuid();
    
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
    
    scanProcess(uid);

    if (uid == 0)
        outputResult(opt_res);
    else 
        outputResultWithoutProc(opt_res);

    return 0;
}