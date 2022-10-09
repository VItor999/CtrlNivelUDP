# Controle de Nível via UDP

## Descrição
Esse repositório contem o trabalho final da disciplinha ENG10048 - Protocolos de Comunicação - UFRGS cursada em 2022/1.
O trabalho foi proposto pelo professor Alceu Heinke Frigeri e desenvolvido pelos alunos Lucas Esteves e Vitor Carvalho.
Para mais detalhes, verifique o *.pdf* que contém a descrição do trabalho.

## Requisitos para compilação:

- POSIX
- SLD 1.2 (opcional)
- GCC >= 4.3

A biblioteca SDL 1.2 é necessária somente para executar o programa com a parte gráfica.

## Compilação 

### Completa

1. Navegue para o diretorio *main* e execute  o script *comp.sh*
2. Inicie o servidor rodando o comando `./server <porta>`
3. Inicie o cliente rodando o comando `./client <serverIP> <porta>`
4. Encerre a janela gráfica do cliente para encerrar ambos os programas simultâneamente

#### Problemas comuns 
- Caso seja necessário, habilite as permissões para o *script* executando o comando: `chmod +x bash.sh`
- Caso a biblioteca SDL 1.2 não esteja sendo encontrada, ela pode ser facilmente instalada em distribuições que utilizam o gerenciador de pacotes **apt** utilizando o seguinte comando: `sudo apt install libsdl1.2-dev`

### Sem gráfico
1. Navegue para o diretorio *main*
2. Abra os arquivos *server.c* e *client.c*
3. Comente, em cada um dos arquivos, o comando `#define GRAPH 1` (linha **11** em ambos os arquivos) utilizando `//`
4. Compile o servidor utilizando os seguintes comandos: `gcc server.c -o server -pthread -lm -lrt`
5. Compile o cliente utilizando os seguintes comandos: `gcc client.c -o client -pthread -lm -lrt`
6. Inicie o servidor rodando o comando `./server <porta>`
7. Inicie o cliente rodando o comando `./client <serverIP> <porta>`
8. Pressione ESC no cliente para encerar ambos os programas simultâneamente
