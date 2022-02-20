#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "balcao.h"
#include "medico.h"


    Medico medico;
    char cliente_FIFO_fnome[30]; //nome do FIFO do cliente
    char medico_FIFO_fnome[30]; //nome do FIFO do cliente
    int balcao_fifo_fd; //identificador do FIFO do balcao
    int cliente_fifo_fd; //identificador do FIFO do cliente
    int medico_fifo_fd; //identificador do FIFO do cliente
    int pidCliente;


void trataSinalBalcaoTerminou(int i) {
    (void) i;

    int pidMedico = getpid();
    sprintf(medico_FIFO_fnome, CLIENTE_FIFO, pidMedico);
    fprintf(stderr, "[INFO] O Balcão foi terminado!\n\nA terminar...\n");
    unlink(medico_FIFO_fnome);
    exit(EXIT_SUCCESS);
}

void trataSignal(int i) {
    (void) i;
    fprintf(stderr, "\n [INFO] Médico interrompido via teclado!\n\n");
    int pid = getpid();

    sprintf(medico_FIFO_fnome, MEDICO_FIFO, pid);
    Consulta consulta;

    if(medico.estado == 1){
        strcpy(consulta.mensagem,"erro\n");
        sprintf(cliente_FIFO_fnome, CLIENTE_FIFO, pidCliente);
        //Abrir o FIFO do cliente 
        cliente_fifo_fd = open(cliente_FIFO_fnome,O_RDWR); 
        if(cliente_fifo_fd == -1){
            fprintf(stderr, "[INFO]  Erro ao abrir FIFO do Cliente!\n");
            unlink(medico_FIFO_fnome);
            exit(EXIT_FAILURE);
        }

        //Enviar mensagem ao cliente a dizer q terminou
        write(cliente_fifo_fd, &consulta, sizeof(consulta));
        close(cliente_fifo_fd);
    }
        unlink(medico_FIFO_fnome);
        exit(EXIT_SUCCESS);
}

void *isAlive(){
 
    while(1){
        Mensagem_utilizador_para_Balcao mens_med_balc;
        mens_med_balc.pid = getpid();
        mens_med_balc.medico_cliente = 0; // 0 = medico
        mens_med_balc.atendido = -1;  // -1 = sinal de vida

        //Enviar sinal de vida para o balcão
        write(balcao_fifo_fd, &mens_med_balc, sizeof(mens_med_balc));       
        sleep(20);
    }   
}

Medico atribuirDadosMedico( char *nome, char *especialidade,int pidM){

    Medico medico;
    strcpy(medico.nome,nome);
    strcpy(medico.especialidade,especialidade);
    medico.id = pidM;
    medico.estado = 0;

    return medico;
}


int main(int argc, char *argv[]){


    Mensagem_Balcao mens_balc_med;
    Mensagem_utilizador_para_Balcao mens_med_balc;
    Consulta consulta;

    int nfd; //valor retorno do select
    fd_set read_fds;
    int nfd2; //valor retorno do select2
    fd_set read_fds2;

    int read_res;
    int pid_Medico = getpid();

    //verificar se já existe um Balcão em funcionamento
    if(access(BALCAO_FIFO, F_OK) != 0){
        printf("[INFO] Balcão não está em funcionamento!\n");
        exit(EXIT_FAILURE);
    }

    //Receber os argumentos e inicializar estrutura do médico
    if(argc == 3){
        sscanf(argv[1],"%s",medico.nome);
        sscanf(argv[2],"%s",medico.especialidade);
    }else{
        printf("[INFO] Número de parâmetros incorreto! Sintaxe: ./medico <nome> <especialidade>\n");
         exit(EXIT_FAILURE);
    }
    medico.id = pid_Medico;
    medico.estado = 0;


    //Definir valores da estrutura Mensagem
    mens_med_balc.pid = pid_Medico;
    mens_med_balc.medico_cliente = 0;
    mens_med_balc.atendido = medico.estado;
    strcpy(mens_med_balc.nome,medico.nome);
    strcpy(mens_med_balc.mensagem,medico.especialidade);

    //Criar FIFO deste medico
    sprintf(medico_FIFO_fnome, MEDICO_FIFO, medico.id);
    if(mkfifo(medico_FIFO_fnome,0777) == -1){
        fprintf(stderr, "\n[INFO] Erro na criação do FIFO do medico!\n");
        exit(EXIT_FAILURE);
    }
   // fprintf(stderr, "[INFO] FIFO do Médico [%d] criado com sucesso!\n",medico.id);

    //Abrir o FIFO do balcão para escrita
    balcao_fifo_fd = open(BALCAO_FIFO,O_WRONLY);
    if(balcao_fifo_fd == -1){
        fprintf(stderr, "[INFO] Balcão não está em funcionamento!\n");
        unlink(medico_FIFO_fnome);
        exit(EXIT_FAILURE);
    }
    //fprintf(stderr, "[INFO] FIFO do Balcão aberto para escrita\n");

    //Enviar dados do Medico pela estrutura Mensagem
    write(balcao_fifo_fd, &mens_med_balc, sizeof(mens_med_balc));


    //Abrir o FIFO do medico para escrita/leitura
    medico_fifo_fd = open(medico_FIFO_fnome,O_RDWR);
    if(medico_fifo_fd == -1){
        fprintf(stderr, "[INFO] Erro ao abrir FIFO do Medico!\n");
        close(balcao_fifo_fd);
        unlink(medico_FIFO_fnome);
        exit(EXIT_FAILURE);
    }
    //fprintf(stderr, "[INFO] FIFO do Medico aberto para leitura e escrita\n");

    //Receber confirmação do sistema
    read_res = read(medico_fifo_fd, &mens_balc_med, sizeof (mens_balc_med));
    if(read_res == sizeof (mens_balc_med)){
        fprintf(stdout, "%s", mens_balc_med.mensagem);

        if(strcmp(mens_balc_med.mensagem, "[INFO] Limite máximo de Médicos atingido!\n") == 0){
            close(balcao_fifo_fd);
            unlink(medico_FIFO_fnome);
            exit(EXIT_FAILURE);
        }

    }else
         fprintf(stderr, "[INFO] Mensagem recebida incompleta! [Bytes lidos: %d]\n", read_res);



    //Thread para enviar sinal de vida
    pthread_t thread;
    if(pthread_create(&thread,NULL,&isAlive,NULL) != 0){
        perror("\nNão foi possível criar a Thread\n");
      exit(EXIT_FAILURE);
     }

    //Para interromper via CTRL-C
    if (signal(SIGINT, trataSignal) == SIG_ERR) {
        perror("\nNão foi possível configurar o sinal SIGINT\n");
        exit(EXIT_FAILURE);
    }

   //Para interromper caso o Balcao termine
    if (signal(SIGHUP, trataSinalBalcaoTerminou) == SIG_ERR) {
        perror("\nNão foi possível configurar o sinal SIGUSR1\n");
        exit(EXIT_FAILURE);
    }



    printf("\n\n----------- MEDICALso -----------\n\n");
    printf("Bem-vindo/a '%s'\n\n",medico.nome);
    printf("Médico [%d]:\n\tNome: %s\n\tEspecialidade: %s\n\tEstado: %d\n\tPid: %d\n\n",0,medico.nome,medico.especialidade,medico.estado,medico.id);


    while(1){

            printf("\n[INFO] Aguarde pelo seu utente!\n");


            FD_ZERO(&read_fds2);
            FD_SET(0, &read_fds2);
            FD_SET(medico_fifo_fd, &read_fds2);
            nfd2 = select(medico_fifo_fd + 1, &read_fds2, NULL, NULL, NULL);

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
               
                mens_med_balc.medico_cliente = 0;
                mens_med_balc.pid = medico.id;
                mens_med_balc.atendido = -3;
                write(balcao_fifo_fd, &mens_med_balc, sizeof(mens_med_balc)); 


                unlink(medico_FIFO_fnome);
                close(medico_fifo_fd);
                close(balcao_fifo_fd);
                exit(EXIT_FAILURE);
             } 
            }
            

        if (FD_ISSET(medico_fifo_fd, &read_fds2))  {

            //Receber o cliente atribuido pelo balcao
            memset(&mens_balc_med, 0, sizeof(mens_balc_med));
            read_res = read(medico_fifo_fd, &mens_balc_med, sizeof (mens_balc_med));

            if(read_res == sizeof (mens_balc_med))
                fprintf(stdout, "\n[INFO] Cliente '%s' atribuído: [%d]\n",mens_balc_med.mensagem, mens_balc_med.pid_cliente);
            else
                fprintf(stderr, "\n[INFO] Mensagem recebida incompleta! [Bytes lidos: %d]\n", read_res);


            pidCliente = mens_balc_med.pid_cliente;
            sprintf(cliente_FIFO_fnome, CLIENTE_FIFO, mens_balc_med.pid_cliente);
            medico.estado = 1;
            //Abrir o FIFO do cliente para escrita/leitura
            cliente_fifo_fd = open(cliente_FIFO_fnome,O_RDWR); // bloqueante
            if(cliente_fifo_fd == -1){
                fprintf(stderr, "[INFO]  Erro ao abrir FIFO do cliente!\n");
                exit(EXIT_FAILURE);
            }

            // fprintf(stderr, "[INFO] FIFO do cliente aberto para leitura e escrita\n");
            fprintf(stderr, "\nConsulta iniciada. Pode agora comunicar com o seu utente!\n");

        }



        while(1) {

            FD_ZERO(&read_fds);
            FD_SET(0, &read_fds);
            FD_SET(medico_fifo_fd, &read_fds);
            nfd = select(medico_fifo_fd + 1, &read_fds, NULL, NULL, NULL);

            if (nfd == 0) {           
                //  printf("\n[INFO] À espera...\n");
                fflush(stdout);
                continue;
            }

            if (nfd == -1) {
                perror("\n[INFO] Erro na criação do 2º select!\n");
                close(medico_fifo_fd);
                close(cliente_fifo_fd);
                exit(EXIT_FAILURE);
            }


            if (FD_ISSET(0, &read_fds)) {
    
                char mensagem[255];
                fgets(mensagem, sizeof(mensagem), stdin);// le o \n
                strcpy(consulta.mensagem, mensagem);

                //Envia mensagem para o cliente
                read_res = write(cliente_fifo_fd, &consulta, sizeof(consulta));
                if (read_res != sizeof(consulta))
                    fprintf(stderr, "[INFO] Mensagem enviada incompleta! [Bytes lidos: %d]\n", read_res);
               if(strcmp(consulta.mensagem,"adeus\n") == 0 || strcmp(consulta.mensagem,"sair\n") == 0)
                 break;           
            
            }

            if (FD_ISSET(medico_fifo_fd, &read_fds)) 
            {
                if(medico.estado == 1){
                    //RECEBER MENSAGENS DO CLIENTE
                    read_res = read(medico_fifo_fd, &consulta, sizeof(consulta));

                    if (read_res == sizeof(consulta)){
                        if(strcmp(consulta.mensagem,"adeus\n") == 0){
                           fprintf(stdout, "[Utente]: %s", consulta.mensagem);
                           medico.estado = 0;
                           break;
                        }
                        if(strcmp(consulta.mensagem,"sair\n") == 0){
                           fprintf(stdout, "[Utente]: %s", consulta.mensagem);
                           medico.estado = 0;
                           break;
                        }



                        if(strcmp(consulta.mensagem,"erro\n") == 0){
                            fprintf(stderr, "[INFO] O cliente saiu da consulta sem aviso prévio!\n");
                            medico.estado = 0;
                            break;
                        }
                        fprintf(stdout, "[Utente]: %s", consulta.mensagem);
                    }        
                }
            }
        }

        fprintf(stdout, "\n[INFO] Fim da Consulta\n"); 
        medico.estado = 0;
        write(balcao_fifo_fd, &mens_med_balc, sizeof(mens_med_balc)); 
    }


    close(medico_fifo_fd);
    close(balcao_fifo_fd);
    unlink(medico_FIFO_fnome);

    return 0;
}


