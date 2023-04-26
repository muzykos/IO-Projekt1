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
#include <getopt.h>
#include <string.h>
#include <signal.h>

int sdfilt (const struct dirent *de);
int check_existance (char *str,struct dirent **namelist,int size);

void handlerSIGUSR1(int sigma);


int main(int argc, char *argv[])
{
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
    
    umask(0);

    openlog ("loglog", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);

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
    int sleep_time = 300;
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

    syslog(LOG_INFO,"initialised successfully");
    while(1){
        if((ndir_inp = scandir(argv[1],&namelist_inp, sdfilt, alphasort))<0){
            syslog(LOG_INFO,"failed scandir on folder %s",argv[1]);
            exit(EXIT_FAILURE);
        }
        if((ndir_out = scandir(argv[2],&namelist_out, sdfilt, alphasort))<0){
            syslog(LOG_INFO,"failed scandir on folder %s",argv[2]);
            exit(EXIT_FAILURE);
        }
        syslog(LOG_INFO,"file %s content",argv[1]);
        
        for (it = 0; it < ndir_inp; it++)
            //syslog(LOG_INFO,"  nl[%2zu] %s\n", it, namelist_inp[it]->d_name);
            if((check_existance(namelist_inp[it]->d_name,namelist_out,ndir_out)) == 1){
                syslog(LOG_INFO,"file exists nl[%2zu] %s\n", it, namelist_inp[it]->d_name);
            }else
            {
                syslog(LOG_INFO,"file doesn't exist nl[%2zu] %s\n", it, namelist_inp[it]->d_name);
            }
        sleep(sleep_time);
    }
    exit(EXIT_SUCCESS);
}

int sdfilt (const struct dirent *de){
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
            //syslog(LOG_INFO,"comparing %s and %s",str, namelist[i]->d_name);
            return 1;
        }
    }
    return 0;
}

void handlerSIGUSR1(int sigma){

}