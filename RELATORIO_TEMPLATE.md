# Relatório do Projeto 2: Simulador de Memória Virtual

**Disciplina:** Sistemas Operacionais
**Professor:** Lucas Figueiredo
**Data:*02/11/2023*

## Integrantes do Grupo

- David Rodrigues de Oliveira - 10410867


---

## 1. Instruções de Compilação e Execução

### 1.1 Compilação

Descreva EXATAMENTE como compilar seu projeto. Inclua todos os comandos necessários.

**Exemplo:**
```bash
gcc -o simulador simulador.c
Utilizado: executável simulator na raiz do projeto.

```

ou

```bash
make
```

### 1.2 Execução

Forneça exemplos completos de como executar o simulador.

**Exemplo com FIFO:**
```bash
./simulador fifo tests/config_1.txt tests/acessos_1.txt
```

**Exemplo com Clock:**
```bash
./simulador clock tests/config_1.txt tests/acessos_1.txt
```

---

## 2. Decisões de Design

### 2.1 Estruturas de Dados

Descreva as estruturas de dados que você escolheu para representar:

**Tabela de Páginas:**
- Qual estrutura usou? (array, lista, hash map, etc.)
vetores (frames[] e rbit[]) dentro de uma struct Processo.
  
- Quais informações armazena para cada página?
frames[i]: número do quadro físico em que a página está (ou -1 se não estiver na memória).
rbit[i]: bit de referência (usado pelo algoritmo Clock).

- Como organizou para múltiplos processos?
cada processo tem sua própria tabela (processos[]).
  
- **Justificativa:** Por que escolheu essa abordagem?
vetores permitem acesso rápido e indexado à página, simplificando a lógica dos algoritmos.

**Frames Físicos:**
- Como representou os frames da memória física?
vetor de structs Frame memoria[MAX_FRAMES].

- Quais informações armazena para cada frame?
- Como rastreia frames livres vs ocupados?
ocupado: se o frame está sendo usado.
pid: dono do frame.
pagina: número da página carregada.

- **Justificativa:** Por que escolheu essa abordagem?
facilita percorrer e encontrar frames livres ou ocupados.

**Estrutura para FIFO:**
- Como mantém a ordem de chegada das páginas?
variável global fifo_index.

- Como identifica a página mais antiga?
índice indica a próxima vítima circularmente.

- **Justificativa:** Por que escolheu essa abordagem?
simples, eficiente e segue a ordem de chegada.

**Estrutura para Clock:**
- Como implementou o ponteiro circular?
variável global clock_hand.
- Como armazena e atualiza os R-bits?
percorre circularmente os frames, verificando o rbit da página associada.
- **Justificativa:** Por que escolheu essa abordagem?
simula com fidelidade o algoritmo da “segunda chance”.
  


### 2.2 Organização do Código

Descreva como organizou seu código:

- Quantos arquivos/módulos criou?
O código do simulador foi organizado em um único arquivo principal chamado simulador.c.
Todas as funções e estruturas estão centralizadas nesse arquivo, o que facilita a compilação e a execução direta sem necessidade de múltiplos módulos.

Arquivos criados:

simulador.c — contém toda a lógica do simulador (leitura de configuração, execução dos algoritmos FIFO e Clock, tratamento de page faults e geração de relatórios).

(Opcionalmente podem existir arquivos auxiliares de teste, como tests/config_1.txt e tests/acessos_1.txt, que servem apenas como entrada de dados.)

Responsabilidade do arquivo principal:

Implementar toda a simulação da memória virtual;

Ler arquivos de configuração e acessos;

Executar os algoritmos de substituição de página (FIFO e Clock);

Registrar resultados detalhados no arquivo de saída;

Gerar estatísticas finais da simulação.

- Qual a responsabilidade de cada arquivo/módulo?

Principais funções do código e suas responsabilidades:
Função	Responsabilidade
main()	Recebe os argumentos da linha de comando, chama as funções principais e coordena a execução completa do simulador.
ler_config(char *arquivo)	Lê o arquivo de configuração que define o número de frames, o tamanho de página e as informações de cada processo. Inicializa as estruturas de memória e processos.
procurar_frame_livre()	Percorre a memória física em busca de um frame livre. Retorna o índice do primeiro frame livre encontrado, ou -1 se a memória estiver cheia.
fifo_substituir()	Implementa a política FIFO (First-In, First-Out), selecionando o próximo frame a ser substituído com base na ordem de chegada. Atualiza o índice global fifo_index.
clock_substituir()	Implementa o algoritmo Relógio (Clock). Utiliza o ponteiro circular clock_hand e os bits de referência (rbit) para dar “segunda chance” às páginas acessadas recentemente.
acessar_memoria(char *algoritmo, int pid, int endereco)	Executa um acesso à memória. Calcula a página correspondente, verifica se está na memória (HIT) ou se ocorre page fault, e realiza a substituição de acordo com o algoritmo escolhido.
simular(char *algoritmo, char *arquivo_acessos, char *arquivo_saida)	Coordena toda a simulação: lê o arquivo de acessos, chama acessar_memoria() para cada linha e gera o relatório final com o total de acessos e falhas de página.

- Quais são as principais funções e o que cada uma faz?
O código foi implementado em um único módulo (simulador.c) por simplicidade e clareza.
Cada função possui uma responsabilidade específica, seguindo uma divisão lógica entre leitura de entrada, controle de simulação, execução dos algoritmos FIFO/Clock e geração de relatórios.
Essa estrutura modular dentro de um único arquivo facilita a depuração e mantém a coerência entre as etapas da simulação.

**Exemplo:**
```
simulador.c
├── main() - lê argumentos e coordena execução
├── ler_config() - processa arquivo de configuração
├── processar_acessos() - loop principal de simulação
├── traduzir_endereco() - calcula página e deslocamento
├── consultar_tabela() - verifica se página está na memória
├── tratar_page_fault() - lida com page faults
├── algoritmo_fifo() - seleciona vítima usando FIFO
└── algoritmo_clock() - seleciona vítima usando Clock
```

### 2.3 Algoritmo FIFO

Explique **como** implementou a lógica FIFO:
O algoritmo FIFO foi implementado de forma simples e direta, utilizando um índice global (fifo_index) para controlar a ordem de chegada das páginas à memória física.

- Como mantém o controle da ordem de chegada?
A cada nova página carregada na memória (durante um page fault), ela é alocada no próximo frame livre disponível.
O índice fifo_index é usado como um ponteiro circular, indicando qual frame foi o primeiro a ser ocupado.
Quando a memória fica cheia, o algoritmo começa a substituir as páginas na mesma ordem em que elas foram inseridas.

Em termos simples: a primeira página que entrou é a primeira que sai (First-In, First-Out).


- Como seleciona a página vítima?
Quando ocorre um page fault e não há mais frames livres, o simulador chama a função:
int fifo_substituir()
Essa função devolve o índice do próximo frame a ser substituído, usando o valor atual de fifo_index.
Após escolher o frame vítima, o índice é incrementado:
fifo_index = (fifo_index + 1) % num_frames;
Isso cria um comportamento circular, garantindo que, após atingir o último frame, o algoritmo volte ao primeiro.

- Quais passos executa ao substituir uma página?

**Não cole código aqui.** Explique a lógica em linguagem natural.

### 2.4 Algoritmo Clock

Explique **como** implementou a lógica Clock:

- Como gerencia o ponteiro circular?
- Como implementou a "segunda chance"?
- Como trata o caso onde todas as páginas têm R=1?
- Como garante que o R-bit é setado em todo acesso?

**Não cole código aqui.** Explique a lógica em linguagem natural.

### 2.5 Tratamento de Page Fault

Explique como seu código distingue e trata os dois cenários:

**Cenário 1: Frame livre disponível**
- Como identifica que há frame livre?
- Quais passos executa para alocar a página?

**Cenário 2: Memória cheia (substituição)**
- Como identifica que a memória está cheia?
- Como decide qual algoritmo usar (FIFO vs Clock)?
- Quais passos executa para substituir uma página?

---

## 3. Análise Comparativa FIFO vs Clock

### 3.1 Resultados dos Testes

Preencha a tabela abaixo com os resultados de pelo menos 3 testes diferentes:

| Descrição do Teste | Total de Acessos | Page Faults FIFO | Page Faults Clock | Diferença |
|-------------------|------------------|------------------|-------------------|-----------|
| Teste 1 - Básico  |                  |                  |                   |           |
| Teste 2 - Memória Pequena |          |                  |                   |           |
| Teste 3 - Simples |                  |                  |                   |           |
| Teste Próprio 1   |                  |                  |                   |           |

### 3.2 Análise

Com base nos resultados acima, responda:

1. **Qual algoritmo teve melhor desempenho (menos page faults)?**

2. **Por que você acha que isso aconteceu?** Considere:
   - Como cada algoritmo escolhe a vítima
   - O papel do R-bit no Clock
   - O padrão de acesso dos testes

3. **Em que situações Clock é melhor que FIFO?**
   - Dê exemplos de padrões de acesso onde Clock se beneficia

4. **Houve casos onde FIFO e Clock tiveram o mesmo resultado?**
   - Por que isso aconteceu?

5. **Qual algoritmo você escolheria para um sistema real e por quê?**

---

## 4. Desafios e Aprendizados

### 4.1 Maior Desafio Técnico

Descreva o maior desafio técnico que seu grupo enfrentou durante a implementação:

- Qual foi o problema?
- Como identificaram o problema?
- Como resolveram?
- O que aprenderam com isso?

### 4.2 Principal Aprendizado

Descreva o principal aprendizado sobre gerenciamento de memória que vocês tiveram com este projeto:

- O que vocês não entendiam bem antes e agora entendem?
- Como este projeto mudou sua compreensão de memória virtual?
- Que conceito das aulas ficou mais claro após a implementação?

---

## 5. Vídeo de Demonstração

**Link do vídeo:** [Insira aqui o link para YouTube, Google Drive, etc.]

### Conteúdo do vídeo:

Confirme que o vídeo contém:

- [ ] Demonstração da compilação do projeto
- [ ] Execução do simulador com algoritmo FIFO
- [ ] Execução do simulador com algoritmo Clock
- [ ] Explicação da saída produzida
- [ ] Comparação dos resultados FIFO vs Clock
- [ ] Breve explicação de uma decisão de design importante

---

## Checklist de Entrega

Antes de submeter, verifique:

- [ ] Código compila sem erros conforme instruções da seção 1.1
- [ ] Simulador funciona corretamente com FIFO
- [ ] Simulador funciona corretamente com Clock
- [ ] Formato de saída segue EXATAMENTE a especificação do ENUNCIADO.md
- [ ] Testamos com os casos fornecidos em tests/
- [ ] Todas as seções deste relatório foram preenchidas
- [ ] Análise comparativa foi realizada com dados reais
- [ ] Vídeo de demonstração foi gravado e link está funcionando
- [ ] Todos os integrantes participaram e concordam com a submissão

---
## Referências
Liste aqui quaisquer referências que utilizaram para auxiliar na implementação (livros, artigos, sites, **links para conversas com IAs.**)


---

## Comentários Finais

Use este espaço para quaisquer observações adicionais que julguem relevantes (opcional).

---
