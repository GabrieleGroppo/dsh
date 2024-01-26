# Shell DIdattica (dsh)

## Requisiti

Nome del file eseguibile: `dsh`
Nome del file sorgente: `dsh.c` (`gcc dsh.c -o dsh`)
1. Presentarci il prompt `dsh$ ` all' apertura (se non vengono specificati parametri) 
2. Interpretare i comandi dell'utente inseriti da riga di comando
    - Built-in (`exit`, `setpath`)
        - Possibilità di specificare il PATH tramite una stringa della forma `/bin/:usr/bin/` ecc.
        - Il builtin deve restiturire all'utente il path attuale se non viene specificato nulla
        - Devo poter uscire dall shell anche con Ctrl-C e Ctrl-D
    - Comandi generici della forma `ls -la` e `/bin/wc -l`
    - Funzioni speciali: Redirezione  `>`, `|`, `&`, `>>`
3. Interpretari i comandi dell'utente caricandoli da file di script
    - File `pippo.dsh` che carico ed eseguo riga per riga
    - Passandolo come argomento da riga di comando
    - Con un built-in (`load` o `run`)
4. Deve fornire una rudimentale gestione degli errori.