#include"mutex.h"
#include"argpass.h"

#include<stdlib.h>
#include<stdio.h>
#include<sys/file.h>
#include<errno.h>
#include<string.h>

int pid_file;
int rc;

void applicationLock(){
    char* homePath = getHomePath();
    char* defaultLockFile = ".blezz.lock";
    char* lockFile = (char*)malloc((strlen(homePath)+strlen(defaultLockFile)+1)*sizeof(char));
    sprintf(lockFile,"%s/%s",homePath,defaultLockFile);

    pid_file = open(lockFile, O_CREAT | O_RDWR, 0666);
    if (pid_file == -1){
        printf("Failed creating the application lock, make sure the directory %s exist, and is executable by your user.\n", lockFile);
    }
    rc = flock(pid_file, LOCK_EX | LOCK_NB);
}

void applicationRelease(){
    flock(pid_file, LOCK_UN);
}

int applicationFirstInstance(){
    if(rc) { //we could not get a lock
        if(EWOULDBLOCK == errno){
            printf("Application lock is already taken, refuses to start a new instance.\n");
            return 0;
        }
        else{
            return 1;
        }
    }
    else { //we got a lock
        return 1;
    }
}
