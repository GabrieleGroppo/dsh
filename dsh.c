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
#define MAX_PROMPT 32

char _path[MAX_PATH] = "/bin/:/usr/bin/";

struct var {
  char * name;
  char * value;
};

struct var * vars[MAX_ARGS];

void panic(const char* msg) {
    if(errno) {
        fprintf(stderr, "PANIC: %s %s\n\n", msg, strerror(errno));
    }else {
        fprintf(stderr, "PANIC: %s\n\n", msg);
    }
    exit(EXIT_FAILURE);
}

int prompt(char *buf, size_t buf_size, const char* prompt_string) {
    printf("%s", prompt_string);
    if(fgets(buf, buf_size, stdin) == NULL) {
        return EOF;
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

void do_redir(const char* out_path, char** arg_list, const char* mode){
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
        FILE* out = fopen(out_path, mode);
        if(out == NULL) {
            perror(out_path);
            exit(EXIT_FAILURE);
        }
        dup2(fileno(out), fileno(stdout));
        exec_rel2abs(arg_list);
        perror(arg_list[0]);
        exit(EXIT_FAILURE);
        //end child code
    }else {
        panic("do_redir: fork");
    }
}

void do_pipe(size_t pipe_pos, char** arg_list) {
    if(pipe_pos == 0) {
        panic("do_pipe: parameter error");
    }

    int pipe_fd[2];
    int pid;

    if(pipe(pipe_fd) < 0) {
        panic("do_pipe: pipe");
    }

    
    pid = fork();

    if(pid > 0){

        int wpid = wait(NULL);
        if (wpid < 0){
            panic("de_pipe: wait");
        }
    }else if(pid == 0) {
        close(pipe_fd[0]);
        /
        dup2(pipe_fd[1], 1);
        close(pipe_fd[1]);
        exec_rel2abs(arg_list);
        perror(arg_list[0]);
        
        exit(EXIT_FAILURE);
        
    }else {
        panic("do_pipe: fork");
    }

    pid = fork();
    
    if(pid > 0){
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        int wpid = wait(NULL);
        if (wpid < 0){
            panic("de_pipe: wait");
        }
    }else if(pid == 0) {
        
        close(pipe_fd[1]);
        dup2(pipe_fd[0], 0);
        close(pipe_fd[0]);
        exec_rel2abs(arg_list + pipe_pos + 1);
        perror(arg_list[pipe_pos + 1]);
        exit(EXIT_FAILURE);
    }else {
        panic("do_pipe: fork");
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
        exec_rel2abs(arg_list);
        perror(arg_list[0]);
        exit(EXIT_FAILURE);
    }else {
        panic("do_exec: fork");
    }
    
}

void handleSet(char ** args, int arg_count) {
  if (arg_count != 3)
    fprintf(stderr, "Error: set\n");

  if (strcmp(args[1], "PATH") == 0) {
    setpath(args[2]);
    return;
  }

  struct var * new_var = malloc(sizeof(struct var));
  if (new_var == NULL) {
    panic("handleSet");
  }

  new_var -> name = malloc(strlen(args[1]) + 1);
  if (new_var -> name == NULL) {
    panic("handleSet");
  }

  new_var -> value = malloc(strlen(args[2]) + 1);
  if (new_var -> value == NULL) {
    panic("handleSet");
  }

  strcpy(new_var -> name, args[1]);
  strcpy(new_var -> value, args[2]);
  vars[arg_count - 3] = new_var;
}

char* getValue(char* name) {
    int i = 0;
    while (vars[i] != NULL) {
        if (strcmp(vars[i] -> name, name) == 0) {
            return vars[i] -> value;
        }
        i++;
    }
    return NULL;
}

int main(void) {
    char input_buffer[MAX_LINE];
    size_t arg_count;
    char* arg_list[MAX_ARGS];
    char prompt_string[MAX_PROMPT] = "\0";
    if (isatty(0)) {
        strcpy(prompt_string, "dsh$ \0");
    }
    
    
    while (prompt(input_buffer, MAX_LINE, prompt_string) >= 0){
        
        arg_count = 0;
        arg_list[arg_count] = strtok(input_buffer, " ");
        
        if (arg_list[arg_count] == NULL){
            
            continue;
        }else {
            do {
                arg_count++;
                if (arg_count >= MAX_ARGS) {
                    break;
                }
                arg_list[arg_count] = strtok(NULL, " ");
            } while (arg_list[arg_count] != NULL);
        }

        //BUILT-IN COMMANDS
        if (strcmp(arg_list[0], "exit") == 0) {
            exit(EXIT_SUCCESS);
        }
        if (strcmp(arg_list[0], "setpath") == 0) {
            setpath(arg_list[1]);
            continue;
        }
        //END BUILT-IN COMMANDS
        {
            size_t redir_pos = 0;
            size_t append_pos = 0;
            size_t pipe_pos = 0;
            size_t value_pos = 0;

            for (int i = 0; i < arg_count; i++) {
                if(strcmp(arg_list[i], ">") == 0) {
                    //Redirect symbol found
                    redir_pos = i;
                    break;
                }
                if(strcmp(arg_list[i], ">>") == 0) {
                    //Redirect symbol append found
                    append_pos = i;
                    break;
                }
                if(strcmp(arg_list[i], "|") == 0) {
                    //Pipe symbol found
                    pipe_pos = i;
                    break;
                }
                if (strstr(args[i], "$") != NULL) {
                    value_pos = i;
                    break;
                }
            }

            if(redir_pos != 0) {
                
                arg_list[redir_pos] = NULL;
                
                do_redir(arg_list[redir_pos + 1], arg_list, "w+");
            } else if(append_pos != 0) {
                arg_list[append_pos] = NULL;
                do_redir(arg_list[append_pos + 1], arg_list, "a+");
            } else if(pipe_pos != 0) {
                arg_list[pipe_pos] = NULL;
                do_pipe(pipe_pos, arg_list);
            } else if(value_pos != 0){
                printf("%s\n", getValue(args[value_pos] + 1));
            } else {
                do_exec(arg_list);
            }
            
        }
    }
    exit(EXIT_SUCCESS);
}

