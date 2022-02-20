#ifndef TP_CLIENTE_H
#define TP_CLIENTE_H


typedef struct cliente Cliente, *pCliente;
struct cliente{

    char nome[30];
    char sintomas[100];
    char areaEspecialidade[100];
    int prioridade;
    int id; // 0 - cliente nao existe  // > 0, - cliente existe e o id corresponde ao seu pid
    int posicaoListaEspera;
    int atendido;
    pid_t medicoAtribuido;
};

void trataSinalBalcaoTerminou(int i);
void trataSinal(int i);
void terminaCliente();

#endif //TP_CLIENTE_H
