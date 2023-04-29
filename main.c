#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
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
#include <time.h>

#define SHA256_DIGEST_LENGTH 32
#define MAX_LENGTH 1000


//Źródła
//https://4programmers.net/Forum/C_i_C++/289018-daemon_synchronizujacy_dwa_katalogi


int isFileFilter (const struct dirent *de);
int check_existance (char *str,struct dirent **namelist,int size);
void MD5_hash(const char *data, int len, char *md5buf);
void handlerSIGUSR1(int signum);
void copyFileWriteRead(const char* source, const char* target);

// z niechęcią to robie ale niech będzie na razie
volatile int stop_signal = 0;

int main(int argc, char *argv[])
{

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

    // error checking for signal creation
    if(signal(SIGUSR1, handlerSIGUSR1)!=0){
        syslog(LOG_INFO,"failed signal function: %s",errno);
        exit(EXIT_FAILURE);
    }

    //create child
    pid_t pid, sid;
    pid = fork();
    if(pid < 0){
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }
    sid = setsid();
    if (sid < 0) {
        perror("Sid set error");
        syslog(LOG_INFO,"Sid setting error");
        exit(EXIT_FAILURE);
    }

    // change direc to param
    /*
    if(chdir(argv[1]) < 0){
        perror("chdir error");
        syslog(LOG_INFO,"chdir error");
        exit(EXIT_FAILURE);
    }
    */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // specific init
    int sleep_time = 5,timer;
    int c;
    opterr = 0;
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

    struct dirent **namelist_inp = NULL, **namelist_out = NULL;
    int ndir_inp = 0, ndir_out = 0;
    size_t it = 0;

    //required for hashing
    OpenSSL_add_all_algorithms();
    char md5[33];
    char path_source[PATH_MAX];
    char path_target[PATH_MAX];

    syslog(LOG_INFO,"initialised successfully");

    // First scan copying all files to chossen directory
    if((ndir_inp = scandir(argv[1],&namelist_inp, isFileFilter, alphasort))<0){
        syslog(LOG_INFO,"failed scandir on folder %s",argv[1]);
        exit(EXIT_FAILURE);
    }
    for(it = 0; it<ndir_inp; it++){
        if(isFileFilter(namelist_inp[it])){
            //strcat(path_target,argv[2]);
            //strcat(path_target,"/");
            //strcat(path_target,namelist_inp[it]->d_name);
            syslog(LOG_INFO,"path_target %s, argv[2]: %s, namelist: %s", path_target, argv[2], namelist_inp[it]->d_name);
            //copyFileWriteRead(namelist_inp[it]->d_name, path_target);
        }
    }
    

    // The loop
    while(1){
        // scaning for folders content
        if((ndir_inp = scandir(argv[1],&namelist_inp, isFileFilter, alphasort))<0){
            syslog(LOG_INFO,"failed scandir on folder %s",argv[1]);
            exit(EXIT_FAILURE);
        }
        if((ndir_out = scandir(argv[2],&namelist_out, isFileFilter, alphasort))<0){
            syslog(LOG_INFO,"failed scandir on folder %s",argv[2]);
            exit(EXIT_FAILURE);
        }
        syslog(LOG_INFO,"Folders content scanned successfully");


        for (it = 0; it < ndir_inp; it++){
            if(isFileFilter(namelist_inp[it])==1){
                MD5_hash(namelist_inp[it]->d_name, strlen(namelist_inp[it]->d_name), md5);
                //syslog(LOG_INFO,"nazwa: %s hash: %s",namelist_inp[it]->d_name, md5);
            }
            

            if((check_existance(namelist_inp[it]->d_name,namelist_out,ndir_out)) == 1){
                syslog(LOG_INFO,"file exists nl[%2zu] %s\n", it, namelist_inp[it]->d_name);
                
            }else
            {
                syslog(LOG_INFO,"file doesn't exist nl[%2zu] %s\n", it, namelist_inp[it]->d_name);
                
                // todo odczytajplik istniejący i zapisz go w lokalizacji docelowej
                // log o zapisaniu pliku
            }
        }
        timer = 0;
        syslog(LOG_INFO,"Attempting Loop");
        while(stop_signal==0 && timer < sleep_time){
            sleep(1);
            timer++;
        }
        if(stop_signal!=0){
            stop_signal = 0;
        }
    }
    exit(EXIT_SUCCESS);
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
    syslog(LOG_INFO,"coping");
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
    if(fsource==-1){syslog(LOG_INFO, "Could not open source file: %s", perror);exit(EXIT_FAILURE);}
    if(ftarget==-1){syslog(LOG_INFO, "Could not open target file: %s", perror);exit(EXIT_FAILURE);}

    unsigned char buffer[MAX_LENGTH]; // buffer 
    size_t bytes_read; // number of bytes remaning to be writeen
    int writtenChars; // number of butes writeen already
    char *bp; // pointer into write buffer
    while(1){
        bytes_read = read(fsource, buffer, sizeof(buffer));
        if(bytes_read>0){
            bp = buffer;
            while(bytes_read>0){
                if((writtenChars = write(ftarget, bp, bytes_read))<0){
                    syslog(LOG_INFO, "Write to file Failed: %s", perror);
                    exit(EXIT_FAILURE);
                }
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

int lastModificationDate();

void copyFolderContent(const char* source, const char* target){

}
