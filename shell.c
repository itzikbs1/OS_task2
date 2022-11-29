/**
 * @file Shell.c
 * @author Shaked Levi, Lior Nagar
 * @brief This program is a Shell project, it will have some basic options like the shell does.
 * includes -> pwd,ls,cd ..., for further details ,enter the "Help" keyword in the program.
 * @version 0.1
 * @date 2022-03-21
 * 
 * @copyright Copyright (c) 2022
 * 
 */

// #include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#define SA struct sockaddr

#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#include <dirent.h> // for directory files 
// #include "server.c"
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
/* method to clear the shell. */
#define clear() printf("\033[H\033[J]")

/*
use with redirection(< > ) to indicate to the function in which mode to open the file
and to know that redirection of the input OR output has to be done
*/
#define INPUT 0
#define OUTPUT 1
#define APPEND 2

/* Globals */
#define TRUE 1
#define FALSE 0
#define MAXSIZE 256
int cli_sock;




void func(int connfd)
{
    char buff[80];
    int n;
    for (;;) {
        bzero(buff, 80);
   
        read(connfd, buff, sizeof(buff));
        printf("From client: %s\t To client: ", buff);
        bzero(buff, 80);
        n = 0;
        while ((buff[n++] = getchar()) != '\n');
   
        write(connfd, buff, sizeof(buff));
   
        if (strncmp("exit", buff, 4) == 0) {
            printf("Server Exit...\n");
            break;
        }
    }
}

/*
    removes the newline and space character from the end and start of a char*
*/
void removeWhiteSpace(char* buf){
	if(buf[strlen(buf)-1]==' ' || buf[strlen(buf)-1]=='\n')
	buf[strlen(buf)-1]='\0';
	if(buf[0]==' ' || buf[0]=='\n') memmove(buf, buf+1, strlen(buf));
}

/*
    tokenizes char* buf using the delimiter c, and returns the array of strings in param
    and the size of the array in pointer nr
*/
void tokenize_buffer(char** param,int *nr,char *buf,const char *c){
	char *token;
	token=strtok(buf,c);
	int pc=-1;
	while(token){
		param[++pc]=malloc(sizeof(token)+1);
		strcpy(param[pc],token);
		removeWhiteSpace(param[pc]);
		token=strtok(NULL,c);
	}
	param[++pc]=NULL;
	*nr=pc;
}

///////////////////// DIR - Q1 //////////////////////

void show_library_files(){

    DIR *folder_contents;
    struct dirent *file_name;
    int idx = 1;
    struct stat is_runnable;

    folder_contents = opendir(".");
    if(folder_contents == NULL){
        printf("Unable to read from directory");
        return;
    }else{
        while( (file_name = readdir(folder_contents))){
            if (stat(file_name->d_name, &is_runnable) == 0 && is_runnable.st_mode & S_IXUSR){ 
                if(strncmp(file_name->d_name,".",1) == 0){
                    printf("%s\n",file_name->d_name);
                }else{
                    printf("%s\n",file_name->d_name);
                }
            }
            else {
                printf("%s\n",file_name->d_name);
            }
                
            
            idx++;
        }
    }

    free(file_name);
    closedir(folder_contents);
}

//////////////// COPY - Q2 //////////////////////

void copy_from_src_to_dst(char *src, char* dst){
    size_t size;
    char buf[BUFSIZ];
    FILE *src_file = fopen(src,"r");
    if(src_file == NULL){
        printf("Failed to open src file..\n");
        return;
    }
    FILE *dst_file = fopen(dst,"w");
    if(dst_file == NULL){
        printf("Failed to open dest file..\n");
        return;
    }
    //reading and writing to the new file.
    size = fread(buf, 1, BUFSIZ, src_file);
    while (size > 0) {
        fwrite(buf, 1, size, dst_file);
        size = fread(buf, 1, BUFSIZ, src_file);
    }
    free(src);
    free(dst);

    fclose(src_file);
    fclose(dst_file);

}

////////////////////// ORIGINAL LINUK COMMAND -Q3 ////////////// 

void original(char **buf){//check what its do.

    /* forks a new child */
    int process_id = fork();

    if(process_id < 0) {
        /* error case */
        return; 
    }else if (process_id == 0){
        /* child process */
        execvp(buf[0], buf);
    }else{   
        /* Parent Process */
        wait(NULL);
    }
    return;
}


//////////////////// PIPE - Q4a and Q4b ////////////////

void executePiped(char** buf,int nr){ //can support up to 8 piped commands (CHACK IF ITS OK FOR NEZER)
	if(nr>8)
         return;
	
	int fd[8][2],i,pc;
	char *argv[100];

	for(i=0;i<nr;i++){
		tokenize_buffer(argv,&pc,buf[i]," ");
		if(i!=nr-1){
			if(pipe(fd[i])<0){
				perror("pipe creating was not successfull\n");
				return;
			}
		}
		if(fork()==0){//child1
			if(i!=nr-1){
				dup2(fd[i][1],1);
				close(fd[i][0]);
				close(fd[i][1]);
			}

			if(i!=0){
				dup2(fd[i-1][0],0);
				close(fd[i-1][1]);
				close(fd[i-1][0]);
			}
			execvp(argv[0],argv);
			perror("invalid input ");
			exit(1);//in case exec is not successfull, exit
		}
		//parent
		if(i!=0){//second process
			close(fd[i-1][0]);
			close(fd[i-1][1]);
		}
		wait(NULL);
	}
}



////////////////////// REDIRECTION -Q5 ////////////// 

void executeRedirect(char** buf,int nr,int mode){
	int pc,fd;
	char *argv[100];
	removeWhiteSpace(buf[1]);
	tokenize_buffer(argv,&pc,buf[0]," ");
	if(fork()==0){

		switch(mode){
		case INPUT:  fd=open(buf[1],O_RDONLY); break;
		case OUTPUT: fd=open(buf[1],O_WRONLY); break;
		case APPEND: fd=open(buf[1],O_WRONLY | O_APPEND); break;
		default: return;
		}

		if(fd<0){
			perror("cannot open file\n");
			return;
		}

		switch(mode){
		case INPUT:  		dup2(fd,0); break;
		case OUTPUT: 		dup2(fd,1); break;
		case APPEND: 		dup2(fd,1); break;
		default: return;
		}
		execvp(argv[0],argv);
		perror("invalid input ");
		exit(1);//in case exec is not successfull, exit
	}
	wait(NULL);
}




//////////////////////// TCP - Q6 ////////////////////

void open_tcp_socket(char * port, char * IP){

    struct sockaddr_in server_addr;

    cli_sock = socket(AF_INET, SOCK_STREAM,0);
    if( cli_sock < 0 ) {
        printf("Socket creation failed, exiting method...\n");
        sleep(1);
        return;

    }else{
        printf("Socket creation was a success..\n");
    }

    bzero(&server_addr, sizeof(server_addr)); // reset the buff.
    unsigned int val = strtoul(port, port + strlen(port), 10);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(IP); // ip that the user put
    server_addr.sin_port = htons(val);
    
    int con = connect(cli_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (con != 0){
        printf("Connection with server failed, exiting method...\n");
        sleep(1);
        return;
    }else{
        printf("Connection with the server was a success..\n");
    }
}

void main(){

    char buf[256],*buffer[256],*buffer2[256], *params1[100],cwd[256];
	int index=0;
	printf("***********************************************************************************"    "\n");
	printf("************************** WALCOME TO GAL & ITZIK SHELL ***************************"    "\n");

	while(1){
		printf("Enter 'exit' to exit):""\n");

		//print current Directory
		if (getcwd(cwd, sizeof(cwd)) != NULL){
		    printf( "%s  " , cwd);
        }
		else{
            perror("getcwd failed\n");

        } 	

		//read user input
		fgets(buf,256,stdin);//buffer overflow cannot happen

		if(strchr(buf,'|')){//tokenize pipe commands
			tokenize_buffer(buffer,&index,buf,"|");
			executePiped(buffer,index);
		}

		else if(strchr(buf,'>')){//redirect output to file
			tokenize_buffer(buffer,&index,buf,">");
			if(index==2){
                executeRedirect(buffer,index, OUTPUT);
            }
			else{ 
                printf("Incorrect output redirection!");
            }
		}

		else if(strchr(buf,'<')){//redirect file to input
			tokenize_buffer(buffer,&index,buf,"<");
			if(index==2){
                executeRedirect(buffer,index, INPUT);
            }
			else{
                 printf("Incorrect input redirection!(has to to be in this form: command < file)");
            }
		}
        else if(strchr(buf,'}')){  // open a tcp port
        	tokenize_buffer(buffer,&index,buf," ");
            tokenize_buffer(buffer2,&index,buffer[2],":");

            open_tcp_socket(buffer2[1], buffer2[0]);
            dup2(1234,1);

			if(strstr(buffer[0],"DIR")){
                show_library_files();
			}
            else if(strstr(buffer[0],"COPY")){
				copy_from_src_to_dst(buffer[1],buffer[2]);        

		    }
			else if(strstr(buffer[0],"exit")){//exit builtin command
				exit(0);
			}
			else original(buffer); // everything else command 

        }
        else if(strchr(buf,'{')){  // open a sever port
            tokenize_buffer(buffer,&index,buf," ");
            char * port = buffer[1];

            unsigned int val = strtoul(port, port + strlen(port), 10);
            int sockfd, connfd, len;
            struct sockaddr_in servaddr, cli;
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd == -1) {
                printf("socket creation failed...\n");
                exit(0);
            }
            else
                printf("Socket successfully created..\n");                
            bzero(&servaddr, sizeof(servaddr));
            
            servaddr.sin_family = AF_INET;
            servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
            servaddr.sin_port = htons(val);
            
            if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
                printf("socket bind failed...\n");
                exit(0);
            }
            else
                printf("Socket successfully binded..\n");
        
            if ((listen(sockfd, 5)) != 0) {
                printf("Listen failed...\n");
                exit(0);
            }
            else
                printf("Server listening..\n");
            len = sizeof(cli);
            
            connfd = accept(sockfd, (SA*)&cli, &len);
            if (connfd < 0) {
                printf("server accept failed...\n");                    
                exit(0);
            }
            else
                printf("server accept the client...\n");
            
            func(connfd);
            
            close(sockfd);
            dup2(1,1234);
            dup2(cli_sock,1);
        }

		else{
			tokenize_buffer(params1,&index,buf," ");
			if(strstr(params1[0],"DIR")){
                show_library_files();
			}
            else if(strstr(params1[0],"COPY")){
				copy_from_src_to_dst(params1[1],params1[2]);        

		    }
			else if(strstr(params1[0],"exit")){//exit builtin command
				exit(0);
			}
			else original(params1); // everything else command 

		}
	}

}
