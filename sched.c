/* Universidade Federal da Bahia - UFBA
 * Alunos: Evaldo Moreira, Lucas Borges, Thiago Pacheco
 * 
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/mman.h>

#define QUANTUM 1	//unidade de tempo de tarefa, 1 segundo de execucao


typedef struct t{
	int dados[5];
	/*
	 	int id;					indice 0
		int instante_chegada;	...
		int tempo_execucao;		...
		int inicio_IO;			...
		int termino_IO;			indice 4
	 */
	struct t *next;
}Tarefa;

/*
 Compartilhar tarefa entre processos:
int shmid;
key_t key;
Tarefa* tarefa;

shmid = shmget(key, sizeof(Tarefa) * n_processos, IPC_CREAT | 0666);
tarefa = shmat(shmid, NULL, 0);
*/

/*
1;0;3
2;3;8
3;1;12
...
Tempo total: 12
Tempo médio: 7.67

gerar saida:
print tempo total: x
print tempo medio: y

verificar o id de cada tarefa executada e por resultado num vetor de n_processos
tempo medio = tempo total da tarefa 1 + tempo total da tarefa 2 + ... dividido pelo n_processos

*/

FILE *input, *output;
Tarefa *tarefas_input, *tarefas_output, *lista_tarefas, *tarefa;	//tarefa =  compartilhada
int *instante;

sem_t *mutex;
sem_t *mutex2; 


void FirstComeFirstServed();
void RoundRobin();
void ShortestJobFirst();
void lerEntradasArquivo();
int tamanho_alloc();
void getTarefas(int n);
Tarefa *criar_tarefas(Tarefa *t, int n);
Tarefa *alocarTarefa();


// execucao: ./sched RR input.txt output.txt
//simular execucao com comando sleep
//algoritmos: FCFS, RR, SJF  (first come first served, round robin, shortest job first)
//3 = ID, 0 = instante de chegada, 10 = tempo de execucao, comecou a fazer IO no instante 1 e terminou no instante 5

//compilar:
//gcc -pthread sched.c -o sched

int main(int argc, char *argv[]) {
	int n_processos, tempo_total = 0, shmID_1, shmID_2, status;
	float tempo_medio = 0;
	long pid_filho1, pid_filho2;
	key_t key_1 = 567194, key_2 = 567193;

	
	input = fopen(argv[2], "r");
	n_processos = tamanho_alloc();			//numero de processos a serem simulados
	fclose(input);
		
	
	//compartilhar tarefa e instante entre os processos
	instante = (int*) malloc (sizeof(int));
	*instante = 0;
	shmID_2 = shmget(key_2, sizeof(int), IPC_CREAT | 0666);
	instante = shmat(shmID_2, NULL, 0);
	
	tarefas_input = criar_tarefas(tarefas_input, n_processos);		//armazena entrada do arquivo

	if ( (shmID_1 = shmget(key_1, sizeof(Tarefa) * n_processos, IPC_CREAT | 0666)) < 0 ){
		perror("shmget error");
		exit(1);
	}
	if( (tarefa = shmat(shmID_1, NULL, 0)) == (Tarefa*) -1){
		perror("shmat error");
		exit(1);
	}
	
	
	mutex  = mmap(NULL,sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	mutex2 = mmap(NULL,sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	
	if(!mutex || !mutex2)
		perror("mmap error");
		
		
	//iniciados em 0 e 1
	sem_init(mutex, 1, 0);
	sem_init(mutex2, 1, 1);
	
	
	pid_filho2 = fork();
	if(pid_filho2 != 0){
		pid_filho1 = fork();
		wait(&status);
	}
	
	
	//processo CPU
	 if(pid_filho1 == 0){
		fprintf(stderr, "[Processo CPU]...\n");
		 int i;
		 tarefas_output = (Tarefa*) calloc(n_processos, sizeof(Tarefa));
		 for(i=0; i < n_processos; i++){
			tarefas_output[i].dados[0] = i;
			tarefas_output[i].dados[1] = -1;
		}
		 while(1){
			 sem_wait(mutex);
			 if(tarefa==NULL)
				break;
			 fprintf(stderr, "\nExecutando Tarefa %d...", tarefa->dados[0]);
			 sleep(QUANTUM);
			 tarefa->dados[2] -= QUANTUM;		//tempo restante de execucao
			 tempo_total += QUANTUM;
			
			 if(tarefas_output[ tarefa->dados[0] ].dados[1] == -1)
				 tarefas_output[ tarefa->dados[0] ].dados[1] = (*instante);
			 else
			 	tarefas_output[ tarefa->dados[0] ].dados[2] += QUANTUM;
			
			 sem_post(mutex2);
		 }
		 
		 //salvar saida
		 output = fopen(argv[3], "w");
		 for(i=0; i < n_processos; i++)
			fprintf(output, "%d;%d;%d;\n", tarefas_output[i].dados[0], tarefas_output[i].dados[1], tarefas_output[i].dados[2]);
		 
		 fprintf(output, "Tempo Total: %d\nTempo medio: %f", tempo_total, tempo_medio);
		 fclose(output);
		 exit(0);
		 
	 }
	 
	 
	 //processo escalonador
	 if(pid_filho2 == 0){
		 fprintf(stderr, "[Processo Escalonador]...\n ");
		 input = fopen(argv[2], "r");
			 			
		 lerEntradasArquivo();
			
				 
		 if(!strcmp(argv[1], "FCFS"))
			 FirstComeFirstServed();
		 else if(!strcmp(argv[1], "RR"))
			 RoundRobin();
		 else if(!strcmp(argv[1], "SJF"))
			 ShortestJobFirst();

		 fclose(input);
		 exit(0);
	 }
	 
	 
	 sem_destroy(mutex);
	munmap(mutex, sizeof(sem_t));
	sem_destroy(mutex2);
	munmap(mutex2, sizeof(sem_t));

		

	
	return EXIT_SUCCESS;
}




/* ########################################################## */
/* ########################################################## */
/* 							FUNCOES							 */
/* ########################################################## */




void FirstComeFirstServed(){

}

/* ########################################################## */

void RoundRobin(){
	int escalonando = 1;
	Tarefa *aux;


	while(escalonando){
		getTarefas(*instante);
		tarefa = lista_tarefas;

		fprintf(stderr, "\nEscalonando, tarefa%d no inst %d", tarefa->dados[0], *instante);
		sem_wait(mutex2);
		//esperar processador, decrementar tempo de execucao no processo do processador
		sem_post(mutex);
		sem_wait(mutex2);
		if(tarefa->dados[2]==0){		//tempo de execucao
			aux = lista_tarefas;
			lista_tarefas = lista_tarefas->next;
			free(aux);
		}
		else
		{	//colocar tarefa executada no final da fila
			aux = lista_tarefas;
			while(aux->next!=NULL)
				aux = aux->next;

			aux->next = lista_tarefas;
			lista_tarefas = lista_tarefas->next;
		}
		
		if(lista_tarefas==NULL)
			escalonando = 0;

		(*instante)++;
		sem_post(mutex2);

	}

}

/* ########################################################## */

void ShortestJobFirst(){

}

/* ########################################################## */

void lerEntradasArquivo(){
	char c, temp[100];
	int i = 0, j = 0;
	Tarefa *aux = tarefas_input;

	rewind(input);


	while( (c = fgetc(input) ) != EOF){
		if(c == '\n'){
			aux = aux->next;
			i=0;
			j=0;
		}
		else
		{	if(isdigit(c)){
				temp[j++] = c;
				temp[j] = '\0';

			}
			else
			{
				aux->dados[i++] = atoi(temp);
				memset(temp, 0, 100);
				j=0;
			}// ( c == ; )

		}// (c != \n)
	}//end while

}

/* ########################################################## */

void getTarefas(int n){
	Tarefa *input_aux = tarefas_input, *aux2, *ant;

	if(lista_tarefas==NULL)
		lista_tarefas = criar_tarefas(lista_tarefas,1);

	aux2 = lista_tarefas;

	while(aux2!=NULL){
		ant = aux2;
		aux2 = aux2->next;
	}

	while(input_aux!=NULL){
		if(input_aux->dados[1] == n){				//instante de chegada
			aux2 = criar_tarefas(aux2, 1);
			aux2 = input_aux;
			ant->next = aux2;
		}

		ant = aux2;
		aux2 = aux2->next;
		input_aux = input_aux->next;
	}


}

/* ########################################################## */

int tamanho_alloc(){
	char c;
	int i = 1;

	while ((c = fgetc(input)) != EOF)
		if( c == '\n'){
			i++;
		}

	return i;
}

/* ########################################################## */

Tarefa *criar_tarefas(Tarefa *t, int n){
	int i;
	Tarefa *aux, *ant;

	aux = alocarTarefa();

	t = aux;
	ant = aux;
	aux = aux->next;


	for(i=1; i < n; i++){
		aux = alocarTarefa();
		ant->next = aux;
		ant = aux;
		aux = aux->next;
	}


	aux = t;
	for(i=0; i < n; i++)
		if(aux!=NULL)
			aux = aux->next;

	return t;
}

/* ########################################################## */

Tarefa *alocarTarefa(){
	Tarefa *temp;
	int i;

	temp = (Tarefa*) malloc (sizeof(Tarefa));
	temp->next = NULL;

	for(i=0; i < 5; i ++)
		temp->dados[i] = 0;

	return temp;
}

/* ########################################################## */
