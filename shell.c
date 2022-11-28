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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#include <dirent.h> // for directory files 

// #include "Shell.h"

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<readline/readline.h>
#include<readline/history.h>
/* method to clear the shell. */
#define clear() printf("\033[H\033[J]")

/* Colors */
#define GREEN "\033[0;32m"
#define PURPLE "\033[0;35m"
#define WHITE "\033[0m"

/*
use with redirection(< > >>) to indicate to the function in which mode to open the file
and to know that redirection of the input OR output has to be done
*/
#define INPUT 0
#define OUTPUT 1
#define APPEND 2

/* Globals */
#define TRUE 1
#define FALSE 0
#define MAXSIZE 256
#define PORT 5555
#define LOCALHOST "0.0.0.0"
int cli_sock;
int tcp_connections = 0; // i use this to not let the user enter LOCAL only after the client opened a tcp connection

void shell_initialize(){
    clear();
    
    // user greetings : 
    printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    // user handle :
    char* username = getenv("USER");
    printf("\n\n\nUSER:@%s", username);
    printf("\n");
    sleep(3); // TODO: make a bind to enter, in order to acess shell.
    clear();

}

void user_input(char *input){

        if(strcmp(input,"DIR") == 0){
            show_library_files();
            return;
        }
        //handle copy case:
        //this line checks if a string starts with 'COPY'
        if(strncmp(input,"COPY",4) == 0){
            copy_from_src_to_dst(input);
            return;     
        }
        // char left[MAXSIZE];
        // int idx=0;
        // int size = strlen(input);
        // while ((*input) != ' ')
        // {
        //     left[idx] = (*input);
        //     idx++;
        //     input++;
        // }
        // input++;
        // printf("left %s", left);
        // printf("input %s ", input);
        // if(strncmp(input, "|", 1) == 0){
        //     // input += 2;
        //     execArgsPiped(left, input);
        // }
        // if(strncmp(input, ">", 1) == 0){
        //     input += 2;
        //     // int file_src = ;
        //     int file_desc = open(input, O_WRONLY | O_APPEND);
        //     int file_src = open(left, O_WRONLY | O_APPEND);
        //     if(file_desc < 0)
        //         printf("Error opening the file_desc\n");

        //     if(file_src < 0)
        //         printf("Error opening the file_src\n");
            
        //     int copy_desc = dup(file_desc);
        //     int copy_src = dup(file_src);
        // }

        /**
         * @brief in case everything else is not accsesed, we will invoke system calls.
         *  system is a 'library method' which invokes the desired system call given the input.
         */
        // system(input);

        manual_system_calls(input);
        return;
        
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
/*
loads and executes a series of external commands that are piped together
*/
void executePiped(char** buf,int nr){//can support up to 10 piped commands
	if(nr>10) return;
	
	int fd[10][2],i,pc;
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

/*
loads and executes a series of external commands that have to be run asyncronously
*/
void executeAsync(char** buf,int nr){
	int i,pc;
	char *argv[100];
	for(i=0;i<nr;i++){
		tokenize_buffer(argv,&pc,buf[i]," ");
		if(fork()==0){
			execvp(argv[0],argv);
			perror("invalid input ");
			exit(1);//in case exec is not successfull, exit
		}
	}
	for(i=0;i<nr;i++){
		wait(NULL);
	}

}

/*
loads and executes a an external command that needs file redirection(input, output or append)
*/
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

/*
shows the internal help
*/
char *split_input[MAXSIZE];
void manual_system_calls(char *input){//check what its do.

    split(input);

    /* forks a new child */
    int process_id = fork();

    if(process_id < 0) {
        /* error case */
        return; 
    }else if (process_id == 0){
        /* child process */
        execvp(split_input[0], split_input);
    }else{   
        /* Parent Process */
        wait(NULL);
    }
    return;
}

void split(char *input){
    char* tmp_input;
    unsigned int i = 0;
    tmp_input = strtok(input," ");
    while(tmp_input != NULL){
        split_input[i] = tmp_input;
        ++i;
        tmp_input = strtok(NULL," ");
    }
    return;
}

void delete_file(char *delete_input){
    /** 
     * i forwad the pointer by 7.
     * this is because i do not need substring 'DELETE'.
     */
    delete_input+=7;
    unlink(delete_input);
}

void copy_from_src_to_dst(char *copy_input){
    /** 
     * i forwad the pointer by 5.
     * this is because i do not need substring 'COPY'.
     */
    copy_input+=5;

    // extract the src location.
    char *src, *handle_src;
    src = (char*)malloc(sizeof(char)*256);
    handle_src = src;
    if(src == NULL){
        printf("Memory allocation failed for src");
    }
    while(*copy_input != ' '){
        *handle_src = *copy_input;
        handle_src++;
        copy_input++;
    }
    *handle_src = '\0';

    copy_input++; // skip the ' ' 

    // extract the dst location
    char *dst, *handle_dst;
    dst = (char*)malloc(sizeof(char)*256);
    handle_dst = dst;
    if(dst == NULL){
        printf("Memory allocation failed for src");
    }
    
    while(*copy_input != '\0'){
        *handle_dst = *copy_input;
        handle_dst++;
        copy_input++;
    }
    *handle_dst = '\0';
    printf("%s\n",src);
    printf("%s\n",dst);
    // Copying the file:
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

//COPY /home/shaked/Desktop/OS-Course/Shell/Txt /home/shaked/Desktop/OS-Course/Shell/Txt2 
    //reading and writing to the new file.
    size = fread(buf, 1, BUFSIZ, src_file);
    while (size > 0) {
        fwrite(buf, 1, size, dst_file);
        size = fread(buf, 1, BUFSIZ, src_file);
    }
    free(src);
    // free(handle_src);
    free(dst);
    // free(handle_dst);

    fclose(src_file);
    fclose(dst_file);

}

void change_directory(char *cd_input){
    /** 
     * i forwad the pointer by 3.
     * this is because i do not need substring 'cd'.
     */
    cd_input+=3;
    if(chdir(cd_input) != 0){
        printf("cd: no such file or directory: %s\n",cd_input);
    }
}


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
                    /*Starts with '.' , so color is pruple */
                    printf(PURPLE);
                    printf("%s\n",file_name->d_name);
                }else{
                    /* executable file so we change its color to green */
                    printf(GREEN); // change color to green
                    printf("%s\n",file_name->d_name);
                }
                printf(WHITE);
            }
            else {
                /* non-executable, stays the same (white) */
                printf("%s\n",file_name->d_name);
            }
                
            
            idx++;
        }
    }

    free(file_name);
    closedir(folder_contents);
}

void open_tcp_socket(){

    struct sockaddr_in server_addr;

    // opening the client socket.
    cli_sock = socket(AF_INET, SOCK_STREAM,0);
    if( cli_sock < 0 ) {
        printf("Socket creation failed, exiting method...\n");
        sleep(1);
        return;

    }else{
        printf("Socket creation was a success..\n");
    }

    bzero(&server_addr, sizeof(server_addr)); // reset the buff.

    // enter ip,port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(LOCALHOST); // local host
    server_addr.sin_port = htons(PORT);
    
    // trying to establish connections.
    int con = connect(cli_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (con != 0){
        printf("Connection with server failed, exiting method...\n");
        sleep(1);
        return;
    }else{
        printf("Connection with the server was a success..\n");
    }
}

void get_curr_directory(){

    long size;
    char *buf;
    char *curr_dir = NULL;
    size = pathconf(".",_PC_PATH_MAX);
    if((buf = (char*)malloc((size_t)size))!=NULL){
        curr_dir = getcwd(buf,(size_t)size);
    }
    printf("%s",curr_dir);

    /*FREE*/
    free(buf);
}

void print_echo_msg(char *return_echo){

    /** 
     * i forwad the pointer by 4.
     * this is because i have no need to see the 'ECHO' in reply.
     */
    return_echo+=5;
    printf("%s",return_echo);
}
// Function to take input
// int takeInput(char* str)
// {
//     char* buf;
  
//     buf = readline("\n>>> ");
//     if (strlen(buf) != 0) {
//         add_history(buf);
//         strcpy(str, buf);
//         return 0;
//     } else {
//         return 1;
//     }
// }
void main(){

    char buf[500],*buffer[100],buf2[500],buf3[500], *params1[100],*params2[100],*token,cwd[1024];
	int nr=0;
	printf("*****************************************************************"    "\n");
	printf("**************************CUSTOM SHELL***************************"    "\n");

	while(1){
		printf("Enter command(or 'exit' to exit):""\n");

		//print current Directory
		if (getcwd(cwd, sizeof(cwd)) != NULL)
		printf( "%s  " , cwd);
		else 	perror("getcwd failed\n");

		//read user input
		fgets(buf, 500, stdin);//buffer overflow cannot happen

		//check if only a simple command needs to be executed or multiple piped commands or other types
		if(strchr(buf,'|')){//tokenize pipe commands
			tokenize_buffer(buffer,&nr,buf,"|");
			executePiped(buffer,nr);
		}
		else if(strchr(buf,'&')){//asyncronous execution
			tokenize_buffer(buffer,&nr,buf,"&");
			executeAsync(buffer,nr);
		}
		else if(strstr(buf,">>")){//append output to file
			tokenize_buffer(buffer,&nr,buf,">>");
			if(nr==2)executeRedirect(buffer,nr,APPEND);
			else printf("Incorrect output redirection!(has to to be in this form: command >> file)");
		}
		else if(strchr(buf,'>')){//redirect output to file
			tokenize_buffer(buffer,&nr,buf,">");
			if(nr==2)executeRedirect(buffer,nr, OUTPUT);
			else printf("Incorrect output redirection!(has to to be in this form: command > file)");
		}
		else if(strchr(buf,'<')){//redirect file to input
			tokenize_buffer(buffer,&nr,buf,"<");
			if(nr==2)executeRedirect(buffer,nr, INPUT);
			else printf("Incorrect input redirection!(has to to be in this form: command < file)");
		}
		else{//single command including internal ones
			tokenize_buffer(params1,&nr,buf," ");
			if(strstr(params1[0],"cd")){//cd builtin command
				chdir(params1[1]);
			}
			// else if(strstr(params1[0],"help")){//help builtin command
			// 	showHelp();
			// }
			else if(strstr(params1[0],"exit")){//exit builtin command
				exit(0);
			}
			// else executeBasic(params1);
		}
	}

	return 0;



    // char inputstring[1000], *parsedArgs[100];
    // char* paredArgsPiped[100];
    // int execFlag = 0;
    // char *inputString;
    // inputString = (char*) malloc(MAXSIZE*sizeof(char));
    // shell_initialize();
    // // printf("%s", "380");
    // struct utsname os_type;
    // uname(&os_type); // name finds.

    // while(TRUE){
  
    //     printf("%s",getenv("USER")); // username.
    //     printf("@%s",os_type.nodename);
    //     printf(":");
    //     printf("~");
    //     printf("$ ");

    //     // printf("inputstring%s ", inputstring);
    //     // printf("parsedArgs%s%s ", parsedArgs);
    //     // printf("pa")
        
    //     bzero(inputString, strlen(inputString));
    //     fgets(inputString,MAXSIZE,stdin);
        
    //     inputString[strcspn(inputString,"\n")] = 0;
    //     // printf("inputString %s\n", inputString);
    //     execFlag = processString(inputString, parsedArgs, paredArgsPiped);
    //     // if(takeInput(inputstring))
    //     //     continue;
    //     // printf("parsedArgs %s\n", parsedArgs);
    //     // printf("paredArgsPiped %s\n ", paredArgsPiped);
    //     // printf("execFlag %d \n", execFlag);
    //     if (execFlag == 2)
    //     {
    //         printf("535\n");
    //         execArgsPiped(parsedArgs, paredArgsPiped);
    //         // printf("484");
    //         // exit(1);
    //         // return;
    //         // continue;;
    //     }
    //     else{
    //         printf("493");
    //         user_input(inputString);
    //     }
    // }
    // free(inputString);

}