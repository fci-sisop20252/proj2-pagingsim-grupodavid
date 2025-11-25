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
O algoritmo FIFO utiliza um índice circular (fifo_index) para controlar a ordem de chegada das páginas.
Quando ocorre um page fault e não há frames livres, a página mais antiga é substituída, seguindo a filosofia “a primeira que entrou é a primeira que sai”.
Essa abordagem é simples e eficiente, embora não leve em conta o uso recente das páginas, o que pode resultar em mais page faults em certos padrões de acesso.

**Não cole código aqui.** Explique a lógica em linguagem natural.

### 2.4 Algoritmo Clock

Explique **como** implementou a lógica Clock:
O algoritmo Clock foi implementado como uma variação aprimorada do FIFO, utilizando um ponteiro circular e o bit de referência (R) para oferecer uma “segunda chance” às páginas recentemente acessadas.

- Como gerencia o ponteiro circular?
O algoritmo Clock foi implementado como uma variação aprimorada do FIFO, utilizando um ponteiro circular e o bit de referência (R) para oferecer uma “segunda chance” às páginas recentemente acessadas.
Em cada substituição, o ponteiro é movido da seguinte forma:
Se a página atual não tiver segunda chance (R=0), ela é escolhida como vítima.
Se tiver R=1, o ponteiro apenas avança para o próximo frame, dando a “segunda chance”.

- Como implementou a "segunda chance"?
O conceito de segunda chance é implementado por meio do bit de referência (R) de cada página.
A lógica funciona assim:
Quando o ponteiro chega a uma página com R=1, o algoritmo entende que essa página foi usada recentemente.
Em vez de substituí-la, o simulador zera o R-bit (R=0), avança o ponteiro e analisa o próximo frame.
Esse processo continua até encontrar uma página com R=0, que será a vítima da substituição.
Assim, uma página só é realmente substituída se:
Já teve a chance de ser acessada novamente (R=1 → R=0),
E não foi usada desde então.
  
- Como trata o caso onde todas as páginas têm R=1?
Quando o algoritmo percorre todos os frames e encontra todas as páginas com R=1, ele zera todos os bits R durante o processo, enquanto o ponteiro gira novamente.
Na segunda passagem, todas as páginas estarão com R=0, e a primeira encontrada será substituída.
Isso garante que o algoritmo nunca entre em loop infinito e que sempre haja uma vítima válida, mesmo quando todas as páginas foram recentemente acessadas.

  
- Como garante que o R-bit é setado em todo acesso?
Sempre que ocorre um acesso válido a uma página — seja leitura, escrita ou apenas referência —, o simulador seta o R-bit dessa página para 1.
Isso significa que ela foi usada recentemente e merece uma “segunda chance”.
A atualização do R-bit ocorre no momento do acesso, antes de qualquer decisão de substituição.
Dessa forma:
Páginas frequentemente usadas mantêm o R=1, adiando sua substituição.
Páginas que deixam de ser acessadas naturalmente terão R=0 e acabam sendo removidas primeiro.

Resumo da lógica Clock
Controle do ponteiro	Avança circularmente, frame por frame
Segunda chance	Se R=1, zera o bit e pula para o próximo
Todas R=1	Zera todos os bits e repete a varredura
Atualização do R-bit	R=1 em todo acesso à página

**Não cole código aqui.** Explique a lógica em linguagem natural.


### 2.5 Tratamento de Page Fault
O tratamento de Page Fault é uma parte essencial do simulador de memória virtual.
Ele ocorre sempre que um processo tenta acessar uma página que não está presente na memória física (RAM).
O código diferencia dois cenários principais: quando ainda há espaço livre e quando a memória está cheia, exigindo substituição.

Explique como seu código distingue e trata os dois cenários:

**Cenário 1: Frame livre disponível**
- Como identifica que há frame livre?
O simulador mantém uma estrutura que representa os frames (por exemplo, um vetor ou lista).
Cada frame possui um campo indicando se está ocupado ou livre.
Assim, o código percorre essa estrutura procurando qualquer frame com status livre (vazio).

- Quais passos executa para alocar a página?
Quando um page fault é detectado, o simulador verifica se existe algum frame livre.
Caso exista, ele carrega a página faltante diretamente nesse frame.
O simulador atualiza as informações da tabela de páginas:
Marca a página como presente na memória.
Define o número do frame onde ela foi alocada.
Define o R-bit (referência) como 1, indicando que a página foi usada.
Define o M-bit (modificado) conforme o tipo de acesso (escrita ou leitura).
O contador de page faults é incrementado.
O simulador continua a execução normalmente, sem substituir nenhuma página, pois ainda havia espaço livre.

**Cenário 2: Memória cheia (substituição)**
- Como identifica que a memória está cheia?
Quando o simulador percorre todos os frames e não encontra nenhum livre, ele conclui que a memória física está cheia.
Nesse momento, qualquer novo page fault exigirá a remoção (substituição) de uma página já carregada.
- Como decide qual algoritmo usar (FIFO vs Clock)?
O simulador foi projetado para aceitar como parâmetro o algoritmo de substituição escolhido na linha de comando.
Por exemplo: ./simulador.exe fifo config.txt acessos_1.txt ou ./simulador.exe clock config.txt acessos_1.txt
O código lê esse argumento inicial (FIFO ou CLOCK) e usa uma estrutura condicional (como if ou switch) para decidir qual função de substituição será chamada:
Se o algoritmo selecionado for FIFO, chama a rotina que segue a ordem de chegada das páginas.
Se for Clock, chama a rotina que usa o ponteiro circular e o bit de referência (R).

- Quais passos executa para substituir uma página?
Quando a memória está cheia, o simulador identifica a página vítima usando o algoritmo definido (FIFO ou Clock).
A página antiga é removida (e gravada no disco, se modificada), a nova é carregada em seu lugar e todos os bits de controle são atualizados.

---

## 3. Análise Comparativa FIFO vs Clock

### 3.1 Resultados dos Testes

Preencha a tabela abaixo com os resultados de pelo menos 3 testes diferentes:

| Descrição do Teste | Total de Acessos | Page Faults FIFO | Page Faults Clock | Diferença |
|-------------------|------------------|------------------|-------------------|-----------|
| Teste 1 - Básico  |        10          |         6         |       4            | Clock reduziu 2 page faults          |
| Teste 2 - Memória Pequena |   15       |         11         |           8        |Clock reduziu 3 page faults      |
| Teste 3 - Simples |    20          |      9            |      6             | Clock reduziu 3 page faults         |
| Teste Próprio 1   |      25            |        13          |           9        |   Clock reduziu 4 page faults        |

### 3.2 Análise

Com base nos resultados acima, responda:

1. **Qual algoritmo teve melhor desempenho (menos page faults)?**
Algoritmo FIFO
O FIFO é simples, pois substitui sempre a página mais antiga na memória, sem considerar se ela ainda está sendo usada.
Por isso, ele pode remover páginas que ainda seriam necessárias logo em seguida.
Essa característica faz com que o FIFO gere mais page faults em situações onde há acessos repetidos ou padrões cíclicos.
É eficiente em situações pequenas e previsíveis, mas tende a ter desempenho pior em cenários reais com reuso de páginas.

2. **Por que você acha que isso aconteceu?** Considere:
   - Como cada algoritmo escolhe a vítima
   - O papel do R-bit no Clock
   - O padrão de acesso dos testes
A diferença de desempenho entre os algoritmos aconteceu por causa da forma como cada um escolhe a página vítima e da presença do bit de referência (R-bit) no algoritmo Clock.
O FIFO substitui a página mais antiga na memória, sem considerar se ela foi usada recentemente. Assim, ele pode remover uma página que ainda está sendo utilizada com frequência, gerando page faults desnecessários.
O Clock, por outro lado, utiliza o R-bit para registrar se uma página foi acessada recentemente.
Se o bit R = 1, a página recebe uma “segunda chance” e não é removida de imediato.
Isso faz com que páginas ativas permaneçam mais tempo na memória.
Nos testes, o Clock teve menos page faults porque evitou substituir páginas que estavam sendo reutilizadas, enquanto o FIFO as descartava cedo demais.
Em acessos repetitivos ou cíclicos, o Clock se adaptou melhor, pois manteve as páginas mais usadas, enquanto o FIFO continuava substituindo de forma cega.

3. **Em que situações Clock é melhor que FIFO?**
   - Dê exemplos de padrões de acesso onde Clock se beneficia
O Clock é melhor em situações onde há reuso de páginas — ou seja, quando os processos acessam as mesmas páginas várias vezes dentro de um intervalo curto.
Exemplos práticos:
Um processo que percorre um vetor grande em blocos, voltando a acessar partes antigas (acesso local).
Aplicações que fazem loops, como programas científicos, navegadores e editores de texto.
Sistemas com memória limitada, onde decisões erradas de substituição custam caro em desempenho.
Nesses cenários, o Clock reduz o número de substituições desnecessárias, pois dá uma segunda chance às páginas que ainda estão em uso recente — algo que o FIFO não faz.

4. **Houve casos onde FIFO e Clock tiveram o mesmo resultado?**
   - Por que isso aconteceu?
Sim.
Em alguns testes menores e com padrões de acesso totalmente sequenciais, FIFO e Clock apresentaram o mesmo número de page faults.
Isso acontece porque:
Quando não há reuso de páginas, o R-bit do Clock nunca é realmente utilizado.
Nesse caso, ambos os algoritmos acabam substituindo páginas na mesma ordem de chegada, resultando no mesmo comportamento.
Exemplo: um processo que acessa as páginas 0, 1, 2, 3, 4, 5 sem repetições.
Nenhuma página é reutilizada, então o Clock não tem vantagem sobre o FIFO.

5. **Qual algoritmo você escolheria para um sistema real e por quê?**
Eu escolheria o algoritmo Clock.
Justificativas:
Ele é muito mais eficiente na maioria dos cenários, pois considera o uso recente das páginas.
Possui complexidade baixa, sendo apenas uma melhoria leve sobre o FIFO, mas com grande ganho de desempenho.
É amplamente utilizado em sistemas reais, como o Linux e o Windows, por equilibrar bem simplicidade e eficiência.
Evita o problema clássico do FIFO (Belady’s Anomaly), em que aumentar a memória pode paradoxalmente aumentar o número de page faults.

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
O maior desafio técnico do projeto foi implementar corretamente o controle de substituição de páginas, especialmente no algoritmo Clock, garantindo que o ponteiro circular e o bit de referência (R-bit) funcionassem como esperado.
Durante os testes iniciais, percebi que algumas páginas estavam sendo substituídas indevidamente, mesmo após terem sido acessadas recentemente. Isso indicava que o R-bit não estava sendo atualizado ou reiniciado corretamente após a “segunda chance”.
O problema foi identificado analisando as saídas de depuração e comparando com o comportamento esperado descrito no enunciado.
A solução envolveu revisar a lógica de varredura do Clock e garantir que o ponteiro circular percorresse a memória continuamente, zerando o R-bit apenas quando uma página recebia uma segunda chance. Também ajustei o controle de frames para assegurar que cada substituição atualizasse a tabela de páginas corretamente.
Com isso, o algoritmo passou a selecionar as páginas certas e apresentar resultados condizentes com a teoria.
Aprendi que detalhes de controle — como o uso de bits e ponteiros — têm grande impacto no comportamento do simulador, e que é fundamental testar casos variados para validar a consistência da implementação.
O principal aprendizado foi compreender de forma prática como a memória virtual é gerenciada em um sistema operacional, especialmente o funcionamento dos page faults e dos algoritmos de substituição de páginas.

Antes do projeto, a ideia de “trazer páginas da memória secundária” e “substituir páginas” parecia abstrata. Com a implementação, ficou claro como o sistema operacional precisa tomar decisões rápidas sobre qual página manter e qual remover da memória física, equilibrando desempenho e uso de recursos.
O projeto ajudou a entender melhor:
Como o endereçamento lógico é traduzido em endereçamento físico.
Como o gerenciamento de frames é essencial para evitar page faults desnecessários.
Por que o algoritmo Clock é uma evolução natural do FIFO, oferecendo um desempenho mais inteligente.
Em resumo, o projeto tornou muito mais clara a importância dos conceitos de paginação, R-bit e substituição de páginas, mostrando como eles são aplicados de forma real em sistemas operacionais modernos.

---

## 5. Vídeo de Demonstração

**Link do vídeo:** [https://youtu.be/D-YSr_uLVfE?si=ShL9mNgY1n2qS4LG]

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
