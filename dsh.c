/*dsh.c*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 4096
#define MAX_ARGS 256
#define MAX_PATH 512

char _path[MAX_PATH] = "/bin/:/usr/bin/";

void panic(const char* msg) {
    if(errno) {
        fprintf(stderr, "PANIC: %s %s\n\n", msg, strerror(errno));
    }else {
        fprintf(stderr, "PANIC: %s\n\n", msg);
    }
    exit(EXIT_FAILURE);
}

int prompt(char *buf, size_t buf_size) {
    printf("dsh$ ");
    if(fgets(buf, buf_size, stdin) == NULL) {
        return EOF;//Esco dalla shell con ctrl+d
    }

    size_t cur = -1;

    do {
        cur++;
    }while (buf[cur] != '\n' && buf[cur] != '\0');
    
    if (buf[cur] == '\n') {
        buf[cur] = '\0';
    }

    return cur;
}

void setpath (const char* new_path) {
    if (new_path != NULL){
        //SOSTITUISCO IL PATH
        #if USE_DEBUG_PRINTF
            printf("DEBUG: new_path = %s\n", new_path);
        #endif
        
        int new_path_len = strlen(new_path);
        if (new_path_len > MAX_PATH -1){
            fprintf(stderr, "Error: Path string too long.\n");
            return;
        }
        if(new_path_len > 0){
            memcpy(_path, new_path, new_path_len + 1);
        }
    }
    printf("%s\n", _path);
}

//rel_path: path relativo o nome del comando. Esempio: ls, /bin/ls, ./ls
void path_lookup(char* abs_path, const char* rel_path) {
    char* prefix = strtok(_path, ":");
    char buf[MAX_PATH];
    if(abs_path == NULL || rel_path == NULL) {
        panic("get_abs_path: parameter error");
    }

    while (prefix != NULL){
        strcpy(buf, prefix);
        strcat(buf, rel_path);
        if (access(buf, X_OK) == 0){
            strcpy(abs_path, buf);
            return;
        }
        prefix = strtok(NULL, ":");
    }
    strcpy(abs_path, rel_path);
}

void exec_rel2abs(char** arg_list) {
    if(arg_list[0][0] == '\\') {
        //assume absolute path
        execv(arg_list[0], arg_list);
    }else {
        //assume relative path
        char abs_path[MAX_PATH];
        path_lookup(abs_path, arg_list[0]);
        execv(abs_path, arg_list);
    }
}

void do_redir(const char* out_path, char** arg_list){
    if(out_path == NULL) {
        panic("do_redir: parameter error");
    }

    int pid = fork();

    if(pid > 0){
        int wpid = wait(NULL);
        if (wpid < 0){
            panic("de_redir: wait");
        }
    }else if(pid == 0) {
        //begin child code
        FILE* out = fopen(out_path, "w+");//w+ va cambiato per le altre operazioni di redirect
        if(out == NULL) {
            perror(out_path);
            exit(EXIT_FAILURE);//termino il processo figlio
        }
        dup2(fileno(out), fileno(stdout));//STDOUT_FILENO == 1 == fileno(stdout)
        exec_rel2abs(arg_list);
        perror(arg_list[0]);
        exit(EXIT_FAILURE);//termino il processo figlio
        //end child code
    }else {
        panic("do_redir: fork");
    }
}

void do_exec(char** arg_list) {
    int pid = fork();

    if(pid > 0){
        int wpid = wait(NULL);
        if (wpid < 0){
            panic("de_exec: wait");
        }
    }else if(pid == 0) {
        //begin child code
        exec_rel2abs(arg_list);
        perror(arg_list[0]);
        //Se exec fallisce, termino il processo figlio
        exit(EXIT_FAILURE);
        //end child code
    }else {
        panic("do_exec: fork");
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
        if (strcmp(arg_list[0], "exit") == 0) {
            exit(EXIT_SUCCESS);
        }
        if (strcmp(arg_list[0], "setpath") == 0) {
            setpath(arg_list[1]);
            continue;//back to prompt
        }
        //END BUILT-IN COMMANDS
        {
            size_t redir_pos = 0;
            //check for special characters (e.g. |, <, >, >>, &, etc.)
            //TODO
            for (int i = 0; i < arg_count; i++) {
                if(strcmp(arg_list[i], ">") == 0) {
                    //Redirect symbol found
                    redir_pos = i;
                    break;
                }
                //if(strcmp(arg_list[i], "<") == 0) { //TODO}
                //if(strcmp(arg_list[i], "|") == 0) { //TODO}
                //if(strcmp(arg_list[i], "&") == 0) { //TODO}
            }

#if USE_DEBUG_PRINTF //-DUSE_DEBUG_PRINTF//Guardia per il preprocessore
            printf("DEBUG: tokens");
            for (size_t i = 0; i < arg_count; i++) {
                printf(" %s", arg_list[i]);
            }
            puts("");
#endif
            //do shell operations
            //Per semplicita' considero un solo argomento dopo il redirect
            if(redir_pos != 0) {
                //Remove the redirection from the argument list
                arg_list[redir_pos] = NULL;
                //effettuo la ridirezione
                //nome del file su cui scrivere in arg_list[redir_pos + 1]
                do_redir(arg_list[redir_pos + 1], arg_list);
            // } else if() { //TODO}
            } else {
                //exec
                do_exec(arg_list);
            }
            
        }
    }
    puts("");
    exit(EXIT_SUCCESS);
}
