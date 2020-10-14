#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <error.h>

int main(int argc, char* argv[]){
    int fd, ch, count; //fd as file descripter, ch as char
    char in[8192];
    char str[8192];
    size_t bufsiz;


    if(argc == 1){ //if no file specified in command
        while(read(STDIN_FILENO, in, 1) > 0){

            if(*in == '\n'){
                write(STDOUT_FILENO, in,1);
                write(STDOUT_FILENO, "\n",1);
            }
        }
    }
    else{
        for (int i=1; i< argc; i++){ //go through all files specified in command
            if(argv[i] == '-'){
                
            }
            else{
                fd = open(argv[i], O_RDONLY);

                if(fd < 0){
                    perror("Error: ");
                    goto SKIP; //skip the error file
                }else{
                    while(read(fd, &ch, 1)){
                        write(STDOUT_FILENO, &ch, 1);
                    }
                    close(fd);
                }
                SKIP:
            }
        }
    }

    return 0;
    
}