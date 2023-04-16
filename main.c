#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>

int main(int argc, char const *argv[])
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

    // open log

    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    // change direc to param

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    while(1){
        //do
    }


    exit(EXIT_SUCCESS);
}
