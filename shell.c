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

#include <unistd.h>
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

/* method to clear the shell. */
#define clear() printf("\033[H\033[J]")

/* Colors */
#define GREEN "\033[0;32m"
#define PURPLE "\033[0;35m"
#define WHITE "\033[0m"

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

        // Exit shell case:
        // if(strcmp(input,"EXIT") == 0){
        //     printf("Exiting shell, good bye ...\n");
        //     free(input);
        //     sleep(1);
        //     exit(1);
        // }
        // get directory case:
        // if(strcmp(input,"pwd") == 0){
        //     // get_curr_directory();
        //     printf("\n");
        //     return;
        // }
        
        // open a tcp localhost, we are client.
        // if(strcmp(input,"TCP PORT") == 0){

        //     /* Try to auto start server failure :( */
        //     // const char *run_server = "gnome-terminal --command=\"./Server\" ";
        //     // system(run_server);
        //     // sleep(1);

        //     open_tcp_socket();
        //     dup2(1,1234);
        //     dup2(cli_sock,1);
        //     tcp_connections++;
        //     return;
        // }

        // close the client socket.
        // if(strcmp(input,"LOCAL") == 0 && tcp_connections>0 ){
        //     /*Tells the server to close connection*/
        //     printf("Exit\n");
        //     sleep(1);

        //     close(cli_sock);
        //     dup2(1234,1);
        //     tcp_connections = 0; 
        //     return;
        // }
        if(strcmp(input,"DIR") == 0){
            show_library_files();
            return;
        }

        //handle cd case:
        //this line checks if a string starts with 'CD'
        // if(strncmp(input,"CD",2) == 0){
        //     change_directory(input);
        //     return;
        // }
        
        //handle copy case:
        //this line checks if a string starts with 'COPY'
        if(strncmp(input,"COPY",4) == 0){
            copy_from_src_to_dst(input);
            return;
        }
        char left[MAXSIZE];
        int idx=0;
        int size = strlen(input);
        // printf("%d", 128);
        while ((*input) != ' ')
        {
            // printf("%d", 131);
            left[idx] = (*input);
            idx++;
            input++;
        }
        input++;
        if(strncmp(input, "|", 1) == 0){
            // printf("%d", 128);
            input += 2;
            int i;
            for(i=1; i<size-1; i++)
            {
                int pd[2];
                pipe(pd);

                if (!fork()) {
                    dup2(pd[1], 1); // remap output back to parent
                    execlp(input[i], input[i], NULL);
                    perror("exec");
                    abort();
                }

                // remap output from previous child to input
                dup2(pd[0], 0);
                close(pd[1]);
            }

            execlp(left, left, NULL);
            perror("exec");
            abort();
            return;
        }
        if(strncmp(input, ">", 1) == 0){
            input += 2;
            // int file_src = ;
            int file_desc = open(input, O_WRONLY | O_APPEND);
            int file_src = open(left, O_WRONLY | O_APPEND);
            if(file_desc < 0)
                printf("Error opening the file_desc\n");

            if(file_src < 0)
                printf("Error opening the file_src\n");
            
            int copy_desc = dup(file_desc);
            int copy_src = dup(file_src);
        }

        // handle echo msg case:
        // this line checks if a string starts with 'ECHO'
        // if(strncmp(input,"ECHO",4) == 0){
        //     print_echo_msg(input);
        //     printf("\n");
        //     return;
        // }

        // // handle delete file case:
        // // this line checks if a string starts with 'DELETE'
        // if(strncmp(input,"DELETE",6) == 0){
        //     delete_file(input);
        //     return;
        // }

        /**
         * @brief in case everything else is not accsesed, we will invoke system calls.
         *  system is a 'library method' which invokes the desired system call given the input.
         */
        // system(input);

        manual_system_calls(input);
        return;
        
}
char *split_input[MAXSIZE];
void manual_system_calls(char *input){

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

void main(){

    // printf("%d", 376);
    char *inputString ;
    inputString = (char*) malloc(MAXSIZE*sizeof(char));
    shell_initialize();
    // printf("%s", "380");
    struct utsname os_type;
    uname(&os_type); // name finds.

    while(TRUE){

        // printf("%d", 384);
        // printf("%s", argv[1]);
        // if (strcmp(argv[1], "|") == 0){
        // int i;
        // for( i=1; i<argc-1; i++)
        // {
        //     int pd[2];
        //     pipe(pd);

        //     if (!fork()) {
        //         dup2(pd[1], 1); // remap output back to parent
        //         execlp(argv[i], argv[i], NULL);
        //         perror("exec");
        //         abort();
        //     }

        //     // remap output from previous child to input
        //     dup2(pd[0], 0);
        //     close(pd[1]);
        // }

        // execlp(argv[i], argv[i], NULL);
        // perror("exec");
        // abort();
        //     }
        
        // printf(GREEN); // change color to green
        // printf("%s", "387");
        printf("%s",getenv("USER")); // username.
        // printf("%s", "388");
        printf("@%s",os_type.nodename); // os 
        // printf("%s", "389");
        // printf(WHITE); // changes to the basic color.
        printf(":");
        // printf(PURPLE); // change color to purple
        printf("~");
        // get_curr_directory(); //directory
        printf("$ ");

        // printf(WHITE); // changes to the basic color.
        /*using fgets so it will include spaces, etc..*/
        bzero(inputString, strlen(inputString));
        fgets(inputString,MAXSIZE,stdin);
        inputString[strcspn(inputString,"\n")] = 0;
        user_input(inputString);
    }
    free(inputString);
}