#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


#include <unistd.h>
#include <sys/types.h>

#define QUANTUM 1	//unidade de tempo de tarefa, 1 segundo de execucao


typedef struct t{
	int dados[5];	//0 = id, 1 = instante de chegada, ...
	/*
	 	int id;
		int instante_chegada;
		int tempo_execucao;
		int inicio_IO;
		int termino_IO;
	 */
}Tarefa;

void FirstComeFirstServed();
void RoundRobin();
void ShortestJobFirst();
void getTarefas();
int t_alloc();

FILE *input, *output;
Tarefa *tarefas_t;



// execucao: ./sched FCFS input.txt output.txt
//simular execucao com comando sleep
//algoritmos: FCFS, RR, SJF  (first come first served, round robin, shortest job first)
//formato dos arquivos:
//3;0;10;1,5;		ou		2;3;6;;
//3 = ID, 0 = instante de chegada, 10 = tempo de execucao, comecou a fazer IO no instante 1 e terminou no instante 5

int main(int argc, char *argv[]) {
	int i, n_processos;
	long pid_filho1, pid_filho2;
	Tarefa tarefa;	//tarefa compartilhada pela CPU e pelo escalonador, escolhida pra executar

	input = fopen(argv[2], "r");
	output = fopen(argv[3], "w");


	n_processos = t_alloc();	//numero de processos a serem simulados
	tarefas_t = (Tarefa*) calloc (n_processos, sizeof(Tarefa));
	getTarefas();


	//---------------------teste
	printf("\nNumero de processos: %d", n_processos);
		for(i=0; i <n_processos; i++)
			printf("\n%d;%d;%d;%d,%d;", tarefas_t[i].dados[0], tarefas_t[i].dados[1], tarefas_t[i].dados[2],
					tarefas_t[i].dados[3], tarefas_t[i].dados[4]);
	//---------------------teste


	 pid_filho1 = fork();

	 if(pid_filho1 == 0){
	 	 //processo pra CPU
	 }
	 else
	 {
		  //processo pra escalonador
		 pid_filho2 = fork();

		 if(pid_filho2 == 0){
			 if(!strcmp(argv[1], "FCFS"))
				 FirstComeFirstServed();
			 else if(!strcmp(argv[1], "RR"))
				 RoundRobin();
			 else if(!strcmp(argv[1], "SJF"))
				 ShortestJobFirst();
		 }
	 }


	fclose(input);
	fclose(output);

	return 0;
}

/* ########################################################## */


void FirstComeFirstServed(){

}

/* ########################################################## */

void RoundRobin(){

}

/* ########################################################## */

void ShortestJobFirst(){

}

/* ########################################################## */

void getTarefas(){
	char c, temp[100];
	int count = 0, i = 0, j = 0;

	rewind(input);

	while( (c = fgetc(input) ) != EOF){
		if(c == '\n'){
			count++;
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
				tarefas_t[count].dados[i++] = atoi(temp);
				memset(temp, 0, 100);
				j=0;
			}// ( c == ; )

		}// (c != \n)
	}//end while

}

/* ########################################################## */

int t_alloc(){
	char c;
	int i = 1;

	while ((c = fgetc(input)) != EOF)
		if( c == '\n'){
			i++;
		}

	return i;
}

/* ########################################################## */


