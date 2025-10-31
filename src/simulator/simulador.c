#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCS 10
#define MAX_FRAMES 100
#define MAX_PAGES 100

typedef struct {
    int pid;
    int tamanho;
    int num_paginas;
    int *frames; // frame ocupado, -1 se não estiver na memória
    int *rbit;   // R-bit para Clock
} Processo;

typedef struct {
    int ocupado;
    int pid;
    int pagina;
} Frame;

Processo processos[MAX_PROCS];
Frame memoria[MAX_FRAMES];

int num_processos, num_frames, tamanho_pagina;
int fifo_index = 0;
int clock_hand = 0;
int total_acessos = 0;
int total_page_faults = 0;

void ler_config(char *arquivo) {
    FILE *fp = fopen(arquivo, "r");
    if (!fp) { printf("Erro ao abrir arquivo de configuração\n"); exit(1); }

    fscanf(fp, "%d", &num_frames);
    fscanf(fp, "%d", &tamanho_pagina);
    fscanf(fp, "%d", &num_processos);

    for (int i = 0; i < num_processos; i++) {
        int pid, tamanho;
        fscanf(fp, "%d %d", &pid, &tamanho);
        processos[i].pid = pid;
        processos[i].tamanho = tamanho;
        processos[i].num_paginas = (tamanho + tamanho_pagina - 1)/tamanho_pagina;
        processos[i].frames = malloc(sizeof(int) * processos[i].num_paginas);
        processos[i].rbit = malloc(sizeof(int) * processos[i].num_paginas);
        for(int j=0;j<processos[i].num_paginas;j++){
            processos[i].frames[j] = -1;
            processos[i].rbit[j] = 0;
        }
    }

    for(int i=0;i<num_frames;i++){
        memoria[i].ocupado = 0;
        memoria[i].pid = -1;
        memoria[i].pagina = -1;
    }

    fclose(fp);
}

int procurar_frame_livre() {
    for(int i=0;i<num_frames;i++){
        if(!memoria[i].ocupado) return i;
    }
    return -1;
}

int fifo_substituir() {
    int f = fifo_index;
    fifo_index = (fifo_index+1)%num_frames;
    return f;
}

int clock_substituir() {
    while(1){
        if(memoria[clock_hand].pid == -1) { int f = clock_hand; clock_hand = (clock_hand+1)%num_frames; return f; }
        Processo *p = NULL;
        for(int i=0;i<num_processos;i++){
            if(processos[i].pid == memoria[clock_hand].pid){p=&processos[i];break;}
        }
        int pg = memoria[clock_hand].pagina;
        if(p->rbit[pg]==0){
            int f = clock_hand;
            clock_hand = (clock_hand+1)%num_frames;
            return f;
        } else {
            p->rbit[pg]=0;
            clock_hand = (clock_hand+1)%num_frames;
        }
    }
}

void acessar_memoria(char *algoritmo, int pid, int endereco) {
    total_acessos++;
    Processo *p = NULL;
    for(int i=0;i<num_processos;i++){
        if(processos[i].pid==pid){p=&processos[i]; break;}
    }
    if(!p){ printf("Endereco inválido: PID %d Endereco %d\n", pid, endereco); return; }

    int pagina = endereco/tamanho_pagina;
    int deslocamento = endereco % tamanho_pagina;

    if(pagina>=p->num_paginas){ 
        printf("Endereco inválido: PID %d Endereco %d\n", pid, endereco);
        return;
    }

    if(p->frames[pagina]!=-1){
        // HIT
        p->rbit[pagina]=1;
        printf("Acesso: PID %d, Endereço %d (Página %d, Deslocamento %d) -> HIT: Página %d (PID %d) já está no Frame %d\n",
               pid,endereco,pagina,deslocamento,pagina,pid,p->frames[pagina]);
        return;
    }

    // PAGE FAULT
    total_page_faults++;
    int f = procurar_frame_livre();
    if(f==-1){
        // Substituição
        if(strcmp(algoritmo,"fifo")==0) f = fifo_substituir();
        else f = clock_substituir();

        // desaloca antiga
        int pid_ant = memoria[f].pid;
        int pg_ant = memoria[f].pagina;
        for(int i=0;i<num_processos;i++){
            if(processos[i].pid==pid_ant){
                processos[i].frames[pg_ant]=-1;
                break;
            }
        }
        printf("Acesso: PID %d, Endereço %d (Página %d, Deslocamento %d) -> PAGE FAULT -> Memória cheia. Página %d (PID %d) (Frame %d) será desalocada. -> Página %d (PID %d) alocada no Frame %d\n",
               pid,endereco,pagina,deslocamento,pg_ant,pid_ant,f,pagina,pid,f);
    } else {
        printf("Acesso: PID %d, Endereço %d (Página %d, Deslocamento %d) -> PAGE FAULT -> Página %d (PID %d) alocada no Frame livre %d\n",
               pid,endereco,pagina,deslocamento,pagina,pid,f);
    }

    // aloca nova página
    memoria[f].ocupado=1;
    memoria[f].pid=pid;
    memoria[f].pagina=pagina;
    p->frames[pagina]=f;
    p->rbit[pagina]=1;
}

void simular(char *algoritmo, char *arquivo_acessos){
    FILE *fp = fopen(arquivo_acessos,"r");
    if(!fp){ printf("Erro ao abrir arquivo de acessos\n"); exit(1); }

    int pid,endereco;
    while(fscanf(fp,"%d %d",&pid,&endereco)!=EOF){
        acessar_memoria(algoritmo,pid,endereco);
    }

    printf("--- Simulação Finalizada (Algoritmo: %s)\n", algoritmo);
    printf("Total de Acessos: %d\n", total_acessos);
    printf("Total de Page Faults: %d\n", total_page_faults);

    fclose(fp);
}

int main(int argc, char *argv[]){
    if(argc!=4){
        printf("Uso: %s <fifo|clock> <arquivo_config> <arquivo_acessos>\n", argv[0]);
        return 1;
    }

    char *algoritmo = argv[1];
    char *arquivo_config = argv[2];
    char *arquivo_acessos = argv[3];

    ler_config(arquivo_config);
    simular(algoritmo,arquivo_acessos);

    return 0;
}
