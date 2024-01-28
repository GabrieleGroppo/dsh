# dsh

Autore: Gabriele Groppo
Mat: 902238
## Implemento il punto 4

Utilizzo un array di variabili `vars` per salvare le variabili di ambiente che vengono passate al programma. In questo modo, quando viene eseguito il comando `set` le variabili vengono salvate nell'array e saranno successivamente richiamabili con il carattere speciale `$`.

Utilizzo una variabile `var_count` per tenere traccia del numero di variabili di ambiente che sono state salvate nell'array.



###Funzioni utilizzate:
#### GETVALUE
La funzione `getValue(const char* name)` viene utilizzata per stampare il valore della variabile di ambiente passata per parametro. 
- Se la variabile non esiste, viene stampato un messaggio di errore.
- Se la variabile è `PATH`, viene stampato il valore della variabile `PATH` salvata in `_path`.

```C
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
        printf("Variable %s does not exist.\n", name);
        return;
    }
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i]->name, name) == 0) {
            printf("%s\n", vars[i]->value);
            break;
        }
    }
}
```
 #### SETVARS
La funzione `setvars(char** args, int arg_count)` viene utilizzata per aggiungere una variabile di ambiente all'array `vars`.
- Se il numero di argomenti passati è diverso da 3, viene stampato un messaggio di errore.
- Se la variabile è `PATH`, viene chiamata la funzione `setpath` per aggiornare il valore della variabile `PATH`.
- Se la variabile non esiste, viene creata una nuova variabile e viene aggiunta all'array `vars`.
- Se la variabile esiste, viene aggiornato il valore della variabile.


```C
void setvars(char** args, int arg_count) {
    if (arg_count != 3)
		printf("Invalid number of arguments\n");
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
```

### Aletrazioni al codice originale

- Aggiunto comando BUILTIN `set` per aggiungere una variabile di ambiente.
- Aggiunto comando BUILTIN `$` per stampare il valore di una variabile di ambiente.


### Aggiunte non richieste

- welcome_message per la stampa di un messaggio di benvenuto.
- `setprompt` per la modifica della scritta stampata come prompt.
- aggiunta builtint `help` per stampare la lista dei comandi **DSH**.
