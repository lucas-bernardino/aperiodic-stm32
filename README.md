### Escalonador aperiódico e com compartilhamento de recursos em um RTOS utilizando STM32F103

[Objetivos](#objetivos)

[Rate Monotonic (RM)](#rate-monotonic-rm)

[Implementação do RM](#implementação-do-rm)

[Background Server (BS)](#background-server-bs)

[Implementação do BS](#implementação-do-bs)

[Nom-Preemptive Protocol (NPP)](#nom-preemptive-protocol-npp)

[Implementação do NPP](#implementação-do-npp)

[Utilização e Visualização](#utilização-e-visualização)

![BluepillStlink](/assets/bluepill_stlink.jpg)

## Objetivos
Esse repositório contém a implementação de um escalonador para tarefas aperiódicas e algoritmo para compartilhamento de recursos. Ele foi implementado em um sistema de tempo real, mais especificamente o MInimal Real-time Operating System (MiROS). Para as tarefas periódicas, foi utilizado o *Rate Monotonic* (RM) e para tarefas aperiódicas foi utilizado o *Background Server* (BS). Além disso, foi utilizado o *Nom-Preemptive Protocol* (NPP) como algoritmo de compartilhamento de recursos.

## Rate Monotonic (RM)
O algoritmo Rate Monotonic é muito utilizado em sistemas operacionais de tempo real (RTOS) como escalonador para tarefas periódicas. Ele é baseado em prioridades estaticas, de modo que elas são definidas de modo que quanto maior o período de uma tarefa, menor será a sua prioridade. Assim, é garantido que tarefas que devem ser executadas em ciclos menores possuam maior prioridade no sistema.

## Implementação do RM
Para implementar o RM, foi necessário alterar a função *OSThread_start*, que agora passa a receber o custo e o período da tarefa, denotados como *Ci* e *Ti*, respectivamente. Além disso, a *struct* da *OSThread* também foi modificada, de modo que foram adicionados o campo de *Ci* e *Ti*, assim como o tempo restante da tarefa, *remainingTime* e uma flag para verificar se a tarefa está ativa ou não, chamada *isActive*.

A maior mudança ocorreu na *OS_sched()*. Como dito anteriormente, o RM julga as tarefas de acordo com seu período, onde o menor período possuirá a maior prioridade. Assim, na *OS_sched()* primeiramente é chamada uma função auxiliar, *checkLowestPriorityThread()* que verifica qual tarefa possui a menor prioridade para atribuir a maior prioridade. Após isso, é verificado quais tarefas já foram completadas e por fim verifica-se qual deve ser a próxima tarefa a ser executada.

Na *main.c* também é chamada a função *TaskAction()*, que basicamente representa e simula a tarefa em execução.

## Background Server (BS)
Em um sistema como tarefas periódicas e aperiódicas, foi assumido que as tarefas periódicas respeitarão o escalonamento por RM. Para as tarefas aperódicas, utilizou-se o Background Server, que funciona de uma maneira relativamente simples: quando não há nenhuma tarefa periódica sendo executada, o escalonador deve executar a fila de tarefas aperiódicas. Ou seja, quando não há tarefas periódicas, as tarefas aperiódicas são escolhidas de modo que aquelas que chegaram primeiro possuem a maior prioridade na fila.

## Implementação do BS
Foi adicionado uma *struct* para tarefas aperiódicas, contendo os campos de tempo de chegada, custo e um ponteiro para a função a ser executada. Na *main.c*, o mecanismo de adição de tarefas aperiódicas se dá pela função *addAperiodicTask*, a qual deve receber os campos da struct mencionada para adicionar uma nova tarefa aperiódica na fila de tarefas aperiódicas. Além disso, será feita uma ordenação nessa fila por ordem de chegada. 

Na função *OS_sched*, caso tarefas periódicas não estejam sendo executadas, é chamada a função *executeAperiodicTasks*, que irá percorrer o *array* das tarefas aperiódicas e realizar a lógica para executá-las caso seja possível e ainda há custo restante nelas. Quando a exeucação é finalizada, o tempo de chegada será setado para o "infinito", para que ela não possa ser executada novamente.

## Nom-Preemptive Protocol (NPP)
Em sistemas multitarefas, geralmente se trabalha com exclusão mutua, de modo que existem alguns protocolos para garantir a exclusão mutua dos recursos a serem compartilhados. Essa parte do código é chamada de seção crítica e deve ser protegida por semáforos. 

O Nom-Preemptive Protocol (NPP) aumenta a prioridade de uma tarefa quando ela entra em uma seção crítica, de modo que ela recebe a maior prioridade do sistema a fim de que ela não sofra preempção. A sua prioridade deve voltar ao normal após sair da seção crítica. 

## Implementação do NPP
Para implementação do NPP, foi utilizado o semáforo, implementado em um trabalho anterior. Foi necessário modificar as funções de *sem_wait* e *sem_post* para respeitar o NPP. 

Criou-se um novo campo na struct da *OSThread*, chamado *startupTi*, que guarda o período original da tarefa. Como o período de uma tarefa está diretamente relacionado à sua prioridade no RM, esse campo será utilizado para manipular a prioridade da tarefa e seguir o NPP. Além disso, foi adicionado uma função chamad *getLowestPeriod*, que irá percorrer o *array* de tarefas periódicas e salvar o menor período (que será a maior prioridade) em uma variável chamada *lowestPeriodTask*.

Na função *sem_wait*, agora é necessário passar como parâmetro qual tarefa está chamando essa função, para que ela possa ser elevada como maior prioridade, com o auxílio da variável *lowestPeriodTask*.

Na função *sem_post*, a tarefa passada como parâmetro volta a ter seu período e, consequentemente, sua prioridade original, com o auxílio do campo *startupTi*. 

## Utilização e Visualização
Na STM32CubeIde, execute o código em modo *debug*. 

Após iniciar nesse modo, selecione a visualização das seguintes variáveis para terem os seguintes propósitos:
Variável                 | Propósito |
----                     |  ----                                                    |
task1Visualizer          | Visualizar ticks da tarefa periódica 1                   |
task2Visualizer          | Visualizar ticks da tarefa periódica 2                   |
task3Visualizer          | Visualizar ticks da tarefa periódica 3                   |
aperiodicExecution       | Visualizar ticks da tarefa aperiódica 1                  |
aperiodicExecution2      | Visualizar ticks da tarefa aperiódica 2                  |
resource                 | Visualizar recurso compartilhado entre tarefa 1 tarefa 3 |
counter                  | Visualizar variável de espera na tarefa 3                |


Na *main.c*, estão representadas três tarefas periódicas, com os seguintes custo de executação e período:
|    | Ci | Ti |
|----|----|----|
| T1 | 3  | 5  |
| T2 | 1  | 8  |
| T3 | 1  | 10 |

Para verificar se esse conjunto de tarefas é escalonável pelo Rate Monotonic, podemos utilizar o teste de Liu and Layland:

$\sum_{i=1}^{n}\frac{Ci}{Ti} \le n\left( 2^{\frac{1}{n}}-1 \right)$ 

Onde $n$ representa o número de tarefas, $Ci$ o custo de execucação e $Ti$ o período da tarefa.

Com o conjunto de tarefas acima, é possível verificar que o teste de Liu and Layland falha, visto que U = $\frac{3}{5} + \frac{1}{8} + \frac{1}{10} = 0.825$ e $Ub = 3\left( 2^{\frac{1}{3}}-1 \right) = 0.779$. Assim, não é possível afirmar se o sistema é escalonável por RM.

Pode-se utilizar o teste hiperbólico, dado por $\prod_{i=i}^{n}(Ui + 1) \le 2 $. Como $\prod_{i=i}^{n}(Ui + 1) = 1.98$ para esse conjunto de tarefas, o sistema passa no teste hiperbólico e, portanto, pode-se concluir que ele é escalonável por RM.

Assim, ao executar o código em modo Debug e analisando as variáveis *task1Visualizer*, *task2Visualizer* e *task3Visualizer*, pode-se perceber que as tarefas são executadas exatamente como mostrado na seguinte figura:

![Escalonamento por RM](/assets/rmscheduling.png)


Ao executar o código com as tarefas aperiódicas, é possível verificar que elas serão executadas entre os *gaps* de tarefas periódicas. Ou seja, a tarefa aperiódica 1 será executada entre o tempo 9 e 10, e a tarefa aperiódica 2 será executada entre o tempo 14 e 15.

Ao executar o código com o compartilhamento de recursos, é possível visualizar a variável *resource* sendo manipulada pela tarefa 1 e tarefa 3, de modo que não ocorre mais preempção quando estão nas zonas críticas. A tarefa 1 é responsável por adicionar 5 unidades nessa variável, enquanto a tarefa 3 é responsável por tirar 5 unidades dela