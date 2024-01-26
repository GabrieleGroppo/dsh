/*dsh.c*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 4096
#define MAX_ARGS 256

void panic(const char* msg){
    if(errno) {
        fprintf(stderr, "PANIC: %s %s\n\n", msg, strerror(errno));
    }else {
        fprintf(stderr, "PANIC: %s\n\n", msg);
    }
    exit(EXIT_FAILURE);
}

int prompt(char *buf, size_t buf_size){
    printf("dsh$ ");
    if(fgets(buf, buf_size, stdin) == NULL){
        return EOF;//Esco dalla shell con ctrl+d
    }

    size_t cur = -1;

    do {
        cur++;
    }while (buf[cur] != '\n' && buf[cur] != '\0');
    
    if (buf[cur] == '\n'){
        buf[cur] = '\0';
    }

    return cur;
}

void do_exec(char** arg_list){
    int pid = fork();

    if(pid > 0){
        int wpid = wait(NULL);
        if (wpid < 0){
            panic("wait");
        }
    }else if(pid == 0){
            execv(arg_list[0], arg_list);
            perror(arg_list[0]);
            exit(EXIT_FAILURE);//Se exec fallisce, termino il processo figlio
    }else {
            panic("fork");
    }
    
}



int main(void) {
    char input_buffer[MAX_LINE];
    size_t arg_count;
    char* arg_list[MAX_ARGS];
    
    while (prompt(input_buffer, MAX_LINE) >= 0){
        //printf("DEBUG: I read %s\n", input_buffer);
        arg_count = 0;
        arg_list[arg_count] = strtok(input_buffer, " ");
        
        if (arg_list[arg_count] == NULL){
            // se non specifico nulla, continuo (riga vuota)
            continue;
        }else {
            do {
                arg_count++;
                if (arg_count >= MAX_ARGS) {
                    break;
                }
                arg_list[arg_count] = strtok(NULL, " ");//NULL perche' strtok mantiene lo stato del buffer precedente
            } while (arg_list[arg_count] != NULL);
        }

        //BUILT-IN COMMANDS
        if (strcmp(arg_list[0], "exit") == 0){
            exit(EXIT_SUCCESS);
        }

        //exec
        do_exec(arg_list);

#if USE_DEBUG_PRINTF //-DUSE_DEBUG_PRINTF//Guardia per il preprocessore
        printf("DEBUG: tokens");
        for (size_t i = 0; i < arg_count; i++) {
            printf(" %s", arg_list[i]);
        }
        puts("");
#endif


    }
    puts("");
    exit(EXIT_SUCCESS);
}
