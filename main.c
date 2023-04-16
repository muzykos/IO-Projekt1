#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>
#include <syslog.h>

int main(int argc, char *argv[])
{
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
    if(chdir(argv[1]) < 0){
        perror("chdir error");
        syslog(LOG_INFO,"chdir error");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // specific init
    int sleep_time = 300;
    int c;
    opterr = 0;
    while ((c = getopt(argc,argv,"t:")) != -1)
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
     

    while(1){
        sleep(sleep_time);
    }


    exit(EXIT_SUCCESS);
}
