#ifndef TP_BALCAO_H
#define TP_BALCAO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "medico.h"
#include "cliente.h"



#define MAXESPECIALIDADES 5
#define MAX 256

// nome do FIFO do Balcão
#define BALCAO_FIFO "balcao_fifo"
// nome do FIFO de cada Cliente - %d = pid
#define CLIENTE_FIFO "cliente_%d_fifo"
// nome do FIFO de cada Medico - %d = pid
#define MEDICO_FIFO "medico_%d_fifo"


enum especialidades{
    oftalmologia = 0,
    neurologia = 1,
    estomatologia = 2,
    ortopedia = 3,
    geral = 4
};


typedef struct balcao Balcao, *pBalcao;
struct balcao{
    int maxClientes;
    int maxMedicos;
    int numClientes;
    int numMedicos;
    int numEspecialistas[MAXESPECIALIDADES]; // numero de especialistas por area
    int filaEspera[MAXESPECIALIDADES][5]; //numero de pessoas em fila de espera por area
    int tempoFila;
    pthread_mutex_t mutex;

    //Exemplo:
    //filaEspera[0] - 1 utente - oftalmologia
    //filaEspera[1] - 3 utentes (na fila de espera) - neurologia
    // ...

};

//Estrutura da mensagem: cliente -> balcao
typedef struct {
    pid_t	pid;
    int medico_cliente; // medico = 0; cliente = 1
    char nome[30];
    int  atendido; //1 - a atender/ser atendido
    char especialidade[30];
    char	mensagem[MAX];
} Mensagem_utilizador_para_Balcao;



//Estrutura da mensagem entre cliente e medico

typedef struct {
    char	mensagem[MAX];
} Consulta;


//Estrutura da mensagem de resposta do balcao
typedef struct {
    pid_t pid_balcao;
    pid_t pid_medico;
    pid_t pid_cliente;
    char	mensagem[MAX];
} Mensagem_Balcao;






long long tempoAtual();
void encerra();
void help();
const char* classifica(int pid,int b2c[2], int c2b[2],char *sintomas);
void comandos(Balcao *balcao, pCliente listaUtentes, pMedico listaMedicos);

Balcao inicializarDadosBalcao(int MAXMEDICOS, int MAXCLIENTES);
Medico inicializarMedico();
void terminaClientesMedicos(Balcao *balcao, pCliente listaUtentes,pMedico listaMedicos);

void *mostraListaXsec(void *balcaoo);
void alteraTempo(Balcao *b,int tempo);
int devolveFila(char *especialidade);

int validaDelut(Balcao *balcao, pCliente listaUtentes,int id);
int validaDelesp(Balcao *balcao, pMedico listaMedicos,int id);

/// MEDICO
void adicionarMedico(Balcao *b, pMedico listaMedicos, int id, Medico medico);
void removerMedico(Balcao *b, pMedico medico, int id);
void mostrarTodosMedicos(Balcao *b, pMedico medico);
void mostrarDadosMedico(pid_t id, Medico medico);
void sinal_vida(int s);

/// CLIENTE
void adicionarCliente(Balcao *b, pCliente utente, int id, Cliente cliente);
void removerCliente(Balcao *b,pCliente utente, int id);
void mostrarDadosCliente(int id, Cliente utente);
void mostrarTodosClientes(Balcao *b, pCliente utente);
Cliente inicializarCliente();
void AtribuiLista(Balcao *b, Cliente cli);
void organizaLista(Balcao *b, pCliente cli);
void removeFromLista(Balcao *b,pid_t pid);

void mostraListasEspera(Balcao *b);


#endif //TP_BALCAO_H
