# Quantum Circuit Simulator (qsim)

Progetto per il corso di Sistemi Operativi II (a.a. 2025-26).
Studente: Charlotte Carretta (Matricola: 2128660)

## 1. Introduzione
Il simulatore esegue l'evoluzione unitaria di un sistema quantistico composto da $n$ qubit e, opzionalmente, effettua la misurazione dello stato finale per stimare la distribuzione di probabilità dei diversi stati di base. L'obiettivo principale del progetto è applicare costrutti di programmazione concorrente a basso livello (thread POSIX, mutex, variabili di condizione) per accelerare le computazioni quantistiche, che sono intrinsecamente onerose a causa della crescita esponenziale delle dimensioni dello spazio degli stati ($2^n$).

## 2. Architettura del Simulatore
Il flusso di esecuzione del simulatore si articola nelle seguenti fasi consecutive:
1. **Parsing**: Il programma legge due file di input. Il primo contiene il numero di qubit e lo stato iniziale ($2^n$ ampiezze complesse). Il secondo definisce le matrici delle porte quantistiche ($2^n \times 2^n$ elementi complessi) e la sequenza delle porte da applicare (direttiva `#circ`), terminando facoltativamente con l'istruzione `measure` per specificare il numero di ripetizioni del campionamento.
2. **Inizializzazione delle Risorse**: Viene creato il Thread Pool con il numero di thread worker specificato dall'utente.
3. **Evoluzione Unitaria (Fase 1 del parallelismo)**: Calcola l'effetto delle porte quantistiche sullo stato del sistema. Moltiplica le matrici delle porte quantistiche in parallelo secondo un algoritmo di riduzione ad albero, riducendo l'elenco delle porte a un'unica matrice unitaria finale. Questa viene infine moltiplicata per il vettore di stato iniziale per ottenere il vettore di stato finale.
4. **Misurazione e Campionamento (Fase 2 del parallelismo)**:
   * Viene calcolata la densità di probabilità associata a ciascuno stato di base calcolando il modulo quadro $|a_j|^2$ di ciascuna ampiezza in parallelo.
   * Se sono specificate le ripetizioni (`repetitions > 0`), viene eseguita una simulazione di campionamento discreto multithreaded usando la libreria GSL per stimare le frequenze relative di ciascuno stato.
5. **Output e Deallocazione**: Stampa il vettore di stato finale (se non è stata richiesta la misurazione) o la distribuzione stimata. Libera tutta la memoria allocata e distrugge il thread pool.

## 3. Parallelismo a Due Livelli
Il simulatore sfrutta il parallelismo a due livelli distinti per ottimizzare le prestazioni computazionali:

### 3.1 Evoluzione Unitaria (Riduzione ad Albero)
L'evoluzione di un circuito quantistico con porte $U_0, U_1, \dots, U_{m-1}$ applicate a uno stato iniziale $|\psi_0\rangle$ è descritta dal prodotto:
$$|\psi_{\text{finale}}\rangle = (U_{m-1} \times \dots \times U_1 \times U_0) |\psi_0\rangle$$

Poiché la moltiplicazione matriciale è associativa, non è necessario moltiplicare le porte sequenzialmente da destra a sinistra. Il simulatore implementa una **riduzione ad albero binario** in parallelo:
1. Al livello corrente dell'albero, le porte adiacenti vengono accoppiate e ogni coppia viene moltiplicata in parallelo inserendo un task `execute_matrix_matrix_prod` nel Thread Pool.
2. Una volta completato il livello (tramite sincronizzazione con barriera implicita tramite `tpool_wait`), le porte risultanti diventano l'input per il livello successivo.
3. Questo processo si ripete dimezzando a ogni iterazione il numero di matrici, fino a ridursi a una singola matrice finale di evoluzione $U_{\text{tot}}$.
4. Infine, viene eseguito il prodotto matrice-vettore $U_{\text{tot}} |\psi_0\rangle$ in parallelo (`execute_matrix_vector_prod`).

##### Schema dell'Albero di Riduzione (esempio con 4 porte):
```text
  Livello 0 (Input)      U_0          U_1          U_2          U_3
                          \          /              \          /
                           \        /                \        /
  Livello 1 (Parallelo)     [U_1*U_0]                [U_3*U_2]
                                \                        /
                                 \                      /
  Livello 2 (Parallelo)             [ (U_3*U_2) * (U_1*U_0) ]   --> Matrice Finale U_tot
                                               |
                                               v
                                        U_tot * |psi_0>
```

### 3.2 Campionamento e Misura
La fase di misura si divide a sua volta in due step parallelizzati:
1. **Calcolo dei Moduli Quadri**: Per ottenere la distribuzione di probabilità, il simulatore calcola $|a_j|^2 = \text{Re}(a_j)^2 + \text{Im}(a_j)^2$ per ogni stato $j \in [0, 2^n - 1]$. Questo calcolo è parallelizzato suddividendo il vettore tra i thread tramite task `execute_complex_mod`.
2. **Campionamento Concorrente**: Viene istanziato un generatore discreto di GSL (`gsl_ran_discrete_preproc`). Le ripetizioni del campionamento (`repetitions`) vengono accodate al Thread Pool come task indipendenti (`execute_sample_task`).

#### Prevenzione delle Race Condition nel Campionamento:
Ciascun thread worker genera un campione casuale in base alla distribuzione quantistica. Per accumulare le frequenze totali di occorrenza di ciascuno stato, i thread devono aggiornare un array condiviso di conteggi (`counts`). Per prevenire race condition (scritture simultanee che corrompono i dati), l'incremento di `counts[sample]` è inserito in una **sezione critica** protetta da un mutex di mutua esclusione (`count_mutex`):
```c
// Sezione critica all'interno del task di campionamento
if (task->mutex)
    pthread_mutex_lock(task->mutex);
task->counts[sample]++;
if (task->mutex)
    pthread_mutex_unlock(task->mutex);
```

## 4. Design del Thread Pool
Il Thread Pool gestisce l'esecuzione dei task concorrenti ed è implementato in `thread.c`/`thread.h` utilizzando le API `pthread`.

### 4.1 Struttura del Pool
* **Coda dei Task:** Gestita come una lista semplicemente concatenata di nodi `ThreadPoolWork`. Ogni nodo contiene un puntatore alla funzione da eseguire (`func`) e il puntatore agli argomenti (`arg`).
* **Sincronizzazione della Coda:**
  * `work_mutex` (Mutex): Protegge la coda da accessi simultanei. Ogni inserimento (`tpool_add_work`) e prelievo (`tpool_work_get`) avviene sotto la mutua esclusione del mutex.
  * `work_cond` (Variabile di Condizione): Utilizzata per evitare il busy polling. I thread worker si mettono in attesa su questa variabile quando la coda è vuota. Quando viene aggiunto un task, viene inviato un segnale per svegliare uno o tutti i thread worker.
* **Sincronizzazione delle Fasi (Barriera):**
  * `working_cond` (Variabile di Condizione) e `working_cnt` (Contatore): Tengono traccia di quanti thread sono attivi. La funzione `tpool_wait` blocca il thread principale fino a quando la coda dei task non si svuota e il numero di thread attivi scende a zero. Questo garantisce la sincronizzazione tra le fasi (es. tra i livelli dell'evoluzione unitaria o tra il calcolo dei moduli quadri e il campionamento).

### 4.2 Ciclo di Vita dei Thread Worker
1. All'avvio del pool (`tpool_create`), vengono generati $T$ thread che eseguono la funzione `tpool_worker` in modalità **detached** (`pthread_detach`). Questo consente al sistema operativo di reclamare automaticamente le risorse dei thread alla loro uscita senza dover invocare `pthread_join`.
2. I thread rimangono in attesa di segnali su `work_cond`.
3. Quando un task è disponibile, un thread si sveglia, estrae il task, incrementa `working_cnt`, rilascia il mutex della coda, esegue la funzione, e successivamente riduce `working_cnt`.
4. Se tutti i compiti sono terminati, invia un segnale su `working_cond` per sbloccare eventuali chiamate a `tpool_wait` nel thread principale.
5. Durante la distruzione (`tpool_destroy`), viene impostato un flag `stop = true` e inviato un segnale di broadcast (`pthread_cond_broadcast`) su `work_cond` per risvegliare tutti i thread bloccati, che interrompono immediatamente il ciclo dei worker ed escono.


## 5. Formato dei File di Input
Il simulatore utilizza file con estensione `.q` sia per la definizione dello stato iniziale che del circuito. Entrambi i file supportano commenti e righe vuote. Il parser ignora gli spazi bianchi e gestisce la continuazione su più righe finché non incontra la parentesi quadra di chiusura `]`.

### 5.1 File dello Stato Iniziale
Il file descrive la dimensione del sistema e lo stato quantistico di partenza. Deve contenere due direttive:
1. `#qubits n`: dove $n$ è il numero intero di qubit del sistema (lo spazio degli stati avrà dimensione $2^n$).
2. `#init [alpha_0, alpha_1, ..., alpha_{2^n-1}]`: il vettore di stato iniziale contenente esattamente $2^n$ ampiezze di probabilità espresse come numeri complessi.

##### Gestione dei Numeri Complessi e Delimitatori:
Il parser rimuove i caratteri `[`, `]`, `(`, `)` e la virgola `,`, usandoli come semplici delimitatori. Le ampiezze possono essere scritte in vari formati:
* Parte reale pura: `1.0` o `0.7071`
* Parte immaginaria pura: `i` (interpretato come `1i`), `-i`, `i0.5`
* Forma algebrica compatta: `1+0i`, `0+1i`, `0.7071+i0.0`, `0.7071-i0.7071`
* Forma con delimitatori (es. parte reale e immaginaria tra parentesi): `(0.7071, 0.0)`

##### Esempio di file iniziale (1 qubit):
```text
#qubits 1
#init [ 1+0i, 0+0i ]
```

### 5.2 File del Circuito
Il file del circuito contiene le definizioni delle porte logiche da applicare e la sequenza delle operazioni.
1. `#define NOME [matrice]`: definisce una porta quantistica personalizzata di nome `NOME` tramite una matrice unitaria quadrata di dimensioni $2^n \times 2^n$ (quindi $2^{2n}$ elementi complessi, specificati in formato flat riga per riga). È possibile definire più porte una per riga.
2. `#circ PORTA1 PORTA2 ... [measure REPS]`: descrive la sequenza temporale di applicazione delle porte (da sinistra a destra).
   * Se è presente l'istruzione `measure` seguita da un intero positivo `REPS`, il simulatore esegue il campionamento dello stato finale per `REPS` volte.
   * Se la direttiva `measure` è assente o `REPS` è impostato a 0, il simulatore stamperà direttamente il vettore delle ampiezze complesse dello stato finale.

##### Esempio di file circuito (1 qubit con misura):
```text
#define H [
  (0.7071, 0.0) (0.7071, 0.0)
  (0.7071, 0.0) (-0.7071, 0.0)
]
#define X [ (0, 0) (1, 0) (1, 0) (0, 0) ]

#circ H X measure 1000
```

## 6. Compilazione e Istruzioni per l'Uso

### 6.1 Compilazione
Il progetto include un `Makefile` pronto all'uso. La compilazione richiede il compilatore `gcc` e la libreria scientifica **GNU Scientific Library (GSL)**.
Per compilare il programma, eseguire nella directory del progetto:
```bash
make
```
Verrà generato l'eseguibile `qsim`. Per ripulire i file oggetto e l'eseguibile, utilizzare:
```bash
make clean
```

### 6.2 Esecuzione e Parametri
L'eseguibile accetta tre argomenti obbligatori passati da riga di comando:
```bash
./qsim <file_iniziale> <file_circuito> <num_thread>
```
* `<file_iniziale>`: Percorso del file `.q` contenente la definizione dei qubit e lo stato iniziale.
* `<file_circuito>`: Percorso del file `.q` contenente la sequenza delle porte logiche ed eventualmente le ripetizioni di misura.
* `<num_thread>`: Numero intero positivo di thread worker da utilizzare nel Thread Pool per la simulazione (es. `1` per l'esecuzione sequenziale, `2`, `4`, ecc. per quella parallela).

## 7. Struttura dei Moduli
Il codice sorgente è organizzato nei seguenti file:
* **[main.c](main.c)**: Punto di ingresso dell'applicazione. Coordina la chiamata al parser, alloca lo stato finale, lancia la simulazione parallela e si occupa della pulizia finale.
* **[parser.c](parser.c)** / **[parser.h](parser.h)**: Gestisce il parsing token-by-token dei file di input, effettua la rimozione dei delimitatori sintattici, decodifica la sintassi dei numeri complessi e popola le strutture del circuito quantistico.
* **[thread.c](thread.c)** / **[thread.h](thread.h)**: Implementa il Thread Pool personalizzato basato su thread detached, la coda di task (lista concatenata FIFO) e le funzioni di accodamento (`tpool_add_work`), barriera (`tpool_wait`) e distruzione (`tpool_destroy`).
* **[data.c](data.c)** / **[data.h](data.h)**: Contiene le definizioni delle strutture dati matematiche (ampiezze complesse, porte, registri e task) e implementa l'algebra complessa, le funzioni di moltiplicazione matriciale e la logica di evoluzione unitaria ad albero (`calculate_circuit`).
* **[measurement.c](measurement.c)** / **[measurement.h](measurement.h)**: Gestisce il calcolo parallelo dei moduli quadri e l'esecuzione del campionamento parallelo (protetto da mutex) mediante l'utilizzo dei generatori discreti di GSL, formattando infine l'output a schermo.
* **[memory.c](memory.c)** / **[memory.h](memory.h)**: Fornisce le routine centralizzate di allocazione, riallocazione (per l'albero di riduzione) e deallocazione della memoria dinamica usata dalle strutture dati del simulatore.

