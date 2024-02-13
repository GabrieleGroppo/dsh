/*

DIDATTIC SHELL: dsh.c

Autore: Gabriele Groppo
Mat: 902238
Punto sviluppato: 4

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <termios.h>
#include <signal.h>

#define MAX_LINE 4096
#define MAX_ARGS 256
#define MAX_PATH 512
#define MAX_PROMPT 32

char _path[MAX_PATH] = "/bin/:/usr/bin/";
char prompt_string[MAX_PROMPT] = "\0";

struct var {
    char* name;
    char* value;
};

typedef struct Node Node;
struct Node* head = NULL;

struct Node {
    char command[200];
    struct Node* next;
    struct Node* prev;
};

int nodes = 0;

struct var* vars[MAX_ARGS];
int var_count = 0;

void panic(const char* msg) {
    if(errno) {
        fprintf(stderr, "PANIC: %s %s\n\n", msg, strerror(errno));
    }else {
        fprintf(stderr, "PANIC: %s\n\n", msg);
    }
    exit(EXIT_FAILURE);
}

void set_propt_string(const char* new_prompt_string) {
    if(new_prompt_string != NULL) {
        strcpy(prompt_string, new_prompt_string);
    }
}

int prompt(char *buf, size_t buf_size, const char* prompt_string) {
    printf("%s", prompt_string);
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

void clearUntilDollar() {
    // Utilizza la sequenza di escape ANSI per spostarti all'inizio della riga
    printf("\033[1G");

    // Stampa spazi per cancellare fino al carattere '$'
    printf("\033[K");
}

void handle_signal_child(int sig) {
    if(sig == SIGTERM) {
        kill(getpid(), SIGKILL);
    }
}

int prompt_alternativa(char *buf, size_t buf_size, const char* prompt_string) {
    struct termios oldt, newt;
    Node *cur = head;
    
    fflush(stdin);
    // recupera la configurazione attuale del terminale e la salva in oldt
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    // Disabilita l'echo e la modalità canonica
    newt.c_lflag &= ~(ICANON | ECHO);
    // Applica la nuova configurazione del terminale a STDIN
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    printf("%s",  prompt_string);
    char command[200];
    int pos = -1;

    while (1) {
        char c = getchar();
        
        //termino se premo ctrl+d
        if(c == 4) {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);//resetto il terminale
            fflush(stdin);//pulisco il buffer di input
            printf("\n");
            return -1;//termino il programma
        }

        if(c == '\b' || c == 127){
            if(pos >= 0){
                pos--;
                printf("\b \b");
                
            }else{
                pos = -1;
            }
            continue;
        }else {
            pos ++;
        }

        command[pos] = c;

        if(c == '\n'){
            printf("\n");
            command[pos] = '\0';
            break;
        }
        if (c == '[') {
            c = getchar();
            /*

            possibile implementazione per le frecce

            if(c == 'C') {
                printf("Right arrow key pressed\n");
            } else if (c == 'D') {
                printf("dovrei uscire");
                printf("Left arrow key pressed\n");
            } else 
            
            */
            
            if (c == 'B') {
                if(cur != NULL && cur->prev != NULL){
                    cur = cur->prev;
                    clearUntilDollar();
                    printf("%s", prompt_string);
                    printf("%s", cur->command);
                    pos = strlen(cur->command);
                    strcpy(command, cur->command);
                    pos --;
                }else{
                	//cur = head;
                    clearUntilDollar();
                    printf("%s", prompt_string);
                    pos = -1;
                }
            } else if (c == 'A') {
                if(cur != NULL && cur->next != NULL){
                    
                    clearUntilDollar();
                    printf("%s", prompt_string);
                    printf("%s", cur->command);
                    pos = strlen(cur->command);
                    strcpy(command, cur->command);
                    pos --;
                    cur = cur->next;
                }else if(cur != NULL){
                    clearUntilDollar();
                    printf("%s", prompt_string);
                    printf("%s", cur->command);
                    pos = strlen(cur->command);
                    strcpy(command, cur->command);
                    pos --;
                }
                else{
                    clearUntilDollar();
                    printf("%s", prompt_string);
                    pos = -1;
                }
            }else{
                printf("%c", c);
            }
        }else
            printf("%c", c);
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fflush(stdin);
    strcpy(buf, command);
    return pos;
}

Node* replace_old_command(Node* newNode){
    Node* cur = head;
    while(cur->next != NULL){
        cur = cur->next;
    }
    Node* temp = cur;
    cur = cur->prev;
    cur->next = NULL;
    newNode->next = head;
    head->prev = newNode;
    free(temp);
    head = newNode;
}

Node addCommands(char command[200]){
    Node* newNode = (Node*) malloc(sizeof(Node));
    strcpy(newNode->command, command);
    newNode->next = head;
    newNode->prev = NULL;

    if(nodes < 10){
        if(head != NULL){
            head->prev = newNode;
        }
        head = newNode;
        nodes++;
    }else{
        replace_old_command(newNode);
    }
}

void printlist(Node* head){
    Node* cur = head;
    printf("Commands history: \n");
    int count = 0;
    while(cur != NULL){
    	count ++;
        printf("%d. %s\n", count, cur->command);
        cur = cur->next;
    }
    printf("\n");
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

void do_redir(const char* out_path, char** arg_list, const char* mode) {
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
        FILE* out = fopen(out_path, mode);//w+ va cambiato per le altre operazioni di redirect
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

void do_pipe(size_t pipe_pos, char** arg_list) {
    if(pipe_pos == 0) {
        panic("do_pipe: parameter error");
    }

    //Es. ls | wc -l
    //Vettore con i file descriptor dei due lati della pipe 
    //(0: lettura, 1: scrittura)
    int pipe_fd[2];
    int pid;
    //left size of the pipe
    if(pipe(pipe_fd) < 0) {
        panic("do_pipe: pipe");
    }

    
    pid = fork();

    if(pid > 0) {

        int wpid = wait(NULL);
        if (wpid < 0){
            panic("de_pipe: wait");
        }
    }else if(pid == 0) {
        //begin child code
        //Chiudo il lato di lettura della pipe
        close(pipe_fd[0]);
        //Ridirigo lo STDOUT sul lato di scrittura della pipe
        dup2(pipe_fd[1], 1);//STDOUT_FILENO == 1 == fileno(stdout)
        close(pipe_fd[1]);
        exec_rel2abs(arg_list);
        perror(arg_list[0]);
        //Se exec fallisce, termino il processo figlio
        exit(EXIT_FAILURE);
        //end child code
    }else {
        panic("do_pipe: fork");
    }

    //right size of the pipe
    pid = fork();
    
    if(pid > 0){
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        int wpid = wait(NULL);
        if (wpid < 0){
            panic("de_pipe: wait");
        }
    }else if(pid == 0) {
        //begin child code
        //Chiudo il lato di scrittura della pipe
        close(pipe_fd[1]);
        //Ridirigo lo STDIN sul lato di lettura della pipe
        dup2(pipe_fd[0], 0);//STDIN_FILENO == 0 == fileno(stdin)
        close(pipe_fd[0]);
        exec_rel2abs(arg_list + pipe_pos + 1);//+1 per saltare il simbolo |
        perror(arg_list[pipe_pos + 1]);
        //Se exec fallisce, termino il processo figlio
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

void getValue(const char* name) {
    
    if (name == NULL) {
        printf("Name can't be NULL\n");
        return;
    }
    if (strcmp(name, "PATH") == 0) {
        printf("%s\n", _path);
        return;
    }
    if (var_count == 0) {
        printf("No variables other than the PATH have yet been set.\n");
        return;
    }
    
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i]->name, name) == 0) {
            printf("%s\n", vars[i]->value);
            return;
        }
    }
    printf("Variable %s does not exist.\n", name);
}
//GESTIONE (AGGIUNTA e MODIFICA) VARIABILI
void setvars(char** args, int arg_count) {
    if (arg_count != 3){
        printf("Invalid number of arguments\n");
        return;
    }

    //Controllo se sto modificando il path
    //Il path ha bisogno di passare controlli più severi per essere aggiornato
    if (strcmp(args[1], "PATH") == 0) {
        setpath(args[2]);
        return;
    }
    //Altrimenti aggiungo una variabile
    struct var* new_var = malloc(sizeof(struct var));
    if (new_var == NULL) {
        panic("setvars: Malloc");
    }
    
    new_var->name = malloc(strlen(args[1]) + 1);

    if (new_var->name == NULL) {
        panic("setvars: Malloc");
    }

    new_var->value = malloc(strlen(args[2]) + 1);
    if (new_var->value == NULL) {
        panic("setvars: Malloc");
    }

    strcpy(new_var->name, args[1]);
    strcpy(new_var->value, args[2]);
    //Verifico se la variabile esiste gia'
    for(int i = 0; i < var_count; i++) {
        if (strcmp(vars[i]->name, new_var->name) == 0) {
            vars[i] -> value = new_var -> value;
            return;
        }
    }
    //Se non esiste, la aggiungo
    vars[var_count++] = new_var;
    
}

void welcome_message(){
    //Stampa DSH in grosso
    printf("Welcome! \n\n");
        char matrice[5][40] = {
        " #####    ######  #    # ",
        " #    #  #       #    # ",
        " #    #   ####   ###### ",
        " #    #       #  #    # ",
        " #####   #####  #    # "
    };

    for (int i = 0; i < 5; i++) {
        printf("%s\n", matrice[i]);
    }

    printf("\nGabriele Groppo's DIDATTIC SHELL\n");
    printf("Type help for the list of commands.\n\n");
}

int main(void) {
    welcome_message();
    char input_buffer[MAX_LINE];
    size_t arg_count;
    char* arg_list[MAX_ARGS];
    
    if (isatty(0)) {//contrllo se stdin e' un terminale 
        strcpy(prompt_string, "dsh$ \0");
    }
    
    while (prompt_alternativa(input_buffer, MAX_LINE, prompt_string) >= 0){
        
        //printf("\n DEBUG: I read [%s]\n", input_buffer);
        addCommands(input_buffer);
        
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
        //set command
        if (strcmp(arg_list[0], "set") == 0) {
            setvars(arg_list, arg_count);
            continue;
        }
        //setprompt command
        if (strcmp(arg_list[0], "setprompt") == 0) {
            set_propt_string(arg_list[1]);
            continue;
        }
        //info command
        if (strcmp(arg_list[0], "info") == 0) {
            welcome_message();
            continue;
        }
        //info command
        if (strcmp(arg_list[0], "mycommands") == 0) {
            printlist(head);
            continue;
        }
        //help command
        if(strcmp(arg_list[0], "help") == 0) {
            printf("Built-in commands:\n");
            printf("exit: exit the shell\n");
            printf("set: set a variable\n");
            printf("setprompt: set the prompt string\n");
            printf("info: print info about the shell\n");
            printf("help: print this help message\n");
            printf("mycommands: print the dsh commands history (last 10 commands)\n");
            continue;
        }

        //END BUILT-IN COMMANDS
        {
            size_t redir_pos = 0;
            size_t append_pos = 0;
            size_t pipe_pos = 0;
            size_t value_pos = 0;
            //check for special characters (e.g. |, <, >, >>, &, etc.)
            //TODO
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
                if (strstr(arg_list[i], "$") != NULL) {
                    value_pos = i;
                    break;
                }

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
                do_redir(arg_list[redir_pos + 1], arg_list, "w+");
            } else if(append_pos != 0) {
                arg_list[append_pos] = NULL;
                do_redir(arg_list[append_pos + 1], arg_list, "a+");
            } else if(pipe_pos != 0) {
                arg_list[pipe_pos] = NULL;
                do_pipe(pipe_pos, arg_list);
            } else if(value_pos != 0) {
                getValue(arg_list[value_pos] + 1);
            }
            else {
                //exec
                do_exec(arg_list);
            }
            
        }
    }
    exit(EXIT_SUCCESS);
}

