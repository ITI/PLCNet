#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "utility_functions.h"
#include <fcntl.h>    /* For O_RDWR */
 
#include <cstring>

const char *FILENAME = "/proc/dilation/status"; //where TimeKeeper LKM is reading commands
const char *SOCKETHOOK_FILENAME = "/proc/send_hook/send_hook_command";

/*
Sends a specific command to the TimeKeeper Kernel Module. To communicate with the TLKM, you send messages to the location specified by FILENAME
*/
int send_to_timekeeper(char * cmd) {
    FILE *fp;
    //fp = fopen(FILENAME, "a");
    int fd = open(FILENAME,O_WRONLY);
    char new_cmd[200];
    int i = 0;
    int ret = 0;

    for(i = 0; i <  200; i++)
        new_cmd[i] = '\0';

    
    if (fd < 0) {
        perror("Open file to TimeKeeper failed in send to timekeeper");
        //printf("Error communicating with TimeKeeper\n");
        return -1;
    }
    sprintf(new_cmd,"%s,",cmd);
    //ret = fwrite(new_cmd,1,sizeof(new_cmd),fp);
    ret = write(fd,new_cmd,strlen(new_cmd));

    if(ret < 0){
        ret = -10;
        //perror("Write to timekeeper file failed\n");
    }
    //fprintf(fp, "%s,", cmd); //add comma to act as last character
    //fclose(fp);
    close(fd);
    
    return ret;
}

int send_to_socket_hook_monitor(char * cmd) {
    FILE *fp;
    fp = fopen(SOCKETHOOK_FILENAME, "a");
    if (fp == NULL) {
        perror("Open file to Socket Hook failed");
        //printf("Error communicating with Socket hook\n");
        return -1;
    }
    fprintf(fp, "%s", cmd); //add comma to act as last character
    fclose(fp);
    return 0;
}

void start_socket_hook(){

	char command[100];
	sprintf(command,"%c",START_COMMAND);
	send_to_socket_hook_monitor(command);
	

}
void stop_socket_hook(){
	char command[100];
	sprintf(command,"%c",STOP_COMMAND);
	send_to_socket_hook_monitor(command);

}

void add_lxc_to_socket_monitor(int pid, char * lxcName){
	char command[200];
	sprintf(command,"%c %d %s,",ADD_COMMAND,pid,lxcName);
	send_to_socket_hook_monitor(command);


}
int load_lxc_latest_info(char * lxcName){
	char command[200];
	sprintf(command,"%c %s,",LOAD_COMMAND,lxcName);
	return send_to_socket_hook_monitor(command);

}
/*
Returns the thread id of a process
*/
int gettid() {
        return syscall(SYS_gettid);
}

/*
Checks if it is being ran as root or not. All of my code requires root.
*/
int is_root() {
        if (geteuid() == 0)
                return 1;
        printf("Needs to be ran as root\n");
        return 0;

}

/*
Returns 1 if module is loaded, 0 otherwise
*/
int isModuleLoaded() {
        if( access( FILENAME, F_OK ) != -1 ) {
                return 1;
        } else {
                printf("TimeKeeper kernel module is not loaded\n");
                return 0;
        }
}

/*
Converts the double into a integer, makes computation easier in kernel land
ie .5 is converted to -2000, 2 to 2000
*/
int fixDilation(double dilation) {
        int dil;
        if (dilation < 0) {
                printf("Negative dilation does not make sense\n");
                return -1;
        }
        if (dilation < 1.0 && dilation > 0.0) {
                dil = (int)((1/dilation)*1000.0);
                dil = dil*-1;
        }
        else if (dilation == 1.0 || dilation == -1.0) {
                dil = 0;
        }
        else {
                dil = (int)(dilation*1000.0);
        }
        return dil;
}

/*
Given a LXC name, returns the Pid
*/
int getpidfromname(char *lxcname) {
        char command[500];
        char temp_file_name[100];
        FILE *fp;
        int mytid;
        int pid;
        #ifdef SYS_gettid
                mytid = syscall(SYS_gettid);
        #else
                printf("error getting tid\n");
                return -1;
        #endif
        sprintf(temp_file_name, "/tmp/%d.txt", mytid);
        sprintf(command, "lxc-info -n %s | grep pid | tr -s ' ' | cut -d ' ' -f 2 > %s", lxcname, temp_file_name);
        system(command);
        fp = fopen(temp_file_name, "r");
        fscanf(fp, "%d", &pid);
        fclose(fp);
        sprintf(command, "rm %s", temp_file_name);
        system(command);
        return pid;
}

