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

/*Resultado: Simulador de Gerenciamento de Memória com Algoritmo FIFO
O Windows PowerShell
Copyright (C) Microsoft Corporation. Todos os direitos reservados.

Instale o PowerShell mais recente para obter novos recursos e aprimoramentos! https://aka.ms/PSWindows

PS C:\WINDOWS\system32> cd C:\Users\david\proj2-pagingsim-grupodavid\src\simulator
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> gcc -o simulador.exe simulador.c
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> dir simulador.exe


    Diretório: C:\Users\david\proj2-pagingsim-grupodavid\src\simulator


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a----        02/11/2025     20:35          45946 simulador.exe


PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> .\simulador.exe fifo ../../tests/config_1.txt ../../tests/acessos_1.txt ../../output_fifo_1.txt
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> Get-Content ../../output_fifo_1.txt
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> PAGE FAULT -> Pagina 2 (PID 0) alocada no Frame livre 0
Acesso: PID 1, Endereco 12 (Pagina 1, Deslocamento 2) -> PAGE FAULT -> Pagina 1 (PID 1) alocada no Frame livre 1
Acesso: PID 0, Endereco 28 (Pagina 2, Deslocamento 8) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 1, Endereco 55 (Pagina 5, Deslocamento 5) -> PAGE FAULT -> Pagina 5 (PID 1) alocada no Frame livre 2
Acesso: PID 0, Endereco 42 (Pagina 4, Deslocamento 2) -> PAGE FAULT -> Pagina 4 (PID 0) alocada no Frame livre 3
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 1, Endereco 12 (Pagina 1, Deslocamento 2) -> HIT: Pagina 1 (PID 1) ja esta no Frame 1
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Pagina 0 (PID 0) alocada no Frame livre 4
--- Simulacao Finalizada (Algoritmo: fifo)
Total de Acessos: 8
Total de Page Faults: 5
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> .\simulador.exe fifo ../../tests/config_2.txt ../../tests/acessos_2.txt ../../output_fifo_2.txt
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> Get-Content ../../output_fifo_2.txt
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Pagina 0 (PID 0) alocada no Frame livre 0
Acesso: PID 1, Endereco 8 (Pagina 0, Deslocamento 8) -> PAGE FAULT -> Pagina 0 (PID 1) alocada no Frame livre 1
Acesso: PID 2, Endereco 12 (Pagina 1, Deslocamento 2) -> PAGE FAULT -> Pagina 1 (PID 2) alocada no Frame livre 2
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> PAGE FAULT -> Pagina 1 (PID 0) alocada no Frame livre 3
Acesso: PID 1, Endereco 20 (Pagina 2, Deslocamento 0) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 0) (Frame 0) sera desalocada. -> Pagina 2 (PID 1) alocada no Frame 0
Acesso: PID 2, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 1) (Frame 1) sera desalocada. -> Pagina 0 (PID 2) alocada no Frame 1
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 2) (Frame 2) sera desalocada. -> Pagina 2 (PID 0) alocada no Frame 2
Acesso: PID 1, Endereco 10 (Pagina 1, Deslocamento 0) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 0) (Frame 3) sera desalocada. -> Pagina 1 (PID 1) alocada no Frame 3
Acesso: PID 2, Endereco 18 (Pagina 1, Deslocamento 8) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 1) (Frame 0) sera desalocada. -> Pagina 1 (PID 2) alocada no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 2) (Frame 1) sera desalocada. -> Pagina 0 (PID 0) alocada no Frame 1
--- Simulacao Finalizada (Algoritmo: fifo)
Total de Acessos: 10
Total de Page Faults: 10
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> .\simulador.exe fifo ../../tests/config_3.txt ../../tests/acessos_3.txt ../../output_fifo_3.txt
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> Get-Content ../../output_fifo_3.txt
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Pagina 0 (PID 0) alocada no Frame livre 0
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> PAGE FAULT -> Pagina 1 (PID 0) alocada no Frame livre 1
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> PAGE FAULT -> Pagina 2 (PID 0) alocada no Frame livre 2
Acesso: PID 0, Endereco 35 (Pagina 3, Deslocamento 5) -> PAGE FAULT -> Pagina 3 (PID 0) alocada no Frame livre 3
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 2
--- Simulacao Finalizada (Algoritmo: fifo)
Total de Acessos: 7
Total de Page Faults: 4
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> .\simulador.exe fifo ../../tests/config_4.txt ../../tests/acessos_4.txt ../../output_fifo_4.txt
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> Get-Content ../../output_fifo_4.txt
Acesso: PID 0, Endereco 3 (Pagina 0, Deslocamento 3) -> PAGE FAULT -> Pagina 0 (PID 0) alocada no Frame livre 0
Acesso: PID 2, Endereco 15 (Pagina 0, Deslocamento 15) -> PAGE FAULT -> Pagina 0 (PID 2) alocada no Frame livre 1
Acesso: PID 1, Endereco 17 (Pagina 0, Deslocamento 17) -> PAGE FAULT -> Pagina 0 (PID 1) alocada no Frame livre 2
Acesso: PID 0, Endereco 69 (Pagina 3, Deslocamento 9) -> PAGE FAULT -> Pagina 3 (PID 0) alocada no Frame livre 3
Acesso: PID 0, Endereco 75 (Pagina 3, Deslocamento 15) -> HIT: Pagina 3 (PID 0) ja esta no Frame 3
Acesso: PID 3, Endereco 4 (Pagina 0, Deslocamento 4) -> PAGE FAULT -> Pagina 0 (PID 3) alocada no Frame livre 4
Acesso: PID 0, Endereco 11 (Pagina 0, Deslocamento 11) -> HIT: Pagina 0 (PID 0) ja esta no Frame 0
Acesso: PID 1, Endereco 29 (Pagina 1, Deslocamento 9) -> PAGE FAULT -> Pagina 1 (PID 1) alocada no Frame livre 5
Acesso: PID 0, Endereco 71 (Pagina 3, Deslocamento 11) -> HIT: Pagina 3 (PID 0) ja esta no Frame 3
Acesso: PID 1, Endereco 91 (Pagina 4, Deslocamento 11) -> PAGE FAULT -> Pagina 4 (PID 1) alocada no Frame livre 6
Acesso: PID 3, Endereco 28 (Pagina 1, Deslocamento 8) -> PAGE FAULT -> Pagina 1 (PID 3) alocada no Frame livre 7
Acesso: PID 3, Endereco 75 (Pagina 3, Deslocamento 15) -> PAGE FAULT -> Pagina 3 (PID 3) alocada no Frame livre 8
Acesso: PID 2, Endereco 51 (Pagina 2, Deslocamento 11) -> PAGE FAULT -> Pagina 2 (PID 2) alocada no Frame livre 9
Acesso: PID 0, Endereco 20 (Pagina 1, Deslocamento 0) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 0) (Frame 0) sera desalocada. -> Pagina 1 (PID 0) alocada no Frame 0
Acesso: PID 3, Endereco 43 (Pagina 2, Deslocamento 3) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 2) (Frame 1) sera desalocada. -> Pagina 2 (PID 3) alocada no Frame 1
Acesso: PID 2, Endereco 9 (Pagina 0, Deslocamento 9) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 1) (Frame 2) sera desalocada. -> Pagina 0 (PID 2) alocada no Frame 2
Acesso: PID 1, Endereco 97 (Pagina 4, Deslocamento 17) -> HIT: Pagina 4 (PID 1) ja esta no Frame 6
Acesso: PID 2, Endereco 6 (Pagina 0, Deslocamento 6) -> HIT: Pagina 0 (PID 2) ja esta no Frame 2
Acesso: PID 0, Endereco 48 (Pagina 2, Deslocamento 8) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 0) (Frame 3) sera desalocada. -> Pagina 2 (PID 0) alocada no Frame 3
Acesso: PID 0, Endereco 45 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 3
Acesso: PID 2, Endereco 48 (Pagina 2, Deslocamento 8) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 2, Endereco 48 (Pagina 2, Deslocamento 8) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 0, Endereco 77 (Pagina 3, Deslocamento 17) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 3) (Frame 4) sera desalocada. -> Pagina 3 (PID 0) alocada no Frame 4
Acesso: PID 0, Endereco 62 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 2, Endereco 48 (Pagina 2, Deslocamento 8) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 0, Endereco 77 (Pagina 3, Deslocamento 17) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 2, Endereco 48 (Pagina 2, Deslocamento 8) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 2, Endereco 59 (Pagina 2, Deslocamento 19) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 0, Endereco 62 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 2, Endereco 59 (Pagina 2, Deslocamento 19) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 0, Endereco 62 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 77 (Pagina 3, Deslocamento 17) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 62 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 62 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 77 (Pagina 3, Deslocamento 17) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 62 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 2, Endereco 48 (Pagina 2, Deslocamento 8) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 2, Endereco 46 (Pagina 2, Deslocamento 6) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 0, Endereco 77 (Pagina 3, Deslocamento 17) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 2, Endereco 46 (Pagina 2, Deslocamento 6) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 0, Endereco 77 (Pagina 3, Deslocamento 17) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 77 (Pagina 3, Deslocamento 17) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 2, Endereco 59 (Pagina 2, Deslocamento 19) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 2, Endereco 59 (Pagina 2, Deslocamento 19) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 0, Endereco 62 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 3, Endereco 92 (Pagina 4, Deslocamento 12) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 1) (Frame 5) sera desalocada. -> Pagina 4 (PID 3) alocada no Frame 5
Acesso: PID 3, Endereco 79 (Pagina 3, Deslocamento 19) -> HIT: Pagina 3 (PID 3) ja esta no Frame 8
Acesso: PID 1, Endereco 41 (Pagina 2, Deslocamento 1) -> PAGE FAULT -> Memoria cheia. Pagina 4 (PID 1) (Frame 6) sera desalocada. -> Pagina 2 (PID 1) alocada no Frame 6
Acesso: PID 3, Endereco 1 (Pagina 0, Deslocamento 1) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 3) (Frame 7) sera desalocada. -> Pagina 0 (PID 3) alocada no Frame 7
Acesso: PID 0, Endereco 25 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 0
Acesso: PID 1, Endereco 10 (Pagina 0, Deslocamento 10) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 3) (Frame 8) sera desalocada. -> Pagina 0 (PID 1) alocada no Frame 8
Acesso: PID 3, Endereco 1 (Pagina 0, Deslocamento 1) -> HIT: Pagina 0 (PID 3) ja esta no Frame 7
Acesso: PID 3, Endereco 88 (Pagina 4, Deslocamento 8) -> HIT: Pagina 4 (PID 3) ja esta no Frame 5
Acesso: PID 3, Endereco 92 (Pagina 4, Deslocamento 12) -> HIT: Pagina 4 (PID 3) ja esta no Frame 5
Acesso: PID 3, Endereco 82 (Pagina 4, Deslocamento 2) -> HIT: Pagina 4 (PID 3) ja esta no Frame 5
Acesso: PID 1, Endereco 72 (Pagina 3, Deslocamento 12) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 2) (Frame 9) sera desalocada. -> Pagina 3 (PID 1) alocada no Frame 9
Acesso: PID 3, Endereco 82 (Pagina 4, Deslocamento 2) -> HIT: Pagina 4 (PID 3) ja esta no Frame 5
Acesso: PID 3, Endereco 1 (Pagina 0, Deslocamento 1) -> HIT: Pagina 0 (PID 3) ja esta no Frame 7
Acesso: PID 3, Endereco 28 (Pagina 1, Deslocamento 8) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 0) (Frame 0) sera desalocada. -> Pagina 1 (PID 3) alocada no Frame 0
Acesso: PID 1, Endereco 10 (Pagina 0, Deslocamento 10) -> HIT: Pagina 0 (PID 1) ja esta no Frame 8
Acesso: PID 1, Endereco 72 (Pagina 3, Deslocamento 12) -> HIT: Pagina 3 (PID 1) ja esta no Frame 9
Acesso: PID 1, Endereco 37 (Pagina 1, Deslocamento 17) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 3) (Frame 1) sera desalocada. -> Pagina 1 (PID 1) alocada no Frame 1
Acesso: PID 3, Endereco 42 (Pagina 2, Deslocamento 2) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 2) (Frame 2) sera desalocada. -> Pagina 2 (PID 3) alocada no Frame 2
Acesso: PID 2, Endereco 58 (Pagina 2, Deslocamento 18) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 0) (Frame 3) sera desalocada. -> Pagina 2 (PID 2) alocada no Frame 3
Acesso: PID 1, Endereco 41 (Pagina 2, Deslocamento 1) -> HIT: Pagina 2 (PID 1) ja esta no Frame 6
Acesso: PID 0, Endereco 25 (Pagina 1, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 0) (Frame 4) sera desalocada. -> Pagina 1 (PID 0) alocada no Frame 4
Acesso: PID 3, Endereco 88 (Pagina 4, Deslocamento 8) -> HIT: Pagina 4 (PID 3) ja esta no Frame 5
Acesso: PID 0, Endereco 25 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 4
Acesso: PID 1, Endereco 72 (Pagina 3, Deslocamento 12) -> HIT: Pagina 3 (PID 1) ja esta no Frame 9
Acesso: PID 3, Endereco 92 (Pagina 4, Deslocamento 12) -> HIT: Pagina 4 (PID 3) ja esta no Frame 5
Acesso: PID 1, Endereco 64 (Pagina 3, Deslocamento 4) -> HIT: Pagina 3 (PID 1) ja esta no Frame 9
Acesso: PID 0, Endereco 38 (Pagina 1, Deslocamento 18) -> HIT: Pagina 1 (PID 0) ja esta no Frame 4
Acesso: PID 1, Endereco 19 (Pagina 0, Deslocamento 19) -> HIT: Pagina 0 (PID 1) ja esta no Frame 8
Acesso: PID 2, Endereco 48 (Pagina 2, Deslocamento 8) -> HIT: Pagina 2 (PID 2) ja esta no Frame 3
Acesso: PID 1, Endereco 69 (Pagina 3, Deslocamento 9) -> HIT: Pagina 3 (PID 1) ja esta no Frame 9
Acesso: PID 0, Endereco 76 (Pagina 3, Deslocamento 16) -> PAGE FAULT -> Memoria cheia. Pagina 4 (PID 3) (Frame 5) sera desalocada. -> Pagina 3 (PID 0) alocada no Frame 5
Acesso: PID 2, Endereco 31 (Pagina 1, Deslocamento 11) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 1) (Frame 6) sera desalocada. -> Pagina 1 (PID 2) alocada no Frame 6
Acesso: PID 0, Endereco 14 (Pagina 0, Deslocamento 14) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 3) (Frame 7) sera desalocada. -> Pagina 0 (PID 0) alocada no Frame 7
Acesso: PID 2, Endereco 56 (Pagina 2, Deslocamento 16) -> HIT: Pagina 2 (PID 2) ja esta no Frame 3
Acesso: PID 2, Endereco 15 (Pagina 0, Deslocamento 15) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 1) (Frame 8) sera desalocada. -> Pagina 0 (PID 2) alocada no Frame 8
Acesso: PID 0, Endereco 30 (Pagina 1, Deslocamento 10) -> HIT: Pagina 1 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 10 (Pagina 0, Deslocamento 10) -> HIT: Pagina 0 (PID 0) ja esta no Frame 7
Acesso: PID 3, Endereco 8 (Pagina 0, Deslocamento 8) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 1) (Frame 9) sera desalocada. -> Pagina 0 (PID 3) alocada no Frame 9
Acesso: PID 1, Endereco 16 (Pagina 0, Deslocamento 16) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 3) (Frame 0) sera desalocada. -> Pagina 0 (PID 1) alocada no Frame 0
Acesso: PID 3, Endereco 70 (Pagina 3, Deslocamento 10) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 1) (Frame 1) sera desalocada. -> Pagina 3 (PID 3) alocada no Frame 1
Acesso: PID 1, Endereco 33 (Pagina 1, Deslocamento 13) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 3) (Frame 2) sera desalocada. -> Pagina 1 (PID 1) alocada no Frame 2
Acesso: PID 3, Endereco 27 (Pagina 1, Deslocamento 7) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 2) (Frame 3) sera desalocada. -> Pagina 1 (PID 3) alocada no Frame 3
Acesso: PID 1, Endereco 91 (Pagina 4, Deslocamento 11) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 0) (Frame 4) sera desalocada. -> Pagina 4 (PID 1) alocada no Frame 4
Acesso: PID 2, Endereco 25 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 2) ja esta no Frame 6
Acesso: PID 2, Endereco 28 (Pagina 1, Deslocamento 8) -> HIT: Pagina 1 (PID 2) ja esta no Frame 6
Acesso: PID 3, Endereco 15 (Pagina 0, Deslocamento 15) -> HIT: Pagina 0 (PID 3) ja esta no Frame 9
Acesso: PID 1, Endereco 28 (Pagina 1, Deslocamento 8) -> HIT: Pagina 1 (PID 1) ja esta no Frame 2
Acesso: PID 0, Endereco 43 (Pagina 2, Deslocamento 3) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 0) (Frame 5) sera desalocada. -> Pagina 2 (PID 0) alocada no Frame 5
Acesso: PID 0, Endereco 75 (Pagina 3, Deslocamento 15) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 2) (Frame 6) sera desalocada. -> Pagina 3 (PID 0) alocada no Frame 6
Acesso: PID 1, Endereco 75 (Pagina 3, Deslocamento 15) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 0) (Frame 7) sera desalocada. -> Pagina 3 (PID 1) alocada no Frame 7
Acesso: PID 1, Endereco 0 (Pagina 0, Deslocamento 0) -> HIT: Pagina 0 (PID 1) ja esta no Frame 0
Acesso: PID 0, Endereco 7 (Pagina 0, Deslocamento 7) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 2) (Frame 8) sera desalocada. -> Pagina 0 (PID 0) alocada no Frame 8
Acesso: PID 1, Endereco 8 (Pagina 0, Deslocamento 8) -> HIT: Pagina 0 (PID 1) ja esta no Frame 0
Acesso: PID 0, Endereco 42 (Pagina 2, Deslocamento 2) -> HIT: Pagina 2 (PID 0) ja esta no Frame 5
Acesso: PID 0, Endereco 65 (Pagina 3, Deslocamento 5) -> HIT: Pagina 3 (PID 0) ja esta no Frame 6
--- Simulacao Finalizada (Algoritmo: fifo)
Total de Acessos: 100
Total de Page Faults: 39
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> .\simulador.exe fifo ../../tests/config_5.txt ../../tests/acessos_5.txt ../../output_fifo_5.txt
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> Get-Content ../../output_fifo_5.txt
Acesso: PID 0, Endereco 34 (Pagina 2, Deslocamento 4) -> PAGE FAULT -> Pagina 2 (PID 0) alocada no Frame livre 0
Acesso: PID 0, Endereco 52 (Pagina 3, Deslocamento 7) -> PAGE FAULT -> Pagina 3 (PID 0) alocada no Frame livre 1
Acesso: PID 2, Endereco 6 (Pagina 0, Deslocamento 6) -> PAGE FAULT -> Pagina 0 (PID 2) alocada no Frame livre 2
Acesso: PID 0, Endereco 48 (Pagina 3, Deslocamento 3) -> HIT: Pagina 3 (PID 0) ja esta no Frame 1
Acesso: PID 2, Endereco 21 (Pagina 1, Deslocamento 6) -> PAGE FAULT -> Pagina 1 (PID 2) alocada no Frame livre 3
Acesso: PID 0, Endereco 20 (Pagina 1, Deslocamento 5) -> PAGE FAULT -> Pagina 1 (PID 0) alocada no Frame livre 4
Acesso: PID 1, Endereco 21 (Pagina 1, Deslocamento 6) -> PAGE FAULT -> Pagina 1 (PID 1) alocada no Frame livre 5
Acesso: PID 2, Endereco 44 (Pagina 2, Deslocamento 14) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 0) (Frame 0) sera desalocada. -> Pagina 2 (PID 2) alocada no Frame 0
Acesso: PID 1, Endereco 10 (Pagina 0, Deslocamento 10) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 0) (Frame 1) sera desalocada. -> Pagina 0 (PID 1) alocada no Frame 1
Acesso: PID 0, Endereco 55 (Pagina 3, Deslocamento 10) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 2) (Frame 2) sera desalocada. -> Pagina 3 (PID 0) alocada no Frame 2
Acesso: PID 0, Endereco 48 (Pagina 3, Deslocamento 3) -> HIT: Pagina 3 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 0 (Pagina 0, Deslocamento 0) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 2) (Frame 3) sera desalocada. -> Pagina 0 (PID 0) alocada no Frame 3
Acesso: PID 2, Endereco 28 (Pagina 1, Deslocamento 13) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 0) (Frame 4) sera desalocada. -> Pagina 1 (PID 2) alocada no Frame 4
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 3
Acesso: PID 0, Endereco 18 (Pagina 1, Deslocamento 3) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 1) (Frame 5) sera desalocada. -> Pagina 1 (PID 0) alocada no Frame 5
Acesso: PID 1, Endereco 50 (Pagina 3, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 2) (Frame 0) sera desalocada. -> Pagina 3 (PID 1) alocada no Frame 0
Acesso: PID 0, Endereco 37 (Pagina 2, Deslocamento 7) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 1) (Frame 1) sera desalocada. -> Pagina 2 (PID 0) alocada no Frame 1
Acesso: PID 3, Endereco 73 (Pagina 4, Deslocamento 13) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 0) (Frame 2) sera desalocada. -> Pagina 4 (PID 3) alocada no Frame 2
Acesso: PID 3, Endereco 33 (Pagina 2, Deslocamento 3) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 0) (Frame 3) sera desalocada. -> Pagina 2 (PID 3) alocada no Frame 3
Acesso: PID 3, Endereco 4 (Pagina 0, Deslocamento 4) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 2) (Frame 4) sera desalocada. -> Pagina 0 (PID 3) alocada no Frame 4
Acesso: PID 3, Endereco 83 (Pagina 5, Deslocamento 8) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 0) (Frame 5) sera desalocada. -> Pagina 5 (PID 3) alocada no Frame 5
Acesso: PID 2, Endereco 6 (Pagina 0, Deslocamento 6) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 1) (Frame 0) sera desalocada. -> Pagina 0 (PID 2) alocada no Frame 0
Acesso: PID 3, Endereco 70 (Pagina 4, Deslocamento 10) -> HIT: Pagina 4 (PID 3) ja esta no Frame 2
Acesso: PID 2, Endereco 23 (Pagina 1, Deslocamento 8) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 0) (Frame 1) sera desalocada. -> Pagina 1 (PID 2) alocada no Frame 1
Acesso: PID 3, Endereco 28 (Pagina 1, Deslocamento 13) -> PAGE FAULT -> Memoria cheia. Pagina 4 (PID 3) (Frame 2) sera desalocada. -> Pagina 1 (PID 3) alocada no Frame 2
Acesso: PID 2, Endereco 23 (Pagina 1, Deslocamento 8) -> HIT: Pagina 1 (PID 2) ja esta no Frame 1
Acesso: PID 3, Endereco 83 (Pagina 5, Deslocamento 8) -> HIT: Pagina 5 (PID 3) ja esta no Frame 5
Acesso: PID 2, Endereco 6 (Pagina 0, Deslocamento 6) -> HIT: Pagina 0 (PID 2) ja esta no Frame 0
Acesso: PID 3, Endereco 28 (Pagina 1, Deslocamento 13) -> HIT: Pagina 1 (PID 3) ja esta no Frame 2
Acesso: PID 2, Endereco 6 (Pagina 0, Deslocamento 6) -> HIT: Pagina 0 (PID 2) ja esta no Frame 0
Acesso: PID 2, Endereco 6 (Pagina 0, Deslocamento 6) -> HIT: Pagina 0 (PID 2) ja esta no Frame 0
Acesso: PID 3, Endereco 70 (Pagina 4, Deslocamento 10) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 3) (Frame 3) sera desalocada. -> Pagina 4 (PID 3) alocada no Frame 3
Acesso: PID 3, Endereco 83 (Pagina 5, Deslocamento 8) -> HIT: Pagina 5 (PID 3) ja esta no Frame 5
Acesso: PID 2, Endereco 23 (Pagina 1, Deslocamento 8) -> HIT: Pagina 1 (PID 2) ja esta no Frame 1
Acesso: PID 3, Endereco 83 (Pagina 5, Deslocamento 8) -> HIT: Pagina 5 (PID 3) ja esta no Frame 5
Acesso: PID 3, Endereco 83 (Pagina 5, Deslocamento 8) -> HIT: Pagina 5 (PID 3) ja esta no Frame 5
Acesso: PID 3, Endereco 70 (Pagina 4, Deslocamento 10) -> HIT: Pagina 4 (PID 3) ja esta no Frame 3
Acesso: PID 2, Endereco 6 (Pagina 0, Deslocamento 6) -> HIT: Pagina 0 (PID 2) ja esta no Frame 0
Acesso: PID 2, Endereco 23 (Pagina 1, Deslocamento 8) -> HIT: Pagina 1 (PID 2) ja esta no Frame 1
Acesso: PID 3, Endereco 28 (Pagina 1, Deslocamento 13) -> HIT: Pagina 1 (PID 3) ja esta no Frame 2
Acesso: PID 3, Endereco 70 (Pagina 4, Deslocamento 10) -> HIT: Pagina 4 (PID 3) ja esta no Frame 3
Acesso: PID 2, Endereco 23 (Pagina 1, Deslocamento 8) -> HIT: Pagina 1 (PID 2) ja esta no Frame 1
Acesso: PID 2, Endereco 6 (Pagina 0, Deslocamento 6) -> HIT: Pagina 0 (PID 2) ja esta no Frame 0
Acesso: PID 2, Endereco 23 (Pagina 1, Deslocamento 8) -> HIT: Pagina 1 (PID 2) ja esta no Frame 1
Acesso: PID 2, Endereco 6 (Pagina 0, Deslocamento 6) -> HIT: Pagina 0 (PID 2) ja esta no Frame 0
Acesso: PID 3, Endereco 28 (Pagina 1, Deslocamento 13) -> HIT: Pagina 1 (PID 3) ja esta no Frame 2
Acesso: PID 3, Endereco 83 (Pagina 5, Deslocamento 8) -> HIT: Pagina 5 (PID 3) ja esta no Frame 5
Acesso: PID 3, Endereco 70 (Pagina 4, Deslocamento 10) -> HIT: Pagina 4 (PID 3) ja esta no Frame 3
Acesso: PID 3, Endereco 70 (Pagina 4, Deslocamento 10) -> HIT: Pagina 4 (PID 3) ja esta no Frame 3
Acesso: PID 2, Endereco 23 (Pagina 1, Deslocamento 8) -> HIT: Pagina 1 (PID 2) ja esta no Frame 1
Acesso: PID 2, Endereco 27 (Pagina 1, Deslocamento 12) -> HIT: Pagina 1 (PID 2) ja esta no Frame 1
Acesso: PID 0, Endereco 9 (Pagina 0, Deslocamento 9) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 3) (Frame 4) sera desalocada. -> Pagina 0 (PID 0) alocada no Frame 4
Acesso: PID 2, Endereco 34 (Pagina 2, Deslocamento 4) -> PAGE FAULT -> Memoria cheia. Pagina 5 (PID 3) (Frame 5) sera desalocada. -> Pagina 2 (PID 2) alocada no Frame 5
Acesso: PID 1, Endereco 47 (Pagina 3, Deslocamento 2) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 2) (Frame 0) sera desalocada. -> Pagina 3 (PID 1) alocada no Frame 0
Acesso: PID 0, Endereco 9 (Pagina 0, Deslocamento 9) -> HIT: Pagina 0 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 6 (Pagina 0, Deslocamento 6) -> HIT: Pagina 0 (PID 0) ja esta no Frame 4
Acesso: PID 2, Endereco 34 (Pagina 2, Deslocamento 4) -> HIT: Pagina 2 (PID 2) ja esta no Frame 5
Acesso: PID 2, Endereco 27 (Pagina 1, Deslocamento 12) -> HIT: Pagina 1 (PID 2) ja esta no Frame 1
Acesso: PID 0, Endereco 42 (Pagina 2, Deslocamento 12) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 2) (Frame 1) sera desalocada. -> Pagina 2 (PID 0) alocada no Frame 1
Acesso: PID 1, Endereco 48 (Pagina 3, Deslocamento 3) -> HIT: Pagina 3 (PID 1) ja esta no Frame 0
Acesso: PID 3, Endereco 81 (Pagina 5, Deslocamento 6) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 3) (Frame 2) sera desalocada. -> Pagina 5 (PID 3) alocada no Frame 2
Acesso: PID 2, Endereco 27 (Pagina 1, Deslocamento 12) -> PAGE FAULT -> Memoria cheia. Pagina 4 (PID 3) (Frame 3) sera desalocada. -> Pagina 1 (PID 2) alocada no Frame 3
Acesso: PID 0, Endereco 9 (Pagina 0, Deslocamento 9) -> HIT: Pagina 0 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 42 (Pagina 2, Deslocamento 12) -> HIT: Pagina 2 (PID 0) ja esta no Frame 1
Acesso: PID 2, Endereco 34 (Pagina 2, Deslocamento 4) -> HIT: Pagina 2 (PID 2) ja esta no Frame 5
Acesso: PID 0, Endereco 42 (Pagina 2, Deslocamento 12) -> HIT: Pagina 2 (PID 0) ja esta no Frame 1
Acesso: PID 2, Endereco 27 (Pagina 1, Deslocamento 12) -> HIT: Pagina 1 (PID 2) ja esta no Frame 3
Acesso: PID 0, Endereco 6 (Pagina 0, Deslocamento 6) -> HIT: Pagina 0 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 6 (Pagina 0, Deslocamento 6) -> HIT: Pagina 0 (PID 0) ja esta no Frame 4
Acesso: PID 1, Endereco 51 (Pagina 3, Deslocamento 6) -> HIT: Pagina 3 (PID 1) ja esta no Frame 0
Acesso: PID 2, Endereco 34 (Pagina 2, Deslocamento 4) -> HIT: Pagina 2 (PID 2) ja esta no Frame 5
Acesso: PID 1, Endereco 47 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 1) ja esta no Frame 0
Acesso: PID 0, Endereco 6 (Pagina 0, Deslocamento 6) -> HIT: Pagina 0 (PID 0) ja esta no Frame 4
Acesso: PID 1, Endereco 51 (Pagina 3, Deslocamento 6) -> HIT: Pagina 3 (PID 1) ja esta no Frame 0
Acesso: PID 0, Endereco 42 (Pagina 2, Deslocamento 12) -> HIT: Pagina 2 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 6 (Pagina 0, Deslocamento 6) -> HIT: Pagina 0 (PID 0) ja esta no Frame 4
Acesso: PID 3, Endereco 0 (Pagina 0, Deslocamento 0) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 0) (Frame 4) sera desalocada. -> Pagina 0 (PID 3) alocada no Frame 4
Acesso: PID 1, Endereco 47 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 1) ja esta no Frame 0
Acesso: PID 0, Endereco 6 (Pagina 0, Deslocamento 6) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 2) (Frame 5) sera desalocada. -> Pagina 0 (PID 0) alocada no Frame 5
Acesso: PID 2, Endereco 27 (Pagina 1, Deslocamento 12) -> HIT: Pagina 1 (PID 2) ja esta no Frame 3
Acesso: PID 3, Endereco 68 (Pagina 4, Deslocamento 8) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 1) (Frame 0) sera desalocada. -> Pagina 4 (PID 3) alocada no Frame 0
Acesso: PID 2, Endereco 9 (Pagina 0, Deslocamento 9) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 0) (Frame 1) sera desalocada. -> Pagina 0 (PID 2) alocada no Frame 1
Acesso: PID 2, Endereco 24 (Pagina 1, Deslocamento 9) -> HIT: Pagina 1 (PID 2) ja esta no Frame 3
Acesso: PID 2, Endereco 8 (Pagina 0, Deslocamento 8) -> HIT: Pagina 0 (PID 2) ja esta no Frame 1
Acesso: PID 2, Endereco 33 (Pagina 2, Deslocamento 3) -> PAGE FAULT -> Memoria cheia. Pagina 5 (PID 3) (Frame 2) sera desalocada. -> Pagina 2 (PID 2) alocada no Frame 2
Acesso: PID 0, Endereco 21 (Pagina 1, Deslocamento 6) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 2) (Frame 3) sera desalocada. -> Pagina 1 (PID 0) alocada no Frame 3
Acesso: PID 3, Endereco 55 (Pagina 3, Deslocamento 10) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 3) (Frame 4) sera desalocada. -> Pagina 3 (PID 3) alocada no Frame 4
Acesso: PID 0, Endereco 17 (Pagina 1, Deslocamento 2) -> HIT: Pagina 1 (PID 0) ja esta no Frame 3
Acesso: PID 3, Endereco 85 (Pagina 5, Deslocamento 10) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 0) (Frame 5) sera desalocada. -> Pagina 5 (PID 3) alocada no Frame 5
Acesso: PID 0, Endereco 44 (Pagina 2, Deslocamento 14) -> PAGE FAULT -> Memoria cheia. Pagina 4 (PID 3) (Frame 0) sera desalocada. -> Pagina 2 (PID 0) alocada no Frame 0
Acesso: PID 3, Endereco 85 (Pagina 5, Deslocamento 10) -> HIT: Pagina 5 (PID 3) ja esta no Frame 5
Acesso: PID 3, Endereco 38 (Pagina 2, Deslocamento 8) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 2) (Frame 1) sera desalocada. -> Pagina 2 (PID 3) alocada no Frame 1
Acesso: PID 3, Endereco 3 (Pagina 0, Deslocamento 3) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 2) (Frame 2) sera desalocada. -> Pagina 0 (PID 3) alocada no Frame 2
Acesso: PID 1, Endereco 33 (Pagina 2, Deslocamento 3) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 0) (Frame 3) sera desalocada. -> Pagina 2 (PID 1) alocada no Frame 3
Acesso: PID 2, Endereco 33 (Pagina 2, Deslocamento 3) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 3) (Frame 4) sera desalocada. -> Pagina 2 (PID 2) alocada no Frame 4
Acesso: PID 1, Endereco 21 (Pagina 1, Deslocamento 6) -> PAGE FAULT -> Memoria cheia. Pagina 5 (PID 3) (Frame 5) sera desalocada. -> Pagina 1 (PID 1) alocada no Frame 5
Acesso: PID 0, Endereco 69 (Pagina 4, Deslocamento 9) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 0) (Frame 0) sera desalocada. -> Pagina 4 (PID 0) alocada no Frame 0
Acesso: PID 2, Endereco 14 (Pagina 0, Deslocamento 14) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 3) (Frame 1) sera desalocada. -> Pagina 0 (PID 2) alocada no Frame 1
Acesso: PID 2, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 2) ja esta no Frame 1
Acesso: PID 2, Endereco 32 (Pagina 2, Deslocamento 2) -> HIT: Pagina 2 (PID 2) ja esta no Frame 4
Acesso: PID 1, Endereco 13 (Pagina 0, Deslocamento 13) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 3) (Frame 2) sera desalocada. -> Pagina 0 (PID 1) alocada no Frame 2
Acesso: PID 2, Endereco 16 (Pagina 1, Deslocamento 1) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 1) (Frame 3) sera desalocada. -> Pagina 1 (PID 2) alocada no Frame 3
Acesso: PID 3, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 2) (Frame 4) sera desalocada. -> Pagina 0 (PID 3) alocada no Frame 4
Acesso: PID 0, Endereco 25 (Pagina 1, Deslocamento 10) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 1) (Frame 5) sera desalocada. -> Pagina 1 (PID 0) alocada no Frame 5
Acesso: PID 1, Endereco 48 (Pagina 3, Deslocamento 3) -> PAGE FAULT -> Memoria cheia. Pagina 4 (PID 0) (Frame 0) sera desalocada. -> Pagina 3 (PID 1) alocada no Frame 0
Acesso: PID 0, Endereco 24 (Pagina 1, Deslocamento 9) -> HIT: Pagina 1 (PID 0) ja esta no Frame 5
Acesso: PID 2, Endereco 36 (Pagina 2, Deslocamento 6) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 2) (Frame 1) sera desalocada. -> Pagina 2 (PID 2) alocada no Frame 1
Acesso: PID 1, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 1) ja esta no Frame 2
Acesso: PID 2, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 1) (Frame 2) sera desalocada. -> Pagina 0 (PID 2) alocada no Frame 2
Acesso: PID 2, Endereco 35 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 2) ja esta no Frame 1
Acesso: PID 1, Endereco 59 (Pagina 3, Deslocamento 14) -> HIT: Pagina 3 (PID 1) ja esta no Frame 0
Acesso: PID 1, Endereco 50 (Pagina 3, Deslocamento 5) -> HIT: Pagina 3 (PID 1) ja esta no Frame 0
Acesso: PID 3, Endereco 63 (Pagina 4, Deslocamento 3) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 2) (Frame 3) sera desalocada. -> Pagina 4 (PID 3) alocada no Frame 3
Acesso: PID 3, Endereco 3 (Pagina 0, Deslocamento 3) -> HIT: Pagina 0 (PID 3) ja esta no Frame 4
Acesso: PID 2, Endereco 36 (Pagina 2, Deslocamento 6) -> HIT: Pagina 2 (PID 2) ja esta no Frame 1
Acesso: PID 3, Endereco 43 (Pagina 2, Deslocamento 13) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 3) (Frame 4) sera desalocada. -> Pagina 2 (PID 3) alocada no Frame 4
Acesso: PID 3, Endereco 28 (Pagina 1, Deslocamento 13) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 0) (Frame 5) sera desalocada. -> Pagina 1 (PID 3) alocada no Frame 5
Acesso: PID 1, Endereco 39 (Pagina 2, Deslocamento 9) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 1) (Frame 0) sera desalocada. -> Pagina 2 (PID 1) alocada no Frame 0
Acesso: PID 0, Endereco 63 (Pagina 4, Deslocamento 3) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 2) (Frame 1) sera desalocada. -> Pagina 4 (PID 0) alocada no Frame 1
Acesso: PID 2, Endereco 44 (Pagina 2, Deslocamento 14) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 2) (Frame 2) sera desalocada. -> Pagina 2 (PID 2) alocada no Frame 2
Acesso: PID 0, Endereco 6 (Pagina 0, Deslocamento 6) -> PAGE FAULT -> Memoria cheia. Pagina 4 (PID 3) (Frame 3) sera desalocada. -> Pagina 0 (PID 0) alocada no Frame 3
Acesso: PID 2, Endereco 29 (Pagina 1, Deslocamento 14) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 3) (Frame 4) sera desalocada. -> Pagina 1 (PID 2) alocada no Frame 4
Acesso: PID 2, Endereco 42 (Pagina 2, Deslocamento 12) -> HIT: Pagina 2 (PID 2) ja esta no Frame 2
Acesso: PID 3, Endereco 40 (Pagina 2, Deslocamento 10) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 3) (Frame 5) sera desalocada. -> Pagina 2 (PID 3) alocada no Frame 5
Acesso: PID 2, Endereco 13 (Pagina 0, Deslocamento 13) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 1) (Frame 0) sera desalocada. -> Pagina 0 (PID 2) alocada no Frame 0
Acesso: PID 3, Endereco 43 (Pagina 2, Deslocamento 13) -> HIT: Pagina 2 (PID 3) ja esta no Frame 5
Acesso: PID 1, Endereco 52 (Pagina 3, Deslocamento 7) -> PAGE FAULT -> Memoria cheia. Pagina 4 (PID 0) (Frame 1) sera desalocada. -> Pagina 3 (PID 1) alocada no Frame 1
Acesso: PID 1, Endereco 24 (Pagina 1, Deslocamento 9) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 2) (Frame 2) sera desalocada. -> Pagina 1 (PID 1) alocada no Frame 2
Acesso: PID 0, Endereco 53 (Pagina 3, Deslocamento 8) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 0) (Frame 3) sera desalocada. -> Pagina 3 (PID 0) alocada no Frame 3
Acesso: PID 3, Endereco 64 (Pagina 4, Deslocamento 4) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 2) (Frame 4) sera desalocada. -> Pagina 4 (PID 3) alocada no Frame 4
Acesso: PID 2, Endereco 11 (Pagina 0, Deslocamento 11) -> HIT: Pagina 0 (PID 2) ja esta no Frame 0
Acesso: PID 3, Endereco 47 (Pagina 3, Deslocamento 2) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 3) (Frame 5) sera desalocada. -> Pagina 3 (PID 3) alocada no Frame 5
Acesso: PID 0, Endereco 37 (Pagina 2, Deslocamento 7) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 2) (Frame 0) sera desalocada. -> Pagina 2 (PID 0) alocada no Frame 0
Acesso: PID 0, Endereco 33 (Pagina 2, Deslocamento 3) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 2, Endereco 3 (Pagina 0, Deslocamento 3) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 1) (Frame 1) sera desalocada. -> Pagina 0 (PID 2) alocada no Frame 1
Acesso: PID 3, Endereco 6 (Pagina 0, Deslocamento 6) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 1) (Frame 2) sera desalocada. -> Pagina 0 (PID 3) alocada no Frame 2
Acesso: PID 3, Endereco 38 (Pagina 2, Deslocamento 8) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 0) (Frame 3) sera desalocada. -> Pagina 2 (PID 3) alocada no Frame 3
Acesso: PID 0, Endereco 59 (Pagina 3, Deslocamento 14) -> PAGE FAULT -> Memoria cheia. Pagina 4 (PID 3) (Frame 4) sera desalocada. -> Pagina 3 (PID 0) alocada no Frame 4
Acesso: PID 2, Endereco 15 (Pagina 1, Deslocamento 0) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 3) (Frame 5) sera desalocada. -> Pagina 1 (PID 2) alocada no Frame 5
Acesso: PID 3, Endereco 34 (Pagina 2, Deslocamento 4) -> HIT: Pagina 2 (PID 3) ja esta no Frame 3
Acesso: PID 3, Endereco 72 (Pagina 4, Deslocamento 12) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 0) (Frame 0) sera desalocada. -> Pagina 4 (PID 3) alocada no Frame 0
Acesso: PID 2, Endereco 15 (Pagina 1, Deslocamento 0) -> HIT: Pagina 1 (PID 2) ja esta no Frame 5
Acesso: PID 3, Endereco 64 (Pagina 4, Deslocamento 4) -> HIT: Pagina 4 (PID 3) ja esta no Frame 0
Acesso: PID 2, Endereco 21 (Pagina 1, Deslocamento 6) -> HIT: Pagina 1 (PID 2) ja esta no Frame 5
Acesso: PID 0, Endereco 23 (Pagina 1, Deslocamento 8) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 2) (Frame 1) sera desalocada. -> Pagina 1 (PID 0) alocada no Frame 1
Acesso: PID 0, Endereco 2 (Pagina 0, Deslocamento 2) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 3) (Frame 2) sera desalocada. -> Pagina 0 (PID 0) alocada no Frame 2
Acesso: PID 3, Endereco 84 (Pagina 5, Deslocamento 9) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 3) (Frame 3) sera desalocada. -> Pagina 5 (PID 3) alocada no Frame 3
Acesso: PID 3, Endereco 16 (Pagina 1, Deslocamento 1) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 0) (Frame 4) sera desalocada. -> Pagina 1 (PID 3) alocada no Frame 4
Acesso: PID 3, Endereco 68 (Pagina 4, Deslocamento 8) -> HIT: Pagina 4 (PID 3) ja esta no Frame 0
Acesso: PID 0, Endereco 60 (Pagina 4, Deslocamento 0) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 2) (Frame 5) sera desalocada. -> Pagina 4 (PID 0) alocada no Frame 5
--- Simulacao Finalizada (Algoritmo: fifo)
Total de Acessos: 150
Total de Page Faults: 78
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> .\simulador.exe fifo ../../tests/config_6.txt ../../tests/acessos_6.txt ../../output_fifo_6.txt
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> Get-Content ../../output_fifo_6.txt
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Pagina 0 (PID 0) alocada no Frame livre 0
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> PAGE FAULT -> Pagina 1 (PID 0) alocada no Frame livre 1
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> PAGE FAULT -> Pagina 2 (PID 0) alocada no Frame livre 2
Acesso: PID 0, Endereco 35 (Pagina 3, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 0) (Frame 0) sera desalocada. -> Pagina 3 (PID 0) alocada no Frame 0
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 45 (Pagina 4, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 0) (Frame 1) sera desalocada. -> Pagina 4 (PID 0) alocada no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 0) (Frame 2) sera desalocada. -> Pagina 1 (PID 0) alocada no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 0) (Frame 0) sera desalocada. -> Pagina 2 (PID 0) alocada no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 4 (PID 0) (Frame 1) sera desalocada. -> Pagina 0 (PID 0) alocada no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
--- Simulacao Finalizada (Algoritmo: fifo)
Total de Acessos: 41
Total de Page Faults: 8

*/


/*
/*Resultado: Simulador de Gerenciamento de Memória com algoritmo Clock:
O Windows PowerShell
Copyright (C) Microsoft Corporation. Todos os direitos reservados.

Instale o PowerShell mais recente para obter novos recursos e aprimoramentos! https://aka.ms/PSWindows

PS C:\Users\david>  cd C:\Users\david\proj2-pagingsim-grupodavid\src\simulator
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator>  gcc -o simulador.exe simulador.c
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> dir simulador.exe


    Diretório: C:\Users\david\proj2-pagingsim-grupodavid\src\simulator


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a----        02/11/2025     20:55          45946 simulador.exe


PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> .\simulador.exe clock ../../tests/config_1.txt ../../tests/acessos_1.txt ../../output_clock_1.txt
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> Get-Content ../../output_clock_1.txt
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> PAGE FAULT -> Pagina 2 (PID 0) alocada no Frame livre 0
Acesso: PID 1, Endereco 12 (Pagina 1, Deslocamento 2) -> PAGE FAULT -> Pagina 1 (PID 1) alocada no Frame livre 1
Acesso: PID 0, Endereco 28 (Pagina 2, Deslocamento 8) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 1, Endereco 55 (Pagina 5, Deslocamento 5) -> PAGE FAULT -> Pagina 5 (PID 1) alocada no Frame livre 2
Acesso: PID 0, Endereco 42 (Pagina 4, Deslocamento 2) -> PAGE FAULT -> Pagina 4 (PID 0) alocada no Frame livre 3
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 1, Endereco 12 (Pagina 1, Deslocamento 2) -> HIT: Pagina 1 (PID 1) ja esta no Frame 1
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Pagina 0 (PID 0) alocada no Frame livre 4
--- Simulacao Finalizada (Algoritmo: clock)
Total de Acessos: 8
Total de Page Faults: 5
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> .\simulador.exe clock ../../tests/config_2.txt ../../tests/acessos_2.txt ../../output_clock_2.txt
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> Get-Content ../../output_clock_2.txt
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Pagina 0 (PID 0) alocada no Frame livre 0
Acesso: PID 1, Endereco 8 (Pagina 0, Deslocamento 8) -> PAGE FAULT -> Pagina 0 (PID 1) alocada no Frame livre 1
Acesso: PID 2, Endereco 12 (Pagina 1, Deslocamento 2) -> PAGE FAULT -> Pagina 1 (PID 2) alocada no Frame livre 2
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> PAGE FAULT -> Pagina 1 (PID 0) alocada no Frame livre 3
Acesso: PID 1, Endereco 20 (Pagina 2, Deslocamento 0) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 0) (Frame 0) sera desalocada. -> Pagina 2 (PID 1) alocada no Frame 0
Acesso: PID 2, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 1) (Frame 1) sera desalocada. -> Pagina 0 (PID 2) alocada no Frame 1
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 2) (Frame 2) sera desalocada. -> Pagina 2 (PID 0) alocada no Frame 2
Acesso: PID 1, Endereco 10 (Pagina 1, Deslocamento 0) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 0) (Frame 3) sera desalocada. -> Pagina 1 (PID 1) alocada no Frame 3
Acesso: PID 2, Endereco 18 (Pagina 1, Deslocamento 8) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 1) (Frame 0) sera desalocada. -> Pagina 1 (PID 2) alocada no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 2) (Frame 1) sera desalocada. -> Pagina 0 (PID 0) alocada no Frame 1
--- Simulacao Finalizada (Algoritmo: clock)
Total de Acessos: 10
Total de Page Faults: 10
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> .\simulador.exe clock ../../tests/config_3.txt ../../tests/acessos_3.txt ../../output_clock_3.txt
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> Get-Content ../../output_clock_3.txt
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Pagina 0 (PID 0) alocada no Frame livre 0
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> PAGE FAULT -> Pagina 1 (PID 0) alocada no Frame livre 1
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> PAGE FAULT -> Pagina 2 (PID 0) alocada no Frame livre 2
Acesso: PID 0, Endereco 35 (Pagina 3, Deslocamento 5) -> PAGE FAULT -> Pagina 3 (PID 0) alocada no Frame livre 3
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 2
--- Simulacao Finalizada (Algoritmo: clock)
Total de Acessos: 7
Total de Page Faults: 4
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> .\simulador.exe clock ../../tests/config_4.txt ../../tests/acessos_4.txt ../../output_clock_4.txt
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> Get-Content ../../output_clock_4.txt
Acesso: PID 0, Endereco 3 (Pagina 0, Deslocamento 3) -> PAGE FAULT -> Pagina 0 (PID 0) alocada no Frame livre 0
Acesso: PID 2, Endereco 15 (Pagina 0, Deslocamento 15) -> PAGE FAULT -> Pagina 0 (PID 2) alocada no Frame livre 1
Acesso: PID 1, Endereco 17 (Pagina 0, Deslocamento 17) -> PAGE FAULT -> Pagina 0 (PID 1) alocada no Frame livre 2
Acesso: PID 0, Endereco 69 (Pagina 3, Deslocamento 9) -> PAGE FAULT -> Pagina 3 (PID 0) alocada no Frame livre 3
Acesso: PID 0, Endereco 75 (Pagina 3, Deslocamento 15) -> HIT: Pagina 3 (PID 0) ja esta no Frame 3
Acesso: PID 3, Endereco 4 (Pagina 0, Deslocamento 4) -> PAGE FAULT -> Pagina 0 (PID 3) alocada no Frame livre 4
Acesso: PID 0, Endereco 11 (Pagina 0, Deslocamento 11) -> HIT: Pagina 0 (PID 0) ja esta no Frame 0
Acesso: PID 1, Endereco 29 (Pagina 1, Deslocamento 9) -> PAGE FAULT -> Pagina 1 (PID 1) alocada no Frame livre 5
Acesso: PID 0, Endereco 71 (Pagina 3, Deslocamento 11) -> HIT: Pagina 3 (PID 0) ja esta no Frame 3
Acesso: PID 1, Endereco 91 (Pagina 4, Deslocamento 11) -> PAGE FAULT -> Pagina 4 (PID 1) alocada no Frame livre 6
Acesso: PID 3, Endereco 28 (Pagina 1, Deslocamento 8) -> PAGE FAULT -> Pagina 1 (PID 3) alocada no Frame livre 7
Acesso: PID 3, Endereco 75 (Pagina 3, Deslocamento 15) -> PAGE FAULT -> Pagina 3 (PID 3) alocada no Frame livre 8
Acesso: PID 2, Endereco 51 (Pagina 2, Deslocamento 11) -> PAGE FAULT -> Pagina 2 (PID 2) alocada no Frame livre 9
Acesso: PID 0, Endereco 20 (Pagina 1, Deslocamento 0) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 0) (Frame 0) sera desalocada. -> Pagina 1 (PID 0) alocada no Frame 0
Acesso: PID 3, Endereco 43 (Pagina 2, Deslocamento 3) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 2) (Frame 1) sera desalocada. -> Pagina 2 (PID 3) alocada no Frame 1
Acesso: PID 2, Endereco 9 (Pagina 0, Deslocamento 9) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 1) (Frame 2) sera desalocada. -> Pagina 0 (PID 2) alocada no Frame 2
Acesso: PID 1, Endereco 97 (Pagina 4, Deslocamento 17) -> HIT: Pagina 4 (PID 1) ja esta no Frame 6
Acesso: PID 2, Endereco 6 (Pagina 0, Deslocamento 6) -> HIT: Pagina 0 (PID 2) ja esta no Frame 2
Acesso: PID 0, Endereco 48 (Pagina 2, Deslocamento 8) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 0) (Frame 3) sera desalocada. -> Pagina 2 (PID 0) alocada no Frame 3
Acesso: PID 0, Endereco 45 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 3
Acesso: PID 2, Endereco 48 (Pagina 2, Deslocamento 8) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 2, Endereco 48 (Pagina 2, Deslocamento 8) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 0, Endereco 77 (Pagina 3, Deslocamento 17) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 3) (Frame 4) sera desalocada. -> Pagina 3 (PID 0) alocada no Frame 4
Acesso: PID 0, Endereco 62 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 2, Endereco 48 (Pagina 2, Deslocamento 8) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 0, Endereco 77 (Pagina 3, Deslocamento 17) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 2, Endereco 48 (Pagina 2, Deslocamento 8) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 2, Endereco 59 (Pagina 2, Deslocamento 19) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 0, Endereco 62 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 2, Endereco 59 (Pagina 2, Deslocamento 19) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 0, Endereco 62 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 77 (Pagina 3, Deslocamento 17) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 62 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 62 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 77 (Pagina 3, Deslocamento 17) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 62 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 2, Endereco 48 (Pagina 2, Deslocamento 8) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 2, Endereco 46 (Pagina 2, Deslocamento 6) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 0, Endereco 77 (Pagina 3, Deslocamento 17) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 2, Endereco 46 (Pagina 2, Deslocamento 6) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 0, Endereco 77 (Pagina 3, Deslocamento 17) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 0, Endereco 77 (Pagina 3, Deslocamento 17) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 2, Endereco 59 (Pagina 2, Deslocamento 19) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 2, Endereco 59 (Pagina 2, Deslocamento 19) -> HIT: Pagina 2 (PID 2) ja esta no Frame 9
Acesso: PID 0, Endereco 62 (Pagina 3, Deslocamento 2) -> HIT: Pagina 3 (PID 0) ja esta no Frame 4
Acesso: PID 3, Endereco 92 (Pagina 4, Deslocamento 12) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 1) (Frame 5) sera desalocada. -> Pagina 4 (PID 3) alocada no Frame 5
Acesso: PID 3, Endereco 79 (Pagina 3, Deslocamento 19) -> HIT: Pagina 3 (PID 3) ja esta no Frame 8
Acesso: PID 1, Endereco 41 (Pagina 2, Deslocamento 1) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 3) (Frame 7) sera desalocada. -> Pagina 2 (PID 1) alocada no Frame 7
Acesso: PID 3, Endereco 1 (Pagina 0, Deslocamento 1) -> PAGE FAULT -> Memoria cheia. Pagina 4 (PID 1) (Frame 6) sera desalocada. -> Pagina 0 (PID 3) alocada no Frame 6
Acesso: PID 0, Endereco 25 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 0
Acesso: PID 1, Endereco 10 (Pagina 0, Deslocamento 10) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 3) (Frame 8) sera desalocada. -> Pagina 0 (PID 1) alocada no Frame 8
Acesso: PID 3, Endereco 1 (Pagina 0, Deslocamento 1) -> HIT: Pagina 0 (PID 3) ja esta no Frame 6
Acesso: PID 3, Endereco 88 (Pagina 4, Deslocamento 8) -> HIT: Pagina 4 (PID 3) ja esta no Frame 5
Acesso: PID 3, Endereco 92 (Pagina 4, Deslocamento 12) -> HIT: Pagina 4 (PID 3) ja esta no Frame 5
Acesso: PID 3, Endereco 82 (Pagina 4, Deslocamento 2) -> HIT: Pagina 4 (PID 3) ja esta no Frame 5
Acesso: PID 1, Endereco 72 (Pagina 3, Deslocamento 12) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 2) (Frame 9) sera desalocada. -> Pagina 3 (PID 1) alocada no Frame 9
Acesso: PID 3, Endereco 82 (Pagina 4, Deslocamento 2) -> HIT: Pagina 4 (PID 3) ja esta no Frame 5
Acesso: PID 3, Endereco 1 (Pagina 0, Deslocamento 1) -> HIT: Pagina 0 (PID 3) ja esta no Frame 6
Acesso: PID 3, Endereco 28 (Pagina 1, Deslocamento 8) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 3) (Frame 1) sera desalocada. -> Pagina 1 (PID 3) alocada no Frame 1
Acesso: PID 1, Endereco 10 (Pagina 0, Deslocamento 10) -> HIT: Pagina 0 (PID 1) ja esta no Frame 8
Acesso: PID 1, Endereco 72 (Pagina 3, Deslocamento 12) -> HIT: Pagina 3 (PID 1) ja esta no Frame 9
Acesso: PID 1, Endereco 37 (Pagina 1, Deslocamento 17) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 2) (Frame 2) sera desalocada. -> Pagina 1 (PID 1) alocada no Frame 2
Acesso: PID 3, Endereco 42 (Pagina 2, Deslocamento 2) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 0) (Frame 3) sera desalocada. -> Pagina 2 (PID 3) alocada no Frame 3
Acesso: PID 2, Endereco 58 (Pagina 2, Deslocamento 18) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 0) (Frame 4) sera desalocada. -> Pagina 2 (PID 2) alocada no Frame 4
Acesso: PID 1, Endereco 41 (Pagina 2, Deslocamento 1) -> HIT: Pagina 2 (PID 1) ja esta no Frame 7
Acesso: PID 0, Endereco 25 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 0
Acesso: PID 3, Endereco 88 (Pagina 4, Deslocamento 8) -> HIT: Pagina 4 (PID 3) ja esta no Frame 5
Acesso: PID 0, Endereco 25 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 0
Acesso: PID 1, Endereco 72 (Pagina 3, Deslocamento 12) -> HIT: Pagina 3 (PID 1) ja esta no Frame 9
Acesso: PID 3, Endereco 92 (Pagina 4, Deslocamento 12) -> HIT: Pagina 4 (PID 3) ja esta no Frame 5
Acesso: PID 1, Endereco 64 (Pagina 3, Deslocamento 4) -> HIT: Pagina 3 (PID 1) ja esta no Frame 9
Acesso: PID 0, Endereco 38 (Pagina 1, Deslocamento 18) -> HIT: Pagina 1 (PID 0) ja esta no Frame 0
Acesso: PID 1, Endereco 19 (Pagina 0, Deslocamento 19) -> HIT: Pagina 0 (PID 1) ja esta no Frame 8
Acesso: PID 2, Endereco 48 (Pagina 2, Deslocamento 8) -> HIT: Pagina 2 (PID 2) ja esta no Frame 4
Acesso: PID 1, Endereco 69 (Pagina 3, Deslocamento 9) -> HIT: Pagina 3 (PID 1) ja esta no Frame 9
Acesso: PID 0, Endereco 76 (Pagina 3, Deslocamento 16) -> PAGE FAULT -> Memoria cheia. Pagina 4 (PID 3) (Frame 5) sera desalocada. -> Pagina 3 (PID 0) alocada no Frame 5
Acesso: PID 2, Endereco 31 (Pagina 1, Deslocamento 11) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 3) (Frame 6) sera desalocada. -> Pagina 1 (PID 2) alocada no Frame 6
Acesso: PID 0, Endereco 14 (Pagina 0, Deslocamento 14) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 1) (Frame 7) sera desalocada. -> Pagina 0 (PID 0) alocada no Frame 7
Acesso: PID 2, Endereco 56 (Pagina 2, Deslocamento 16) -> HIT: Pagina 2 (PID 2) ja esta no Frame 4
Acesso: PID 2, Endereco 15 (Pagina 0, Deslocamento 15) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 1) (Frame 8) sera desalocada. -> Pagina 0 (PID 2) alocada no Frame 8
Acesso: PID 0, Endereco 30 (Pagina 1, Deslocamento 10) -> HIT: Pagina 1 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 10 (Pagina 0, Deslocamento 10) -> HIT: Pagina 0 (PID 0) ja esta no Frame 7
Acesso: PID 3, Endereco 8 (Pagina 0, Deslocamento 8) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 1) (Frame 9) sera desalocada. -> Pagina 0 (PID 3) alocada no Frame 9
Acesso: PID 1, Endereco 16 (Pagina 0, Deslocamento 16) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 3) (Frame 1) sera desalocada. -> Pagina 0 (PID 1) alocada no Frame 1
Acesso: PID 3, Endereco 70 (Pagina 3, Deslocamento 10) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 1) (Frame 2) sera desalocada. -> Pagina 3 (PID 3) alocada no Frame 2
Acesso: PID 1, Endereco 33 (Pagina 1, Deslocamento 13) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 3) (Frame 3) sera desalocada. -> Pagina 1 (PID 1) alocada no Frame 3
Acesso: PID 3, Endereco 27 (Pagina 1, Deslocamento 7) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 0) (Frame 0) sera desalocada. -> Pagina 1 (PID 3) alocada no Frame 0
Acesso: PID 1, Endereco 91 (Pagina 4, Deslocamento 11) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 2) (Frame 4) sera desalocada. -> Pagina 4 (PID 1) alocada no Frame 4
Acesso: PID 2, Endereco 25 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 2) ja esta no Frame 6
Acesso: PID 2, Endereco 28 (Pagina 1, Deslocamento 8) -> HIT: Pagina 1 (PID 2) ja esta no Frame 6
Acesso: PID 3, Endereco 15 (Pagina 0, Deslocamento 15) -> HIT: Pagina 0 (PID 3) ja esta no Frame 9
Acesso: PID 1, Endereco 28 (Pagina 1, Deslocamento 8) -> HIT: Pagina 1 (PID 1) ja esta no Frame 3
Acesso: PID 0, Endereco 43 (Pagina 2, Deslocamento 3) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 0) (Frame 5) sera desalocada. -> Pagina 2 (PID 0) alocada no Frame 5
Acesso: PID 0, Endereco 75 (Pagina 3, Deslocamento 15) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 0) (Frame 7) sera desalocada. -> Pagina 3 (PID 0) alocada no Frame 7
Acesso: PID 1, Endereco 75 (Pagina 3, Deslocamento 15) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 2) (Frame 8) sera desalocada. -> Pagina 3 (PID 1) alocada no Frame 8
Acesso: PID 1, Endereco 0 (Pagina 0, Deslocamento 0) -> HIT: Pagina 0 (PID 1) ja esta no Frame 1
Acesso: PID 0, Endereco 7 (Pagina 0, Deslocamento 7) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 3) (Frame 2) sera desalocada. -> Pagina 0 (PID 0) alocada no Frame 2
Acesso: PID 1, Endereco 8 (Pagina 0, Deslocamento 8) -> HIT: Pagina 0 (PID 1) ja esta no Frame 1
Acesso: PID 0, Endereco 42 (Pagina 2, Deslocamento 2) -> HIT: Pagina 2 (PID 0) ja esta no Frame 5
Acesso: PID 0, Endereco 65 (Pagina 3, Deslocamento 5) -> HIT: Pagina 3 (PID 0) ja esta no Frame 7
--- Simulacao Finalizada (Algoritmo: clock)
Total de Acessos: 100
Total de Page Faults: 38
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> .\simulador.exe clock ../../tests/config_5.txt ../../tests/acessos_5.txt ../../output_clock_5.txt
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> Get-Content ../../output_clock_1.txt
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> PAGE FAULT -> Pagina 2 (PID 0) alocada no Frame livre 0
Acesso: PID 1, Endereco 12 (Pagina 1, Deslocamento 2) -> PAGE FAULT -> Pagina 1 (PID 1) alocada no Frame livre 1
Acesso: PID 0, Endereco 28 (Pagina 2, Deslocamento 8) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 1, Endereco 55 (Pagina 5, Deslocamento 5) -> PAGE FAULT -> Pagina 5 (PID 1) alocada no Frame livre 2
Acesso: PID 0, Endereco 42 (Pagina 4, Deslocamento 2) -> PAGE FAULT -> Pagina 4 (PID 0) alocada no Frame livre 3
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 1, Endereco 12 (Pagina 1, Deslocamento 2) -> HIT: Pagina 1 (PID 1) ja esta no Frame 1
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Pagina 0 (PID 0) alocada no Frame livre 4
--- Simulacao Finalizada (Algoritmo: clock)
Total de Acessos: 8
Total de Page Faults: 5
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> .\simulador.exe clock ../../tests/config_6.txt ../../tests/acessos_6.txt ../../output_clock_6.txt
PS C:\Users\david\proj2-pagingsim-grupodavid\src\simulator> Get-Content ../../output_clock_6.txt
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Pagina 0 (PID 0) alocada no Frame livre 0
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> PAGE FAULT -> Pagina 1 (PID 0) alocada no Frame livre 1
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> PAGE FAULT -> Pagina 2 (PID 0) alocada no Frame livre 2
Acesso: PID 0, Endereco 35 (Pagina 3, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 0 (PID 0) (Frame 0) sera desalocada. -> Pagina 3 (PID 0) alocada no Frame 0
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 45 (Pagina 4, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 1 (PID 0) (Frame 1) sera desalocada. -> Pagina 4 (PID 0) alocada no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 2 (PID 0) (Frame 2) sera desalocada. -> Pagina 1 (PID 0) alocada no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 3 (PID 0) (Frame 0) sera desalocada. -> Pagina 2 (PID 0) alocada no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> PAGE FAULT -> Memoria cheia. Pagina 4 (PID 0) (Frame 1) sera desalocada. -> Pagina 0 (PID 0) alocada no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
Acesso: PID 0, Endereco 15 (Pagina 1, Deslocamento 5) -> HIT: Pagina 1 (PID 0) ja esta no Frame 2
Acesso: PID 0, Endereco 25 (Pagina 2, Deslocamento 5) -> HIT: Pagina 2 (PID 0) ja esta no Frame 0
Acesso: PID 0, Endereco 5 (Pagina 0, Deslocamento 5) -> HIT: Pagina 0 (PID 0) ja esta no Frame 1
--- Simulacao Finalizada (Algoritmo: clock)
Total de Acessos: 41
Total de Page Faults: 8

*/
