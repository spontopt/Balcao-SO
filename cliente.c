#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>
#include "balcao.h"
#include "cliente.h"


    Cliente cliente;
    char cliente_FIFO_fnome[30]; //nome do FIFO do cliente
    char medico_FIFO_fnome[30]; //nome do FIFO do cliente
    int balcao_fifo_fd; //identificador do FIFO do balcao
    int cliente_fifo_fd; //identificador do FIFO do cliente
    int medico_fifo_fd; //identificador do FIFO do cliente
    int pidMedico;

void trataSinalBalcaoTerminou(int i) {
    (void) i;

    int pidCliente = getpid();
    sprintf(cliente_FIFO_fnome, CLIENTE_FIFO, pidCliente);
    fprintf(stderr, "[INFO] O Balcão foi terminado!\n\nA terminar...\n");
    unlink(cliente_FIFO_fnome);
    exit(EXIT_SUCCESS);
}

void trataSinal(int i) {
    (void) i;
    fprintf(stderr, "\n [INFO] Cliente interrompido via teclado!\n");

    if(cliente.atendido == 1){
        Consulta consulta;
        strcpy(consulta.mensagem,"erro\n");
        sprintf(medico_FIFO_fnome, MEDICO_FIFO, pidMedico);

       // Abrir o FIFO do medico para escrita/leitura
        medico_fifo_fd = open(medico_FIFO_fnome,O_RDWR); 
        if(medico_fifo_fd == -1){
            fprintf(stderr, "[INFO] Erro ao abrir FIFO do medico!\n");
           exit(EXIT_FAILURE);
        }
        fprintf(stderr, "\n[INFO] Consulta terminada!\n\nA encerrar...\n");
        //Enviar mensagem ao medico a dizer q terminou
        write(medico_fifo_fd, &consulta, sizeof(consulta));
        close(medico_fifo_fd);
    }


    terminaCliente();
}

void terminaCliente(){

    int pid = getpid();

    sprintf(cliente_FIFO_fnome, CLIENTE_FIFO, pid);
    Mensagem_utilizador_para_Balcao mens_balc_cli;
    
    //Abrir o FIFO do balcão para escrita
    balcao_fifo_fd = open(BALCAO_FIFO,O_WRONLY);
    if(balcao_fifo_fd == -1){
        fprintf(stderr, "[INFO] Balcão não está em funcionamento!\n");
        unlink(cliente_FIFO_fnome);
        exit(EXIT_FAILURE);
    }
    
    //Enviar dados do Cliente pela estrutura Mensagem
    mens_balc_cli.pid = pid;
    mens_balc_cli.medico_cliente = 1; // 1 = cliente
    mens_balc_cli.atendido = -1;      // -1 = terminou
    write(balcao_fifo_fd, &mens_balc_cli, sizeof(mens_balc_cli));
   
    close(balcao_fifo_fd);  
    unlink(cliente_FIFO_fnome);
    exit(EXIT_SUCCESS);
}



int main(int argc, char *argv[]){

    Mensagem_Balcao mens_balc_cli;
    Mensagem_utilizador_para_Balcao mens_cli_balc;
    Consulta consulta;
    int read_res;

    int nfd; //valor retorno do select
    fd_set read_fds;
    int nfd2; //valor retorno do select2
    fd_set read_fds2;

    char sintomas[MAX];
    int pid_Cliente = getpid();


    //verificar se existe um Balcão em funcionamento
    if(access(BALCAO_FIFO, F_OK) != 0){
        printf("[INFO] Balcão não está em funcionamento!\n");
        exit(EXIT_FAILURE);
    }

    if(argc != 2){
        printf("[INFO] Número de parâmetros incorreto! Sintaxe: ./cliente <nome>\n");
        exit(EXIT_FAILURE);
    }


    //Criar FIFO deste cliente
    mens_cli_balc.pid = pid_Cliente;
    mens_cli_balc.medico_cliente = 1;
    mens_cli_balc.atendido = 0;
    strcpy(mens_cli_balc.nome,argv[1]);
   // printf("TAMANHO : %ld\n",sizeof (mens_cli_balc));


    sprintf(cliente_FIFO_fnome, CLIENTE_FIFO, mens_cli_balc.pid);
    if(mkfifo(cliente_FIFO_fnome,0777) == -1){
        fprintf(stderr, "\n[INFO] Erro na criação do FIFO do cliente!\n");
        exit(EXIT_FAILURE);
    }
   // fprintf(stderr, "[INFO] FIFO do cliente [%d] criado com sucesso!\n",mens_cli_balc.pid);


    //Abrir o FIFO do balcão para escrita
    balcao_fifo_fd = open(BALCAO_FIFO,O_WRONLY); // bloqueante
    if(balcao_fifo_fd == -1){
        fprintf(stderr, "[INFO] Balcão não está em funcionamento!\n");
        unlink(cliente_FIFO_fnome);
        exit(EXIT_FAILURE);
    }
    //fprintf(stderr, "[INFO] FIFO do Balcão aberto para escrita\n");

    //Abrir o FIFO do cliente para escrita/leitura
    cliente_fifo_fd = open(cliente_FIFO_fnome,O_RDWR); // bloqueante
    if(cliente_fifo_fd == -1){
        fprintf(stderr, "[INFO]  Erro ao abrir FIFO do cliente!\n");
        close(balcao_fifo_fd);
        unlink(cliente_FIFO_fnome);
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "[INFO] FIFO do cliente aberto para leitura e escrita\n");


    //Para interromper via CTRL-C
    if (signal(SIGINT, trataSinal) == SIG_ERR) {
        perror("\nNão foi possível configurar o sinal SIGINT\n");
        exit(EXIT_FAILURE);
    }

    //Para interromper caso o Balcao termine
    if (signal(SIGHUP, trataSinalBalcaoTerminou) == SIG_ERR) {
        perror("\nNão foi possível configurar o sinal SIGUSR1\n");
        exit(EXIT_FAILURE);
    }

    printf("\n----------- MEDICALso -----------\n\n");
    printf("Bem-vindo/a '%s'\n\n", argv[1]);


    while(1){

        printf("Introduza os seus sintomas: \n");
        fgets(mens_cli_balc.mensagem, sizeof(mens_cli_balc.mensagem) - 1, stdin);



        int tam = strlen(mens_cli_balc.mensagem);
        mens_cli_balc.mensagem[tam] = '\0';


        //Envia dados do cliente com os sintomas
        write(balcao_fifo_fd, &mens_cli_balc, sizeof(mens_cli_balc));

        //Recebe dados do cliente do Balcao
        read_res = read(cliente_fifo_fd, &cliente, sizeof (cliente));

        if(read_res == sizeof (cliente)){
            if(cliente.prioridade == -1){
                //Termina
                fprintf(stderr, "[INFO] Limite máximo de Clientes atingido para esta especialidade!\n\nA terminar...");
                close(cliente_fifo_fd);
                close(balcao_fifo_fd);
                unlink(cliente_FIFO_fnome);
                exit(EXIT_FAILURE);
                    
            }else if(cliente.prioridade == -2){
                //Termina
                fprintf(stderr, "[INFO] Limite máximo de Clientes no sistema atingido!\n\nA terminar...");
                close(cliente_fifo_fd);
                close(balcao_fifo_fd);
                unlink(cliente_FIFO_fnome);
                exit(EXIT_FAILURE);
            }else{
             fprintf(stdout, "\nEspecialidade atribuída: %s\n", cliente.areaEspecialidade);
             fprintf(stdout, "Prioridade atribuída: %d\n", cliente.prioridade);
             fprintf(stdout, "Posição Lista Espera: %d\n", cliente.posicaoListaEspera);
             break;
            }
        }
        else
            fprintf(stderr, "[INFO] Mensagem recebida incompleta! [Bytes lidos: %d]\n", read_res);
    }

    printf("\n[INFO] Aguarde pelo seu médico!\n");


    while(1){


            FD_ZERO(&read_fds2);
            FD_SET(0, &read_fds2);
            FD_SET(cliente_fifo_fd, &read_fds2);
            nfd2 = select(cliente_fifo_fd + 1, &read_fds2, NULL, NULL, NULL);

            if (nfd2 == 0) {           
                    //  printf("\n[INFO] À espera...\n");
                    fflush(stdout);
                    continue;
            }

            if (nfd2 == -1) {
                    perror("\n[INFO] Erro na criação do 1º select!\n");
                    close(medico_fifo_fd);
                    exit(EXIT_FAILURE);
             }


            if (FD_ISSET(0, &read_fds2)) {
    
                char mensagem[255];
                fgets(mensagem, sizeof(mensagem), stdin);// le o \n
            
               if(strcmp(mensagem,"sair\n") == 0){
            
                fprintf(stdout, "\n[INFO] A terminar...\n"); 
               
                mens_cli_balc.medico_cliente = 1;
                mens_cli_balc.pid = cliente.id;
                mens_cli_balc.atendido = -3;
                write(balcao_fifo_fd, &mens_cli_balc, sizeof(mens_cli_balc)); 


                unlink(cliente_FIFO_fnome);
                close(cliente_fifo_fd);
                close(balcao_fifo_fd);
                exit(EXIT_FAILURE);
             } 
            }



        if (FD_ISSET(cliente_fifo_fd, &read_fds2))  {

                //Receber o medico atribuido pelo balcao
                read_res = read(cliente_fifo_fd, &mens_balc_cli, sizeof (mens_balc_cli));
                        if(read_res == sizeof (mens_balc_cli))
                            fprintf(stdout, "\n[INFO]Medico '%s' atribuído: [%d]\n",mens_balc_cli.mensagem, mens_balc_cli.pid_medico);
                        else
                            fprintf(stderr, "[INFO] Mensagem recebida incompleta! [Bytes lidos: %d]\n", read_res);
            

                pidMedico = mens_balc_cli.pid_medico;

                sprintf(medico_FIFO_fnome, MEDICO_FIFO, mens_balc_cli.pid_medico);
                //Abrir o FIFO do medico para escrita/leitura
                medico_fifo_fd = open(medico_FIFO_fnome,O_RDWR); 
                if(medico_fifo_fd == -1){
                    fprintf(stderr, "[INFO]  Erro ao abrir FIFO do medico!\n");
                    close(balcao_fifo_fd);
                    unlink(medico_FIFO_fnome);
                    unlink(cliente_FIFO_fnome);
                    exit(EXIT_FAILURE);
                }
                cliente.atendido = 1;
                fprintf(stderr, "\nConsulta iniciada. Pode agora comunicar com o seu médico!\n");
            break;
        }
    }



    while(1){

        FD_ZERO(&read_fds);
        FD_SET(0, &read_fds);
        FD_SET(cliente_fifo_fd, &read_fds);

        nfd = select(cliente_fifo_fd + 1, &read_fds, NULL, NULL, NULL);

        if (nfd == 0) {
            //  printf("\n[INFO] À espera...\n");
            fflush(stdout);
            continue;
        }

        if (nfd == -1) {
            perror("\n[INFO] Erro na criação do select!\n");
            close(cliente_fifo_fd);
            close(medico_fifo_fd);
            unlink(BALCAO_FIFO);
            exit(1);
        }



        if (FD_ISSET(0, &read_fds)) {

            //ENVIAR MENSAGENS PARA O MEDICO
            char mensagem[255];
            fgets(mensagem, sizeof(mensagem), stdin);// le o \n
            strcpy(consulta.mensagem, mensagem);

            //Envia mensagem para o medico
            read_res = write(medico_fifo_fd, &consulta, sizeof(consulta));
             if(strcmp(consulta.mensagem,"adeus\n") == 0){

                fprintf(stdout, "\n[INFO] A terminar...\n"); 
                 break;
             }
             if(strcmp(consulta.mensagem,"sair\n") == 0){

                fprintf(stdout, "\n[INFO] A terminar...\n"); 
                 break;
             }


            if(!read_res == sizeof (consulta))
                fprintf(stderr, "[INFO] Mensagem enviada incompleta! [Bytes lidos: %d]\n", read_res);
        }


        if (FD_ISSET(cliente_fifo_fd, &read_fds)) {

            //RECEBER MENSAGENS DO MEDICO
            read_res = read(cliente_fifo_fd, &consulta, sizeof(consulta));

            if(read_res == sizeof (consulta)){
                if(strcmp(consulta.mensagem,"adeus\n") == 0 || strcmp(consulta.mensagem,"sair\n") == 0){
                    fprintf(stdout, "[Médico]: %s", consulta.mensagem);      
                    fprintf(stderr, "\n[INFO] Consulta terminada!\n\nA encerrar...\n");
                    break;
                 }
                if(strcmp(consulta.mensagem,"erro\n") == 0){
                    fprintf(stderr, "[INFO] O seu médico saiu da consulta sem aviso prévio!\n");
                    break;
                }
                fprintf(stdout, "[Médico]: %s", consulta.mensagem);
            }
            else
                fprintf(stderr, "[INFO] Mensagem enviada incompleta! [Bytes lidos: %d]\n", read_res);
        } 
    }

    terminaCliente();
    return 0;
}
