#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <pthread.h>

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



FILE *input, *output;
Tarefa *tarefas_input, *tarefas, tarefa;	//tarefa =  compartilhada

sem_t mutex;
sem_t mutex2; 


void FirstComeFirstServed();
void RoundRobin();
void ShortestJobFirst();
void getTarefasArquivo();
int tamanho_alloc();
void getTarefas(int n);
Tarefa *criar_tarefas(Tarefa *t, int n);
Tarefa *alocarTarefa();


// execucao: ./sched RR input.txt output.txt
//simular execucao com comando sleep
//algoritmos: FCFS, RR, SJF  (first come first served, round robin, shortest job first)
//formato dos arquivos:
//3;0;10;1,5;		ou		2;3;6;;
//3 = ID, 0 = instante de chegada, 10 = tempo de execucao, comecou a fazer IO no instante 1 e terminou no instante 5



int main(int argc, char *argv[]) {
	int n_processos, tempo_total = 0, shmID;
	float tempo_medio = 0;
	long pid_filho1, pid_filho2;
	key_t key;






	 pid_filho1 = fork();

	//processo CPU
	 if(pid_filho1 == 0){
		 while(1){
			 sem_wait(&mutex);
			 sleep(QUANTUM);
			 tarefa.dados[2] -= QUANTUM;		//tempo restante de execucao
			 tempo_total += QUANTUM;
			 sem_post(&mutex);
		 }
	 }
	 else
	 {

		 pid_filho2 = fork();

		 //processo escalonador
		 if(pid_filho2 == 0){
			 input = fopen(argv[2], "r");
			 n_processos = tamanho_alloc();									//numero de processos a serem simulados
			 //compartilhar tarefa entre os processos
			 shmID = shmget(key, sizeof(Tarefa) * n_processos, IPC_CREAT | 0666);
			 tarefa = shmat(shmID, NULL, 0);

			 tarefas_input = criar_tarefas(tarefas_input, n_processos);		//armazena entrada do arquivo
			 getTarefasArquivo();
			 while(tarefas_input!=NULL){
				printf("%d;%d;%d;%d,%d;\n", tarefas_input->dados[0],tarefas_input->dados[1],tarefas_input->dados[2],tarefas_input->dados[3],tarefas_input->dados[4]);
				tarefas_input = tarefas_input->next;
			 }
			 
			 

			 if(!strcmp(argv[1], "FCFS"))
				 FirstComeFirstServed();
			 else if(!strcmp(argv[1], "RR"))
				 RoundRobin();
			 else if(!strcmp(argv[1], "SJF"))
				 ShortestJobFirst();

			 fclose(input);
		 }
	 }

	 output = fopen(argv[3], "w");
	 fclose(output);

	return 0;
}




/* ########################################################## */
/* ########################################################## */
/* 							FUNCOES							 */
/* ########################################################## */




void FirstComeFirstServed(){

}

/* ########################################################## */

void RoundRobin(){
	int instante = 0, escalonando = 1;
	Tarefa *aux;


	while(escalonando){
		sem_wait(&mutex);
		getTarefas(instante);

		tarefa = *tarefas;
		sem_post(&mutex);
		//esperar processador, decrementar tempo de execucao no processo do processador

		if(tarefa.dados[2]==0){		//tempo de execucao
			aux = tarefas;
			tarefas = tarefas->next;
			free(aux);
		}
		else
		{	//colocar tarefa executada no final da fila
			aux = tarefas;
			while(aux->next!=NULL)
				aux = aux->next;

			aux->next = tarefas;
			tarefas = tarefas->next;
		}

		if(tarefas==NULL)
			escalonando = 0;

		instante++;

	}

}

/* ########################################################## */

void ShortestJobFirst(){

}

/* ########################################################## */

void getTarefasArquivo(){
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

	if(tarefas==NULL)
		tarefas = criar_tarefas(tarefas,1);

	aux2 = tarefas;

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
