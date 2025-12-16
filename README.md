# SCHSIM

## Autors
Aquest projecte ha estat desenvolupat en grup per:
* Oriol Lladó
* Pau Ribalta
* Enric Tomàs

## Descripció
SCHSIM és un simulador de planificació de processos desenvolupat en el marc de l’assignatura de Sistemes Operatius. El projecte té com a objectiu principal facilitar la comprensió del funcionament intern dels algorismes de planificació de la CPU, així com reforçar les habilitats de programació en llenguatge C i la gestió d’estructures de dades bàsiques.

Mitjançant la simulació de diferents polítiques de planificació, l’estudiant pot analitzar el comportament del sistema, comparar resultats i observar com influeixen paràmetres com el temps d’arribada, la durada de la ràfega de CPU, la prioritat o el quantum.


---

## Implementació
* El simulador permet executar els algorismes de planificació **FCFS (First Come First Served)**, **SJF/SJRT (Shortest Job First / Shortest Job Remaining Time)**, **Prioritats** i **Round Robin**.
* Cada procés disposa d’una única ràfega de CPU, sense fases intermèdies d’entrada/sortida ni bloqueig.
* No s’implementa la suspensió de processos ni estats de bloqueig; tots els processos es consideren sempre llestos un cop arriben al sistema.
* El temps de canvi de context entre processos és considerat nul, de manera que no penalitza el rendiment de la simulació.
* Les operacions d’entrada/sortida no bloquegen la CPU i es consideren totalment superposables.
* El sistema permet simular únicament un processador (CPU), sense execució paral·lela.

---

## Format dels processos
Els processos es defineixen mitjançant un fitxer CSV, on cada línia representa un procés independent. Els camps inclosos són:
* Identificador del procés
* Nom del procés
* Prioritat
* Temps d’arribada al sistema
* Temps total de CPU requerit (burst)

Aquest format permet modificar fàcilment els escenaris de simulació i analitzar el comportament dels diferents algorismes sota condicions diverses.

---

## Resultats de la simulació
Un cop finalitzada la simulació, el programa mostra:
* Una representació temporal de l’execució de cada procés, indicant quan es troba en execució o ha finalitzat.
* Mètriques bàsiques del sistema, com ara:
  * Temps total de simulació
  * Temps mitjà d’espera
  * Temps mitjà de resposta
  * Temps mitjà de retorn
  * Ús de la CPU i throughput del sistema

Aquestes mètriques permeten comparar de manera objectiva els diferents algorismes de planificació.

---

## Com fer-ho servir
```sh
make
./main -a fcfs -m preemptive -f ./process.csv
./main -a fcfs -m nonpreemptive -f ./process.csv
./main -a priorities -m preemptive -f ./process.csv
./main -a priorities -m nonpreemptive -f ./process.csv
./main -a sjf -m preemptive -f ./process.csv
./main -a sjf -m nonpreemptive -f ./process.csv
./main -a rr -m preemptive -f ./process.csv
