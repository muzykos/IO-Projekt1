#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <linux/limits.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <stdbool.h> 
#include <time.h>


#define SHA256_DIGEST_LENGTH 32
#define MAX_LENGTH 1000


//Źródła
//https://4programmers.net/Forum/C_i_C++/289018-daemon_synchronizujacy_dwa_katalogi


int isFileFilter (const struct dirent *de);
int check_existance (char *str,struct dirent **namelist,int size);
int getFileCreationTime(const char *path);
void MD5_hash(const char *data, int len, char *md5buf);
void handlerSIGUSR1(int signum);
void addFileToPath(char* path, const char* source, const char *filename);
void copyFileWriteRead(const char* source, const char* target);
void SleepFun(int sleep_time);
bool copyFolderContent(const char* source, const char* target);
bool CheckIfFirstFileISYounger(const char *path1, const char *path2);
bool CheckModifications(const char* source, const char* scan);
bool CheckNewFiles(const char* source, const char* scan);

// global value for breaking sleep loop
volatile int stop_signal = 0;
// recursive option flag
volatile int recursive_option = 0;

int main(int argc, char *argv[])
{
    time_t rawtime;
    time( &rawtime );
    umask(0);
    openlog ("loglog", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
    
    //check argument amount
    if (argc < 3)
    {
        perror("argument amount error");
        exit(EXIT_FAILURE);
    }
    //check whether argument 2 is a directory
    DIR* dir1 = opendir(argv[1]);
    if(dir1){
    }else if(ENOENT == errno){
        perror("directory 1 doesn't exist");
        exit(EXIT_FAILURE);
    }else{
        perror("can't access directory 1");
        exit(EXIT_FAILURE);
    }
    //check whether arg 3 is directory
    DIR* dir2 = opendir(argv[2]);
    if(dir2){
    }else if(ENOENT == errno){
        perror("directory 2 doesn't exist");
        exit(EXIT_FAILURE);
    }else{
        perror("can't access directory 2");
        exit(EXIT_FAILURE);
    }

    char source_full_path[PATH_MAX+1];
    char target_full_path[PATH_MAX+1];
    if(!realpath(argv[1], source_full_path)){
        perror("Path problem");
        exit(EXIT_FAILURE);
    }
    if(!realpath(argv[2], target_full_path)){
        perror("Path problem");
        exit(EXIT_FAILURE);
    }

    // error checking for signal creation
    if(signal(SIGUSR1, handlerSIGUSR1)!=0){
        syslog(LOG_INFO,"failed signal function: %s",errno);
        exit(EXIT_FAILURE);
    }

    //create child
    if(fork()!=0){
        syslog(LOG_INFO,"Forking falied");
        exit(EXIT_FAILURE);
    }
    if (setsid() < 0) {
        perror("Sid set error");
        syslog(LOG_INFO,"Sid setting error");
        exit(EXIT_FAILURE);
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // specific init
    int sleep_time = 5,c;
    while ((c = getopt(argc,argv,"t:")) != -1){
        switch (c)
        {
        case 't':
            syslog(LOG_INFO,"Set Sleeptime to %d seconds",atoi(optarg));
            sleep_time = atoi(optarg);
            break;
        case '?':
            if(optopt == 't')
                syslog(LOG_INFO, "Option -%c requires argument. ",optopt);
            else if(isprint(optopt))
                syslog(LOG_INFO, "Unknown opt '-%c'.",optopt);
            else
                syslog(LOG_INFO, "unknown opt char '\\x%x'",optopt);
            exit(EXIT_FAILURE);
        default:
            exit(EXIT_FAILURE);
        }
    }

    //required for hashing
    OpenSSL_add_all_algorithms();
    char md5[33];
    

    // copying all files to choosen directory on startup
    // this works 100%
    if(0!=copyFolderContent(source_full_path, target_full_path)){
        syslog(LOG_INFO,"Error while handling folder copying");
        exit(EXIT_FAILURE);
    }
    
    syslog(LOG_INFO,"initialised successfully");

    // The loop
    while(1){
        if(1==CheckModifications(source_full_path, target_full_path)){
            syslog(LOG_INFO,"Function CheckModifaction() Failed");
            exit(EXIT_FAILURE);
        }
        if(1==CheckNewFiles(source_full_path, target_full_path)){
            syslog(LOG_INFO,"Function CheckModifaction() Failed");
            exit(EXIT_FAILURE);
        }
        SleepFun(sleep_time);
    }
    return 0;
}


void SleepFun(int sleep_time){
    int timer = 0;
    while(stop_signal==0 && timer < sleep_time){
        sleep(1);
        timer++;
    }
    if(stop_signal!=0){
        stop_signal = 0;
    }
}

int isFileFilter (const struct dirent *de){
    if (strcmp (de->d_name, ".") == 0 || strcmp (de->d_name, "..") == 0)
        return 0;
    else
        return 1;
}

int check_existance (char *str,struct dirent **namelist,int size){
    size_t i = 0;
    for ( i = 0; i < size; i++)
    {
        
        if ((strcmp(str,namelist[i]->d_name))==0)
        {
            return 1;
        }
    }
    return 0;
}

void handlerSIGUSR1(int signum){
    syslog(LOG_INFO,"SIGUSR1 handler activation");
    stop_signal = 1;
}

void MD5_hash(const char *data, int len, char *md5buf){
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_md5();
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len, i;
    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, data, len);
    EVP_DigestFinal_ex(mdctx, md_value, &md_len);
    EVP_MD_CTX_free(mdctx);
    for (i = 0; i < md_len; i++) {
        snprintf(&(md5buf[i * 2]), 16 * 2, "%02x", md_value[i]);
    }
}

void copyFileWriteRead(const char* source, const char* target){
    
    syslog(LOG_INFO,"Copying file: %s", source);
    if(source==NULL){
        syslog(LOG_INFO,"Passed source path doesnt exist");
        exit(EXIT_FAILURE);
    }
    if(target==NULL){
        syslog(LOG_INFO,"Passed target path doesnt exist");
        exit(EXIT_FAILURE);
    }
    
    int fsource = open(source, O_RDONLY);
    int ftarget = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    //syslog(LOG_INFO, "log %s OO %s", source, target);
    if(fsource==-1){
        syslog(LOG_INFO, "Could not open source file");
        exit(EXIT_FAILURE);
    }
    if(ftarget==-1){
        syslog(LOG_INFO, "Could not open target file");
        exit(EXIT_FAILURE);
    }
    unsigned char buffer[MAX_LENGTH]; // buffer 
    size_t bytes_read; // number of bytes remaning to be writeen
    int writtenChars; // number of butes writeen already
    char *bp; // pointer into write buffer
    while(1){
        strcpy(buffer, "");
        bytes_read = read(fsource, buffer, sizeof(buffer));
        if(bytes_read>0){
            bp = buffer;
            while(bytes_read>0){
                if((writtenChars = write(ftarget, bp, bytes_read))<0){
                    syslog(LOG_INFO, "Write to file Failed: %s", perror);
                    exit(EXIT_FAILURE);
                }
                //syslog(LOG_INFO, "Bytes read value: %d", bytes_read);
                bytes_read-=writtenChars;
                bp+=writtenChars;
            }
        }
        else if(bytes_read == 0){break;}
        else{
            syslog(LOG_INFO, "Write to file Failed: %s", perror);
            exit(EXIT_FAILURE);
        }   
    }
    syslog(LOG_INFO,"Succesful copying of a file: %s", source);
    close(fsource);
    close(ftarget);
    return;
}

void addFileToPath(char* path, const char* source, const char *filename){
        strcpy(path, "");           //clearing path
        strcat(path, source);       // adding source folder
        strcat(path, "/");          // adding '/'
        strcat(path, filename);     // adding filename to path
}

bool copyFolderContent(const char* source, const char* target){
    DIR *dir;
    struct dirent *dp;
    char path_src[PATH_MAX+1];
    char path_trg[PATH_MAX+1];
    if((dir = opendir(source)) == NULL){
        syslog(LOG_INFO, "Dictionary doesn't exist: %s", source);
        return 1;
    }
    while((dp = readdir(dir))!=NULL){
        if(isFileFilter(dp)){
            addFileToPath(path_src,source,dp->d_name);
            addFileToPath(path_trg,target,dp->d_name);
            //syslog(LOG_INFO, "src %s trg: %s", path_src, path_trg);
            copyFileWriteRead(path_src, path_trg);
        }   
    }
    return 0;
}

bool CheckModifications(const char* source, const char* scan){
    DIR *dir_scan;
    struct dirent *dp_scan, **dp_src = NULL;
    int dir_src, c1, c2, file_found;
    char path_src[PATH_MAX+1], path_scan[PATH_MAX+1];
    
    if((dir_scan = opendir(scan)) == NULL){
        syslog(LOG_INFO, "Dictionary doesn't exist: %s", dir_scan);
        return 1;
    }
    if((dir_src = scandir(source,&dp_src, isFileFilter, alphasort))<0){
        syslog(LOG_INFO,"failed scandir on folder %s",source);
        return 1;
    }  

    while((dp_scan = readdir(dir_scan))!=NULL){
        if(isFileFilter(dp_scan)){
            file_found = 0;
            for(c1 = 0; c1<dir_src;c1++){
                //syslog(LOG_INFO, "%s, %s", dp_scan->d_name, dp_src[c1]->d_name);
                if(isFileFilter(dp_src[c1])&&(strcmp(dp_scan->d_name, dp_src[c1]->d_name)==0)){
                    //syslog(LOG_INFO, "FOUND: %s, %s", dp_scan->d_name, dp_src[c1]->d_name);
                    file_found = 1;
                    addFileToPath(path_src,source,dp_src[c1]->d_name);  // adding files names to paths
                    addFileToPath(path_scan,scan,dp_scan->d_name);
                    if(!CheckIfFirstFileISYounger(path_src, path_scan)){ // checking which file is younger
                        syslog(LOG_INFO,"File %s was modificated, copying file from source", dp_scan->d_name);
                        copyFileWriteRead(path_src, path_scan);         // if src is younger them coping it to scan 
                    }
                    break;
                }
            }
            if(file_found==0){
                syslog(LOG_INFO,"File %s was not found in the source, removing it from watch", dp_scan->d_name);
                addFileToPath(path_scan,scan,dp_scan->d_name);
                if(remove(path_scan)==-1){
                    syslog(LOG_INFO, "File deletion failed"); 
                    return 1;
                }
            }
        }
        else{

        }
    }
    return 0;
}

bool CheckNewFiles(const char* source, const char* scan){
    DIR *dir_src;
    struct dirent *dp_src, **dp_scan=NULL;
    int dir_scan, c1, file_found;
    char path_src[PATH_MAX+1], path_scan[PATH_MAX+1];
    if((dir_src = opendir(source)) == NULL){
        syslog(LOG_INFO, "Dictionary doesn't exist: %s", source);
        return 1;
    }
    if((dir_scan = scandir(scan,&dp_scan, isFileFilter, alphasort))<0){
        syslog(LOG_INFO,"failed scandir on folder %s",scan);
        return 1;
    } 
    while((dp_src = readdir(dir_src))!=NULL){
        if(isFileFilter(dp_src)){
            file_found = 0;
            for(c1=0;c1<dir_scan;c1++){
                if(isFileFilter(dp_scan[c1])){
                    if(strcmp(dp_scan[c1]->d_name, dp_src->d_name)==0){
                        file_found = 1;
                        break;
                    }
                }
            }
        }
        if(file_found==0){
            addFileToPath(path_src,source,dp_src->d_name);
            addFileToPath(path_scan,scan,dp_src->d_name);
            syslog(LOG_INFO, "File %s was not saved in scans, adding it to watch", dp_src->d_name);
            copyFileWriteRead(path_src, path_scan);
        }
    }
    
    return 0;
}

bool CheckIfFirstFileISYounger(const char *path1, const char *path2){
    struct stat attr1, attr2;
    stat(path1, &attr1);
    stat(path2, &attr2);
    time_t modify_time1 = attr1.st_mtime; 
    time_t modify_time2 = attr2.st_mtime; 
    double seconds = difftime(modify_time1, modify_time2);
    if(seconds > 0){return 0;}
    else{return 1;}
}