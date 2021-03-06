#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <readline/readline.h>
#include "time_functions.h"

#define MAX_BUF 1024

/*
This script will take care of adding the kernel module, adding all LXCs with TDF 0, and stopping the experiment/removing kernel module
Will NOT add simulator LXC or start experiment
*/

//directory containing the 'reader' executable
const char *PATH_TO_READER = "/home/jlamps/lxc-command";
//director containing the 'combined' kernel module (TimeKeeper)
const char *PATH_TO_MODULE = "/home/jlamps/dilation-code";

void createConfigFiles(int num) {
	int i;
	FILE* f;
	char filename[100];
	for (i = 1; i <= num; i++) {
		sprintf(filename, "tmp/lxc-%d.conf", i);
		f = fopen(filename, "w");
		fprintf(f, "lxc.utsname = lxc-%d\n", i);
//		fprintf(f, "lxc.network.type = veth\n");
//		fprintf(f, "lxc.network.flags = up\n");
//		fprintf(f, "lxc.network.link = br-%d\n", i);
//		fprintf(f, "lxc.network.ipv4 = 10.0.0.%d/24\n", i);
		fprintf(f, "lxc.aa_profile = unconfined\n");
		fclose(f);
	}

	f = fopen("tmp/startup.sh", "w");
	fprintf(f, "#!/bin/bash\n");
	for (i = 1; i <= num; i++) {
//		fprintf(f, "brctl addbr br-%d\n", i);
//		fprintf(f, "tunctl -t tap-%d\n", i);
//		fprintf(f, "ifconfig tap-%d 0.0.0.0 promisc up\n", i);
//		fprintf(f, "brctl addif br-%d tap-%d\n", i, i);
//		fprintf(f, "ifconfig br-%d up\n", i);
		fprintf(f, "lxc-create -n lxc-%d -f tmp/lxc-%d.conf\n\n", i, i);
	}
	fclose(f);
	system("chmod +x tmp/startup.sh");

	f = fopen("tmp/teardown.sh", "w");
        fprintf(f, "#!/bin/bash\n");
        for (i = 1; i <= num; i++) {
		fprintf(f, "lxc-stop -n lxc-%d\n", i);
                fprintf(f, "lxc-destroy -n lxc-%d\n", i);
  //              fprintf(f, "ifconfig br-%d down\n", i);
    //            fprintf(f, "brctl delif br-%d tap-%d\n", i, i);
      //          fprintf(f, "brctl delbr br-%d\n", i);
        //        fprintf(f, "ifconfig tap-%d down\n", i);
          //      fprintf(f, "tunctl -d tap-%d\n\n", i);
        }
        fclose(f);
        system("chmod +x tmp/teardown.sh");
}

void startLXCs(int num) {
	int i;
	char filename[100];
	struct timespec tm;
	tm.tv_sec = 0;
	tm.tv_nsec = 100000000L;
	system("tmp/startup.sh");
	for (i = 1; i <= num; i++) {
		sprintf(filename, "lxc-start -n lxc-%d -d %s/reader", i, PATH_TO_READER);
		system(filename);
		printf("starting lxc-%d\n",i);
		nanosleep(&tm, NULL);
	}
}

int main(int argc, char *argv[])
{
	int numberLXCs;
	char *input;
	int i;
	char mypipe[MAX_BUF];
	char command[MAX_BUF]; //command to write the pipe to
	char whichpipe[MAX_BUF]; //which pipe to write to
	char pidsLXCs[MAX_BUF]; //used for finding mappings between pids and lxcs
	int err;
	int lxc;
	int fd;
	int shouldDilate;
	struct timespec tm;
	tm.tv_sec = 0;
	tm.tv_nsec = 150000000L;
	FILE *fp;
	if (argc < 4) {
		printf("Usage: ./command <numberLXCs> <shouldDilate> <pathToLKM>\n");
		return 0;
	}
	numberLXCs = atoi(argv[1]);
	shouldDilate = atoi(argv[2]);
	int pipes[numberLXCs];

	printf("%d STOP %d CONT %d\n", numberLXCs, SIGSTOP, SIGCONT);
	createConfigFiles(numberLXCs);
	startLXCs(numberLXCs);
	printf("After startLXCs\n");

	if (shouldDilate == 1) {
		sprintf(command, "insmod %s/combined.ko", argv[3]);
		system(command);
		//get all containers, add to experiment and set TDF to 0
//perhaps get the reader here, not lxc-start??
		sprintf(pidsLXCs, "pidof reader > tmp/pidof");
		system(pidsLXCs);
		fp = fopen("tmp/pidof", "r");
		for (i = 0; i < numberLXCs; i++) {
			int number;
 			fscanf(fp, "%d", &number);
    			if (feof(fp))
        			break;        // file finished
			printf("The pid is: %d count %d\n",number, i);
			dilate_all(number,0.0);
//			addToExp(number);
			nanosleep(&tm, NULL);
		}
		fclose(fp);
	} //end shouldDilate
	while(1) {
		input = readline ("Enter a command..\n");
		if (strcmp(input, "exit") == 0) {
			printf("Exiting.. \n");
			break;
		}
		else {
			//extract the int, then the command
			char *ptr = strchr(input, ' ');
			if(ptr) {
   				int index = ptr - input;
				*ptr = '\0';
				ptr = ptr+1;
				printf("target LXC: %s, command is: %s\n", input, ptr);
				if ( *input == 'a' && *(input+1) == 'l' && *(input+2) == 'l') {
					for (i = 1; i <= numberLXCs; i++) {
						sprintf(command, "%s",ptr);
						sprintf(whichpipe, "/tmp/lxc-%d" , i);
//						printf("WRITE TO PIPE: %s || RUN COMMAND: %s\n", whichpipe, command);
						fd = open(whichpipe, O_WRONLY | O_NONBLOCK );
						write(fd, command, sizeof(command));
						close(fd);
					}
				}
				else {
				lxc = atoi(input);
				sprintf(command, "%s",ptr);
				sprintf(whichpipe, "/tmp/lxc-%d" , lxc);
//				printf("WRITE TO PIPE: %s || RUN COMMAND: %s\n", whichpipe, command);
				fd = open(whichpipe, O_WRONLY | O_NONBLOCK );
				write(fd, command, sizeof(command));
				close(fd);
				}
			}
			else {
				printf("You wrote: %s, not executing anything\n", input);
			}
		}
	}

	if (shouldDilate == 1) {
		stopExp();
		sleep(10);
		sprintf(command, "rmmod %s/combined", argv[3]);
		system(command);
	}
	system("tmp/teardown.sh");
	for (i = 1; i <= numberLXCs; i++) {
		close(pipes[i-1]);
		sprintf(mypipe, "/tmp/lxc-%d", i);
		unlink(mypipe);
	}

	return 0;
}
