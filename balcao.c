#include "balcao.h"


int balcao_fifo_fd; //identificador do FIFO do balcao
int cliente_fifo_fd; //identificador do FIFO do cliente
int medico_fifo_fd; //identificador do FIFO do medico



void *mostraListaXsec(void *balcaoo){
    Balcao *balcao = (Balcao *) balcaoo;  

    while(1){
        //Inicio de uma parte critica
        pthread_mutex_lock(&balcao->mutex);
        printf("\n[INFO] Tempo de frequência: %d\n", balcao->tempoFila);
        mostraListasEspera(balcao);
        pthread_mutex_unlock(&balcao->mutex);
        sleep(balcao->tempoFila);
        //Fim de uma parte critica    
    }   
}



typedef struct{
    Balcao *balcao;
    pMedico listaMedicos;
    pthread_mutex_t mutex;
}Dados;



void *verificaSinaisVida(void *dadoss){

    Dados *dados = (Dados *) dadoss;  

    while(1){
        //Inicio de uma parte critica
        pthread_mutex_lock(&dados->mutex);

            for (int i = 0; i < dados->balcao->maxMedicos; i++)  {
                if(dados->listaMedicos[i].id!=0){
                    if(dados->listaMedicos[i].isAlive==0){
                        dados->listaMedicos[i].isAlive=tempoAtual();    
                    }else if(tempoAtual() - dados->listaMedicos[i].isAlive > 20500){
                        fprintf(stderr, "[INFO] Médico [%d] deixou de enviar sinais de vida\n",  dados->listaMedicos[i].id);
                        removerMedico(dados->balcao, dados->listaMedicos, dados->listaMedicos[i].id);
                    } 
                }
             }    
        pthread_mutex_unlock(&dados->mutex);
        //Fim de uma parte critica    
    }   
}




void encerra() {

    printf("\nA encerrar o Balcão...\n");

    for (int i = 3; i > 0; i--) {
        printf("%d\n", i);
        sleep(1);
    }
}

void trataSig(int i) {
    (void) i;
    fprintf(stderr, "\n [INFO] Balcão interrompido via teclado!\n\n");



    //close(s_fifo_fd);
    unlink(BALCAO_FIFO);
    exit(EXIT_SUCCESS);
}


const char *classifica(int pid,int b2c[2] , int c2b[2], char sintomas[]) {

    int estado;
    size_t tam, tam1;

    //Criacao dos pipes anonimos
   // int b2c[2], c2b[2];
    static char especialidade_Prioridade[MAX];


    if (pid > 0) { //Pai

        close(b2c[STDIN_FILENO]); //fecha canal de leitura
        close(c2b[STDOUT_FILENO]); //fecha canal de escrita

        tam = write(b2c[STDOUT_FILENO], sintomas, strlen(sintomas));
        tam1 = read(c2b[STDIN_FILENO], especialidade_Prioridade, sizeof(especialidade_Prioridade) - 1);
        especialidade_Prioridade[tam1] = '\0';


    }

    return especialidade_Prioridade;
}


void help() {
    printf("\nComandos disponíveis:\n\n");
    printf("-> utentes\n");
    printf("-> especilalistas\n");
    printf("-> delut x\n");
    printf("-> delesp x\n");
    printf("-> freq N\n");
    printf("-> encerra\n");
    printf("-> help\n\n");
}

void comandos(Balcao *balcao, pCliente listaUtentes, pMedico listaMedicos) {
    char strcomando[50];
    fgets(strcomando, sizeof(strcomando), stdin);// le o \n
    // printf("O administrador introduziu o comando: %s", comando);
    char *comando= strtok(strcomando," ");

    if (strcmp(comando, "utentes\n") == 0) {

        printf("\n---------- Lista de Utentes ----------\n");
        mostrarTodosClientes(balcao, listaUtentes);
        printf("\n--------------------------------------\n");

    } else if (strcmp(comando, "especialistas\n") == 0) {

        printf("\n---------- Lista de Médicos ----------\n");
        mostrarTodosMedicos(balcao, listaMedicos);
        printf("\n------------------------------------\n");

    } else if (strcmp(comando, "delut") == 0) {
        comando = strtok(NULL,"\n");
        int id = atoi(comando);

        if(validaDelut(balcao, listaUtentes,id)==1){
            removerCliente(balcao, listaUtentes,id);
            removeFromLista(balcao,id);
            organizaLista(balcao,listaUtentes);
        }

    } else if (strcmp(comando, "delesp") == 0) {
        comando = strtok(NULL,"\n");
        int id = atoi(comando);
        if(validaDelesp(balcao, listaMedicos,id)==1){
            removerMedico(balcao, listaMedicos,id);
        }
    } else if (strcmp(comando, "freq") == 0) {
        comando = strtok(NULL,"\n");
        int num = atoi(comando);
        balcao->tempoFila = num;
    
    } else if (strcmp(comando, "help\n") == 0) {
        help();
    } else if (strcmp(comando, "encerra\n") == 0) {
        encerra();
        terminaClientesMedicos(balcao,listaUtentes, listaMedicos);
        unlink(BALCAO_FIFO);
        exit(EXIT_SUCCESS);
        exit(1);


    } else {
        printf("[INFO] Comando Invalido!\n\n");
    }


}

int validaDelut(Balcao *balcao, pCliente listaUtentes,int id){
    for(int i = 0; i < balcao->maxClientes; i++){
        if(id==listaUtentes[i].id){
            if(listaUtentes[i].atendido == 0){

                return 1;
            }
        }
    }
    return 0;
}
int validaDelesp(Balcao *balcao, pMedico listaMedicos,int id){
    for(int i = 0; i < balcao->maxMedicos; i++){
        if(id==listaMedicos[i].id){
            if(listaMedicos[i].estado == 0){
                return 1;
            }
        }
    }
    return 0;
}

Balcao inicializarDadosBalcao(int MAXMEDICOS, int MAXCLIENTES) {

    Balcao b;

    b.maxMedicos = MAXMEDICOS;
    b.maxClientes = MAXCLIENTES;
    for (int i = 0; i < MAXESPECIALIDADES; i++) {
        for (int f = 0; f < MAXESPECIALIDADES; f++) {
            b.filaEspera[i][f] = 0;
        }
        b.numEspecialistas[i] = 0;
    }
    b.numClientes = 0;
    b.numMedicos = 0;
    b.tempoFila = 30;

    pthread_mutex_t mutex;
    if (pthread_mutex_init( &mutex, NULL) != 0 )
        printf( "Erro na inicialização do Mutex\n" );

    b.mutex = mutex; //partilhado

    return b;
}

void adicionarMedico(Balcao *b, pMedico listaMedicos, int id, Medico medico) {
    if (b->numMedicos == b->maxMedicos) {
        fprintf(stderr, "[INFO] Lotação máxima de médicos atingida!\n");
        return;
    }

    listaMedicos[id] = medico;
    b->numMedicos++;
    fprintf(stdout, "[INFO] Medico '%s' adicionado ao sistema!\n", medico.nome);
}

void removerMedico(Balcao *b, pMedico medico, int id) {
    if (b->numMedicos > 0) {
        b->numMedicos--;
        for (int i = 0; i < b->maxMedicos; i++) {
            if (medico[i].id == id) {
                fprintf(stdout, "[INFO] Médico '%s' removido do sistema!\n", medico[i].nome);
                medico[i] = inicializarMedico();
                break;
            }
        }    
    }
}

void mostrarTodosMedicos(Balcao *b, pMedico medico) {

    if (b->numMedicos > 0) {

        for (int i = 0; i < b->maxMedicos; i++)
            if (medico[i].id != 0)
                mostrarDadosMedico(i, medico[i]);
    } else
        printf("\n Não existem médicos no sistema!\n");
}

void mostrarDadosMedico(int id, Medico medico) {
    printf("\nMédico [%d]:\n\tNome: %s\n\tPID: %d\n\tEspecialidade: %s\n\tEstado: %d\n\tCliente atribuído: %d\n", id, medico.nome,medico.id, medico.especialidade,
        medico.estado, medico.clienteAtender);
}

void adicionarCliente(Balcao *b, pCliente listaUtentes, int id, Cliente cliente) {

    if (b->numClientes == b->maxClientes) {
        fprintf(stderr, "\n[INFO] LOTAÇÃO MÁXIMA DE CLIENTES ATINGIDA\n");
        return;
    }

    listaUtentes[id] = cliente;
    b->numClientes++;
    fprintf(stdout, "\n[INFO] Cliente '%s' adicionado ao sistema!\n", cliente.nome);
}

void removerCliente(Balcao *b, pCliente utente, int id) {

    if (b->numClientes > 0) {
        b->numClientes--;


        for (int i = 0; i < b->maxClientes; i++) {
            if (utente[i].id == id) {
                fprintf(stdout, "\n[INFO] Cliente '%s' removido do sistema!\n", utente[i].nome);
                utente[i] = inicializarCliente();
                break;
            }
        }
    }
}

int vericaExisteCliente(Balcao b, pCliente listaUtentes, int pid) {


    for (int i = 0; i < b.maxClientes; i++) {
        if (listaUtentes[i].id == pid)
            return 1;
    }

    return 0;
}

int verificaExisteMedico(Balcao b, pMedico listaMedicos, int pid) {
  
     for (int i = 0; i < b.maxMedicos; i++) {
        if (listaMedicos[i].id == pid){
            listaMedicos[i].estado = 0;
            listaMedicos[i].clienteAtender = 0;
            return 1;
        }     
    }

    return 0;
}

int vericaClienteTemEspecialidade(Balcao b, pCliente listaUtentes, int pid) {

    int i;
    for (i = 0; i < b.maxClientes; i++) {
        if (listaUtentes[i].id == pid)
            if (strcmp("", listaUtentes[i].areaEspecialidade) == 0)
                return 1;
    }

    return 0;
}

Cliente inicializarCliente() {

    Cliente c;

    c.id = 0;
    strcpy(c.nome, "");
    strcpy(c.areaEspecialidade, "");
    c.posicaoListaEspera = 0;
    c.prioridade = 0;
    strcpy(c.sintomas, "");

    return c;
}

Medico inicializarMedico() {

    Medico m;
    m.id = 0;
    strcpy(m.nome, "");
    strcpy(m.especialidade, "");
    m.estado = 0;
    m.isAlive=0;

    return m;
}


int verificaNumEspecialistas(Balcao *balcao, char *especialidade){

    //criar Funcao para verificar
    if (strcmp("oftalmologia", especialidade) == 0) {
        if (balcao->numMedicos == balcao->maxMedicos) {
            return 0;      //Maximo atingido
        }
        balcao->numEspecialistas[oftalmologia]++;
        return 1;
    } else if (strcmp("neurologia", especialidade) == 0) {
          if (balcao->numMedicos == balcao->maxMedicos) {
            return 0;
        }
        balcao->numEspecialistas[neurologia]++;
        return 1;
    } else if (strcmp("estomatologia", especialidade) == 0){
       if (balcao->numMedicos == balcao->maxMedicos) {
            return 0;
        }
        balcao->numEspecialistas[estomatologia]++;
        return 1;
    } else if (strcmp("ortopedia", especialidade) == 0) {
        if (balcao->numMedicos == balcao->maxMedicos) {
            return 0;
        }
        balcao->numEspecialistas[ortopedia]++;
        return 1;
    }else if (strcmp("geral", especialidade) == 0) {
        if (balcao->numMedicos == balcao->maxMedicos) {
            return 0;
        }
      balcao->numEspecialistas[geral]++;
        return 1;
    }
    //Especialidade inserida nao existe no sistema
    if (balcao->numMedicos == balcao->maxMedicos) {
            return 0;      //Maximo atingido
    }else
         return 2;

}

void mostrarDadosCliente(int id, Cliente utente) {
    printf("\nCliente [%d]:\n\tNome: %s\n\tPID: %d\n\tSintomas: %s\tEspecialidade: %s\n\tPrioridade: %d\n\tPosicaoEspera: %d\n\tEstado: %d\n\tMédico atribuído: %d\n",
           id, utente.nome, utente.id, utente.sintomas, utente.areaEspecialidade, utente.prioridade, utente.posicaoListaEspera,utente.atendido, utente.medicoAtribuido);
}

void mostrarTodosClientes(Balcao *b, pCliente utente) {

    if (b->numClientes > 0) {
        for (int i = 0; i < b->maxClientes; i++) {
            if (utente[i].id != 0)
                mostrarDadosCliente(i, utente[i]);
        }
    } else
        printf("\n Não existem clientes no sistema!\n");
}




int medico_correto(Balcao b,pMedico listaMedicos,char* especialidade){
    int i;

    for(i=0; i < b.maxMedicos;i++){
        if (strcmp(listaMedicos[i].especialidade,especialidade) && listaMedicos[i].estado==0){
            //printf("\n\n\n\n\n\n\n\n%d\n\n\n\n\n\n\n",listaMedicos[i].id);
            return listaMedicos[i].id;
        }
    }

    return -1;
}
void AtribuiLista(Balcao *b, Cliente cli){
    //*[0]oftalmonologia
    //*[1]neurologia
    //*[2]estomatologia
    //*[3]ortopedia
    //*[4]geral
    int fila;
    if(strcmp(cli.areaEspecialidade,"oftalmologia") == 0){
        fila = 0;
    }else if(strcmp(cli.areaEspecialidade,"neurologia")==0){
        fila = 1;
    }else if(strcmp(cli.areaEspecialidade,"estomatologia") == 0){
        fila = 2;
    }else if(strcmp(cli.areaEspecialidade,"ortopedia") == 0){
        fila = 3;
    }else if(strcmp(cli.areaEspecialidade,"geral") == 0){
        fila = 4;
    }
    int i=0;
    for(i;i<5;i++){
            if(b->filaEspera[fila][i]==0){
                b->filaEspera[fila][i]=cli.id;

                break;
            }
    }
}

void mostraListasEspera(Balcao *b){

    char especialidade[MAX];
     printf("\n");
    for(int i = 0; i < MAXESPECIALIDADES; i++){
            if(i == 0)
              printf("ListaEspera [Oftalmologia]  - ");
            else  if(i == 1)
              printf("ListaEspera [Neurologia]    - ");
            else  if(i == 2)
              printf("ListaEspera [estomatologia] - ");
            else  if(i == 3)
              printf("ListaEspera [ortopedia]     - ");
            else      
              printf("ListaEspera [geral]         - ");
             
        for(int j = 0; j < MAXESPECIALIDADES; j++){
            printf("| %4d ",b->filaEspera[i][j]);
        }
        printf("|\n");
        
    }
   printf("\n");

}

void  organizaLista(Balcao *b, pCliente cli){

    int auxID = -1;
    int auxP = -1;
    for(int g = 0; g < 4;g++){
        for(int i = 0; i < MAXESPECIALIDADES; i++){
            for(int j = 0; j < MAXESPECIALIDADES-1; j++){
                auxID = -1;
                auxP = -1;
                int l;
                for(l = 0; l < b->maxClientes; l++){
                        if(b->filaEspera[i][j] == cli[l].id){
                            if(cli[l].id != 0 && cli[l].atendido==0){
                                auxID = b->filaEspera[i][j];
                                auxP = cli[l].prioridade;
                                cli[l].posicaoListaEspera = j;
                                break;
                            }else{
                                if(b->filaEspera[i][j+1]!=0){
                                    auxID = b->filaEspera[i][j];
                                    b->filaEspera[i][j]=b->filaEspera[i][j+1];
                                    b->filaEspera[i][j+1]=auxID;
                                    cli[l].posicaoListaEspera = j;
                                    break;
                                }
                            } 
                        }
                }
                for(int k = 0; k < b->maxClientes; k++){
                    if(cli[k].id!=0){
                        if(b->filaEspera[i][j+1]==cli[k].id && cli[k].atendido==0){
                            if(auxP > cli[k].prioridade){
                                //printf("p1d: %d, pr1or1dade: %d\n",b->filaEspera[i][j+1],cli[k].prioridade);
                                b->filaEspera[i][j]=b->filaEspera[i][j+1];
                                cli[k].posicaoListaEspera = j;
                                cli[l].posicaoListaEspera = j+1;
                                b->filaEspera[i][j+1]=auxID;
                                break;
                            }
                        } 
                    }
                }
            }
        }
    }
     

}

void removeFromLista(Balcao *b,pid_t pid){
    for(int i = 0; i < MAXESPECIALIDADES; i++){
        for(int j = 0; j < MAXESPECIALIDADES; j++){
            if(b->filaEspera[i][j]==pid){
                b->filaEspera[i][j]=0;
            }
        }
    }

}

int verificaNumClientesEspecialidade(pBalcao balcao, char *especialidade){

int cont = 0;

    if(strcmp(especialidade,"oftalmologia") == 0){ 
      for (int i = 0; i < 5; i++) {
         if(balcao->filaEspera[oftalmologia][i] != 0)
            cont++;
      }
    }else if(strcmp(especialidade,"neurologia")==0){
       for (int i = 0; i < 5; i++) {
         if(balcao->filaEspera[neurologia][i] != 0)
            cont++;
      } 
    }else if(strcmp(especialidade,"estomatologia") == 0){
        for (int i = 0; i < 5; i++) {
         if(balcao->filaEspera[estomatologia][i] != 0)
            cont++;
      }       
    }else if(strcmp(especialidade,"ortopedia") == 0){
         for (int i = 0; i < 5; i++) {
         if(balcao->filaEspera[ortopedia][i] != 0)
            cont++;
      }     
    }else if(strcmp(especialidade,"geral") == 0){
         for (int i = 0; i < 5; i++) {
         if(balcao->filaEspera[geral][i] != 0)
            cont++;
      }   
    }

    if(cont == 5){
        return 1;
    }

     return 0;
}



pid_t devolvePrimeiraPosicaoFila(char *especialidade, Balcao balcao){

int fila;

         if(strcmp(especialidade,"oftalmologia") == 0){
            fila = 0;
         }else if(strcmp(especialidade,"neurologia")==0){
             fila = 1;
         }else if(strcmp(especialidade,"estomatologia") == 0){
              fila = 2;
        }else if(strcmp(especialidade,"ortopedia") == 0){
             fila = 3;
        }else if(strcmp(especialidade,"geral") == 0){
             fila = 4;
        }



      return balcao.filaEspera[fila][0];

}
int devolveFila(char *especialidade){

int fila;

         if(strcmp(especialidade,"oftalmologia") == 0){
            fila = 0;
         }else if(strcmp(especialidade,"neurologia")==0){
             fila = 1;
         }else if(strcmp(especialidade,"estomatologia") == 0){
              fila = 2;
        }else if(strcmp(especialidade,"ortopedia") == 0){
             fila = 3;
        }else if(strcmp(especialidade,"geral") == 0){
             fila = 4;
        }

      return fila;

}


int devolvePosCliente(Balcao balcao,pCliente listaUtentes, int pid){

    int i;
        for (i = 0; i < balcao.maxClientes; i++){
          if(listaUtentes[i].id == pid)
                break;
        }


    return i;
}


void terminaClientesMedicos(Balcao *balcao, pCliente listaUtentes, pMedico listaMedicos){

        
    for (int i = 0; i < balcao->maxClientes; i++){
         if (listaUtentes[i].id != 0) 
             kill (listaUtentes[i].id, SIGHUP);
    }   

    for (int i = 0; i < balcao->maxMedicos; i++){
         if (listaMedicos[i].id != 0) 
             kill (listaMedicos[i].id, SIGHUP);
    }   


}



long long tempoAtual() {
    struct timeval tempoAtual; 
    gettimeofday(&tempoAtual, NULL); 
    long long tempo = tempoAtual.tv_sec*1000LL + tempoAtual.tv_usec/1000; //  tempo em millisegundos
    
    return tempo;
}




//*---------------------MAIN-------------------------------*//

int main(int argc, char *argv[]) {


    int res;
    int maxCLientes, maxMedicos;
    Balcao balcao;
    Cliente utente;
    Medico medico;
    char cliente_FIFO_nome[30]; //nome do FIFO do cliente
    char medico_FIFO_nome[30]; //nome do FIFO do cliente



    int nfd; //valor retorno do select
    fd_set read_fds;



    //Receber as variáveis de ambiente
    if (getenv("MAXCLIENTES") != NULL && getenv("MAXMEDICOS") != NULL) {

        //Receber as variáveis de ambiente
        sscanf(getenv("MAXCLIENTES"), "%d", &maxCLientes);
        sscanf(getenv("MAXMEDICOS"), "%d", &maxMedicos);

        printf("Variáveis de ambiente definidas:\n");
        printf("MAXCLIENTES: %d\n", maxCLientes);
        printf("MAXMEDICOS: %d\n", maxMedicos);

    } else {
        fprintf(stderr, "As variáveis de ambiente não foram definidas!\n");
        exit(1);
    }


    //Criação dos arrays que vão guardar os clientes/médicos
    Cliente listaUtentes[maxCLientes];
    Medico listaMedicos[maxMedicos];

    //Inicializar os dados do balcão/clientes
    balcao = inicializarDadosBalcao(maxMedicos, maxCLientes);


    for (int i = 0; i < balcao.maxClientes; i++)
        listaUtentes[i] = inicializarCliente();

    for (int i = 0; i < balcao.maxMedicos; i++)
        listaMedicos[i] = inicializarMedico(balcao);

    //Estruturas das mensagens
    Mensagem_utilizador_para_Balcao mens_para_balc;
    Mensagem_Balcao mens_Out;


    mens_Out.pid_balcao = getpid();


    //Verificar se ja existe algum balcao em funcionamento
    if (access(BALCAO_FIFO, F_OK) == 0) {
        printf("[INFO] Já existe um Balcão em funcionamento!\n");
        exit(2);
    }

    //Criar pipe do balcao
    mkfifo(BALCAO_FIFO, 0600);
    printf("[INFO] Criei o FIFO do Balcão...\n");

    //Verificar a abertura do fifo
    balcao_fifo_fd = open(BALCAO_FIFO, O_RDWR);
    if (balcao_fifo_fd == -1) {
        perror("\nErro ao abrir o FIFO do servidor (RDWR/blocking)");
        exit(EXIT_FAILURE);
    } else {
      //  fprintf(stderr, "\nFIFO aberto para READ (+WRITE) bloqueante\n");
    }


    //Para interromper via CTRL-C
    if (signal(SIGINT, trataSig) == SIG_ERR) {
        perror("\nNão foi possível configurar o sinal SIGINT\n");
        exit(EXIT_FAILURE);
    }


   pthread_t thread;
    
   if(pthread_create(&thread,NULL,&mostraListaXsec,&balcao) != 0){
          perror("\nNão foi possível criar a Thread\n");
        exit(EXIT_FAILURE);
    }



    pthread_t thread2;
    Dados dados;
    
    pthread_mutex_t mutex2;
     
    if (pthread_mutex_init( &mutex2, NULL) != 0 )
        printf( "mutex init failed\n" );

    dados.balcao = &balcao;
    dados.listaMedicos = listaMedicos;
    dados.mutex= mutex2; 

    if(pthread_create(&thread2,NULL,&verificaSinaisVida,&dados) != 0){
        printf("Erro!");
        exit(-1);
    }


   //Criacao dos pipes anonimos
    int b2c[2], c2b[2];     
    pipe(b2c);
    pipe(c2b);
    //Criacao de processo filho -> Redirecionamento Balcao <-> Classificador
     pid_t pidFork = fork();

    if (pidFork < 0) {
        perror("Erro ao criar o filho!\n");
        exit(1);
    }

   // printf("PID FORK %d\n", pidFork);

 if (pidFork == 0) { //Filho

        close(STDIN_FILENO);   //libertar stdin
        dup(b2c[STDIN_FILENO]); //duplica extremidade de leitura para o lugar libertado
        close(b2c[STDIN_FILENO]); //fecha extremidade de leitura pk ja foi duplicada
        close(b2c[STDOUT_FILENO]); //fecha extremidade de escrita
        close(STDOUT_FILENO);  //libertar stdout
        dup(c2b[STDOUT_FILENO]); //duplica extremidade de escrita para o lugar libertado
        close(c2b[STDOUT_FILENO]); //fecha extremidade de escrita pk ja foi duplicada
        close(c2b[STDIN_FILENO]); //fecha extremidade de leitura

        execl("classificador","classificador",NULL);
    }


    while (1) {


        FD_ZERO(&read_fds);
        FD_SET(0, &read_fds);
        FD_SET(balcao_fifo_fd, &read_fds);

        nfd = select(balcao_fifo_fd + 1, &read_fds, NULL, NULL, NULL);

        if (nfd == 0) {
            //  printf("\n[INFO] À espera...\n");          
            fflush(stdout);
            continue;
        }

        if (nfd == -1) {
            perror("\n[INFO] Erro na criação do select!\n");
            close(balcao_fifo_fd);
            unlink(BALCAO_FIFO);
            exit(1);
        }


        if (FD_ISSET(0, &read_fds)) {
            comandos(&balcao, listaUtentes, listaMedicos); //Ler comandos do teclado
        }

        if (FD_ISSET(balcao_fifo_fd, &read_fds)) {

            res = read(balcao_fifo_fd, &mens_para_balc, sizeof(mens_para_balc));

            if (res < (int) sizeof(mens_para_balc)) {
                fprintf(stderr, "[INFO] Mensagem recebida incompleta! [Bytes lidos: %d]\n", res);

            }      

            //CLIENTE ao terminar o seu programa
            if(mens_para_balc.medico_cliente == 1 && mens_para_balc.atendido == -1){
                   fprintf(stderr, "[INFO] O Utente com Pid [%d] terminou o seu programa!\n", mens_para_balc.pid);

                    for (int i = 0; i < balcao.maxClientes; i++)  {     
                       if(listaUtentes[i].id == mens_para_balc.pid)
                            listaUtentes[i].atendido = 0;
                            listaUtentes[i].medicoAtribuido = 0;
                            break;
                    }    
                   removerCliente(&balcao,listaUtentes,mens_para_balc.pid);
                   removeFromLista(&balcao,mens_para_balc.pid);
                   organizaLista(&balcao,listaUtentes);
            }

            //CLIENTE ao entrar
            if (mens_para_balc.medico_cliente == 1 && mens_para_balc.atendido != -1) {
                if (vericaExisteCliente(balcao, listaUtentes, mens_para_balc.pid) == 0) {
                    char especialidade_prioridade[MAX];

                     //Inicializar Dados do Cliente
                    strcpy(utente.nome, mens_para_balc.nome);
                    utente.id = mens_para_balc.pid;
                    utente.atendido = mens_para_balc.atendido;
    
                    strcpy(utente.sintomas, mens_para_balc.mensagem);
                    strcpy(especialidade_prioridade, classifica(pidFork,b2c,c2b,utente.sintomas));
               
                    //Remover o inteiro da string
                    int i = 0;
                    char especialidade[MAX];
                 
                    while (especialidade_prioridade[i] != ' ') {
                        especialidade[i] = especialidade_prioridade[i];
                        i++;
                    }

                    //Converter a prioridade para um inteiro
                    char priorid = especialidade_prioridade[strlen(especialidade_prioridade)-2];
                    int prioridade = priorid - '0';
                    especialidade[strlen(especialidade_prioridade)-3] = '\0';



                   
                    strcpy(utente.areaEspecialidade, especialidade);

                    utente.prioridade = prioridade;
                    utente.medicoAtribuido = 0;
                    fprintf(stdout, "Classificador:\n");
                    fprintf(stdout, "\tEspecialidade: %s\n", utente.areaEspecialidade);
                    fprintf(stdout, "\tPrioridade: %d\n", utente.prioridade);



                    //Obter filename do FIFO do Cliente para enviar mensagem
                    sprintf(cliente_FIFO_nome, CLIENTE_FIFO, utente.id);
                    //Abrir FIFO do CLIENTE
                    cliente_fifo_fd = open(cliente_FIFO_nome, O_WRONLY);

                    if(verificaNumClientesEspecialidade(&balcao,utente.areaEspecialidade) == 1){
                        //Numero Máximo de clientes Atingido para a especialidade
                        utente.prioridade = -1;
                            if (cliente_fifo_fd == -1)
                                perror("[INFO] Erro na abertura do FIFO do cliente para escrita\n");
                            else {
                                res = write(cliente_fifo_fd, &utente, sizeof(Cliente));
                                if (res == sizeof(Cliente)){
                                    //fprintf(stdout, "[INFO] Mensagem enviada para o cliente com sucesso!\n");
                                }else
                                    fprintf(stderr, "[INFO] Erro ao enviar a mensagem para o cliente!\n");
                           }
                           close(cliente_fifo_fd);
                    }else if(balcao.numClientes >= balcao.maxClientes){  

                        //Numero Máximo de clientes Atingido 
                        utente.prioridade = -2;
                            if (cliente_fifo_fd == -1)
                                perror("[INFO] Erro na abertura do FIFO do cliente para escrita\n");
                            else {
                                res = write(cliente_fifo_fd, &utente, sizeof(Cliente));
                                if (res == sizeof(Cliente)){
                                    //fprintf(stdout, "[INFO] Mensagem enviada para o cliente com sucesso!\n");
                                }else
                                    fprintf(stderr, "[INFO] Erro ao enviar a mensagem para o cliente!\n");
                           }
                           close(cliente_fifo_fd);
                    }else{
                        //Adicionar um novo Cliente 
                       AtribuiLista(&balcao,utente);    
                       adicionarCliente(&balcao, listaUtentes, balcao.numClientes, utente);        
                       organizaLista(&balcao,listaUtentes);
                    }

      
                    //Obter filename do FIFO do Cliente para enviar mensagem
                    sprintf(cliente_FIFO_nome, CLIENTE_FIFO, utente.id);
                    //Abrir FIFO do CLIENTE
                    cliente_fifo_fd = open(cliente_FIFO_nome, O_WRONLY);

                    if (cliente_fifo_fd == -1)
                        perror("[INFO] Erro na abertura do FIFO do cliente para escrita\n");
                    else {
                           for (int i = 0; i < balcao.maxClientes; i++) {
                                if (listaUtentes[i].id == utente.id){
                                   //fprintf(stdout, "Nome > %s  a>>> %d",listaUtentes[i].nome, listaUtentes[i].posicaoListaEspera);
                                   //utente = listaUtentes[i];
                                    res = write(cliente_fifo_fd, &listaUtentes[i], sizeof(Cliente));
                                    if (res == sizeof(Cliente)){
                                        //fprintf(stdout, "[INFO] Mensagem enviada para o cliente com sucesso!\n");
                                    }else
                                        fprintf(stderr, "[INFO] Erro ao enviar a mensagem para o cliente!\n");

                                    break;
                                }
                           }
                        }
                    }
            }


            //SINAL DE VIDA DO MEDICO
            if(mens_para_balc.medico_cliente == 0 && mens_para_balc.atendido == -1){
                   fprintf(stderr, "[INFO] Sinal de vida recebido do Médico [%d]\n", mens_para_balc.pid);
                    for (int i = 0; i < balcao.maxMedicos; i++)  {     
                       if(listaMedicos[i].id == mens_para_balc.pid){
                                listaMedicos[i].isAlive = tempoAtual();
                                break;                           
                       } 
                 }    
            }

            //MEDICO ao entrar
            if (mens_para_balc.medico_cliente == 0  && mens_para_balc.atendido != -1) {
                if (verificaExisteMedico(balcao, listaMedicos, mens_para_balc.pid) == 0) {
                    //Inicializar Dados do Médico
                    strcpy(medico.nome, mens_para_balc.nome);
                    medico.id = mens_para_balc.pid;
                    medico.estado = mens_para_balc.atendido;
                    medico.clienteAtender = 0;
                    strcpy(medico.especialidade, mens_para_balc.mensagem);

                    //Verificar o numero de especialistas para a sua especialidade
                    int valida = verificaNumEspecialistas(&balcao, medico.especialidade);
                    if(valida == 1){
                        printf("[INFO] Novo especialista de '%s'!\n", medico.especialidade);
                        adicionarMedico(&balcao, listaMedicos, balcao.numMedicos, medico);
                        strcpy(mens_Out.mensagem, "[INFO] Foi adicionado ao sistema!\n");
                    }else if (valida == 0){
                        printf("[INFO] Limite máximo de Médicos atingido!\n");
                        strcpy(mens_Out.mensagem, "[INFO] Limite máximo de Médicos atingido!\n");
                    }else {
                        printf("[INFO] Especialidade '%s' não existente no sistema!\n", medico.especialidade);
                        adicionarMedico(&balcao, listaMedicos, balcao.numMedicos, medico);
                        strcpy(mens_Out.mensagem, "[INFO] Especialidade não existente no sistema!\n");
                    }

                    //Obter filename do FIFO do Medico para enviar mensagem
                    sprintf(medico_FIFO_nome, MEDICO_FIFO, medico.id);
                    //Abrir FIFO do Medico
                    medico_fifo_fd = open(medico_FIFO_nome, O_WRONLY);

                    if (medico_fifo_fd == -1)
                        printf("[INFO] Erro ao abrir FIFO do Medico [%d] para escrita!\n", medico.id);
                    else {
                        res = write(medico_fifo_fd, &mens_Out, sizeof(mens_Out));
                        if (res < (int) sizeof(mens_Out)) {
                            fprintf(stderr, "[INFO] Mensagem enviada incompleta! [Bytes enviados: %d]\n", res);
                        }
                        //fprintf(stdout, "[INFO] Mensagem enviada para o Médico [%d] com sucesso! [Bytes enviados: %d]\n",medico.id, res);
                        //close(medico_fifo_fd); // FECHA LOGO O FIFO DO MEDICO!
                        //fprintf(stdout, "\n[INFO] FIFO do MEDICO fechado\n");
                }
             }
            }
        

            //Medico ao sair
            if(mens_para_balc.medico_cliente == 0 && mens_para_balc.atendido == -3){
                   fprintf(stderr, "[INFO] O Médico com Pid [%d] terminou o seu programa!\n", mens_para_balc.pid);
                    for (int i = 0; i < balcao.maxMedicos; i++)  {     
                       if(listaUtentes[i].id == mens_para_balc.pid)
                            removerMedico(&balcao, listaMedicos, listaMedicos[i].id);
                            break;
                    }   
                      
            }

          //Cliente ao sair
            if(mens_para_balc.medico_cliente == 1 && mens_para_balc.atendido == -3){
                   fprintf(stderr, "[INFO] O Utentes com Pid [%d] terminou o seu programa!\n", mens_para_balc.pid);
                   
                    for (int i = 0; i < balcao.maxClientes; i++)  {     
                       if(listaUtentes[i].id == mens_para_balc.pid)
                            listaUtentes[i].atendido = 0;
                            listaUtentes[i].medicoAtribuido = 0;
                            break;
                    }    
                   removerCliente(&balcao,listaUtentes,mens_para_balc.pid);
                   removeFromLista(&balcao,mens_para_balc.pid);
                   organizaLista(&balcao,listaUtentes);
                      
            }


            //Enviar Medico para o Cliente e Cliente para o Medico
            if(balcao.numMedicos > 0 && balcao.numClientes > 0){
              for(int i = 0; i < balcao.maxMedicos; i++){
                    if(listaMedicos[i].id != 0 && listaMedicos[i].estado == 0){
                            if(balcao.filaEspera[devolveFila(listaMedicos[i].especialidade)][0] != 0){ // Tem utente à espera de ser atendido                          
                               int pidCli = balcao.filaEspera[devolveFila(listaMedicos[i].especialidade)][0];
                                sprintf(cliente_FIFO_nome, CLIENTE_FIFO, pidCli);
                                //printf("\n\nNome do cliente a ser atendido >> %s\n\n", cliente_FIFO_nome);
                                cliente_fifo_fd = open(cliente_FIFO_nome, O_WRONLY);
                                sprintf(medico_FIFO_nome, MEDICO_FIFO, listaMedicos[i].id);
                                medico_fifo_fd = open(medico_FIFO_nome, O_WRONLY);
                                if (cliente_fifo_fd == -1) {
                                  //  perror("[INFO] Erro na abertura do FIFO do cliente para escrita\n");

                               } else {
                                    mens_Out.pid_medico = listaMedicos[i].id;
                                    listaMedicos[i].estado = 1;
                                    listaMedicos[i].clienteAtender = pidCli;
                                    strcpy(mens_Out.mensagem,listaMedicos[i].nome);

                                    res = write(cliente_fifo_fd, &mens_Out, sizeof(mens_Out));
                                     if (res == sizeof(mens_Out)){
                                        //fprintf(stdout, "[INFO] Mensagem enviada para o cliente com sucesso!\n");                    
                                    }else
                                        fprintf(stderr, "[INFO] Erro ao enviar a mensagem para o cliente!\n");
                                //close(cliente_fifo_fd); //* FECHA LOGO O FIFO DO CLIENTE! *// 
                               if (medico_fifo_fd == -1) {
                                    //perror("[INFO] Erro na abertura do FIFO do MEDICO para escrita\n");
                               } else {
                                    mens_Out.pid_cliente = pidCli;
                                    int u = devolvePosCliente(balcao,listaUtentes,pidCli);
                                    strcpy(mens_Out.mensagem,listaUtentes[u].nome);   
                                    listaUtentes[u].atendido = 1;
                                    listaUtentes[u].medicoAtribuido = listaMedicos[i].id;
                                    res = write(medico_fifo_fd, &mens_Out, sizeof(mens_Out));
                                     if (res == sizeof(mens_Out)){
                                       // fprintf(stdout, "[INFO] Mensagem enviada para o medico com sucesso!\n");
                                        removeFromLista(&balcao, pidCli);
                                        organizaLista(&balcao,listaUtentes);
                                 }else
                                    fprintf(stderr, "[INFO] Erro ao enviar a mensagem para o medico!\n");
                        }
                    } 
                }
            }
          }
     }

}

}

 //Fechar canais
close(b2c[STDOUT_FILENO]);
close(c2b[STDIN_FILENO]);
close(balcao_fifo_fd);
unlink(BALCAO_FIFO);
return 0;


}





