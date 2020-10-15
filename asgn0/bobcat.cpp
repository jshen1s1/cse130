#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h> 
#include <error.h>

int main(int argc, char* argv[]){
    int fd; //fd as file descripter
    char in[8192]; //use to store the char read from stdin 

    if(argc == 1){ //if no file specified in command
        while(read(0, in, 1) > 0){
                write(1, in, 1);
        }
    }
    else{
        for (int i=1; i< argc; i++){ //go through all files specified in command
            if(!strcmp(argv[i], "-")){
                while(read(0, in, 1) > 0){
                    write(1, in, 1);
                }
   
            }
            else{
                fd = open(argv[i], O_RDONLY);

                if(fd < 0){
                    perror("Bobcat: ");
                    exit(1);
                }else{
                    while(read(fd, in, 1)>0){
                        write(1, in, 1);
                    }
                    close(fd);
                }

            }
        }
    }

    return (EXIT_SUCCESS);
    
}
