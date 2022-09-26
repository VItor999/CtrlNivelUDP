
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <SDL/SDL.h>
#include <math.h>
#include <time.h>

#define BUFFSIZE 32
#define pi 3.141592653

#define SCREEN_W 1000
#define SCREEN_H 500

int startP = 0;
int startG = 0;

pthread_mutex_t mtxDelta = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtxLevel = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtxMax = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtxStartG = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtxStartP = PTHREAD_MUTEX_INITIALIZER;

struct model_Planta{

	int inicioPlanta;     //Ínicio da simulação da planta e plotagem dos gráficos

	long int tempoSimulacao;    //Tempo de simulação transcorrido
	int aux;
	int max;
    int startG;
    int startP;
	float level;
	float angulo_Entrada;
	float angulo_Saida;
	float deltaValvula;


};

//funções auxiliares serverComm////////////////////////////////////
void Die( char *mess) { perror(mess); exit(1); }

void Smsg(int sock, char *buffer, struct sockaddr_in echoclient){
  if (sendto(sock, buffer, strlen(buffer), 0,
	     (struct sockaddr *) &echoclient,
	     sizeof(echoclient)) != strlen(buffer)) {
    Die("Mismatch in number of echo'd bytes");
  }
}
////////////////////////////////////////////////////////////////////
//funções auxiliares grafico////////////////////////////////////////


//#define BPP 8
//typedef Uint8 PixelType;
//#define BPP 16
//typedef Uint16 PixelType;
#define BPP 32
typedef Uint32 PixelType;


typedef struct canvas {
  SDL_Surface *canvas;
  int Height; // canvas height
  int Width;  // canvas width
  int Xoffset; // X off set, in canvas pixels
  int Yoffset; // Y off set, in canvas pixels
  int Xext; // X extra width
  int Yext; // Y extra height
  double Xmax;
  double Ymax;
  double Xstep; // half a distance between X pixels in 'Xmax' scale

  PixelType *zpixel;

} Tcanvas;

typedef struct dataholder {
  Tcanvas *canvas;
  double   Tcurrent;
  double   Lcurrent;
  PixelType Lcolor;
  double   INcurrent;
  PixelType INcolor;
  double   OUTcurrent;
  PixelType OUTcolor;

} Tdataholder;

inline void c_pixeldraw(Tcanvas *canvas, int x, int y, PixelType color)
{
  *( ((PixelType*)canvas->canvas->pixels) + ((-y+canvas->Yoffset) * canvas->canvas->w + x+ canvas->Xoffset)) = color;
}

inline void c_hlinedraw(Tcanvas *canvas, int xstep, int y, PixelType color)
{
  int offset =  (-y+canvas->Yoffset) * canvas->canvas->w;
  int x;

  for (x = 0; x< canvas->Width+canvas->Xoffset ; x+=xstep) {
        *( ((PixelType*)canvas->canvas->pixels) + (offset + x)) = color;
  }
}

inline void c_vlinedraw(Tcanvas *canvas, int x, int ystep, PixelType color)
{
  int offset = x+canvas->Xoffset;
  int y;
  int Ystep = ystep*canvas->canvas->w;

  for (y = 0; y< canvas->Height+canvas->Yext ; y+=ystep) {
    *( ((PixelType*)canvas->canvas->pixels) + (offset + y*canvas->canvas->w)) = color;
  }
}


inline void c_linedraw(Tcanvas *canvas, double x0, double y0, double x1, double y1, PixelType color) {
  double x;

  for (x=x0; x<=x1; x+=canvas->Xstep) {
    c_pixeldraw(canvas, (int)(x*canvas->Width/canvas->Xmax+0.5), (int)((double)canvas->Height/canvas->Ymax*(y1*(x1-x)+y1*(x-x0))/(x1-x0)+0.5),color);
  }
}


Tcanvas *c_open(int Width, int Height, double Xmax, double Ymax)
{
  int x,y;
  Tcanvas *canvas;
  canvas = malloc(sizeof(Tcanvas));

  canvas->Xoffset = 10;
  canvas->Yoffset = Height;

  canvas->Xext = 10;
  canvas->Yext = 10;



  canvas->Height = Height;
  canvas->Width  = Width;
  canvas->Xmax   = Xmax;
  canvas->Ymax   = Ymax;

  canvas->Xstep  = Xmax/(double)Width/2;

  //  canvas->zpixel = (PixelType *)canvas->canvas->pixels +(Height-1)*canvas->canvas->w;

  SDL_Init(SDL_INIT_VIDEO); //SDL init
  canvas->canvas = SDL_SetVideoMode(canvas->Width+canvas->Xext, canvas->Height+canvas->Yext, BPP, SDL_SWSURFACE);

  c_hlinedraw(canvas, 1, 0, (PixelType) SDL_MapRGB(canvas->canvas->format,  255, 255,  255));
  for (y=10;y<Ymax;y+=10) {
    c_hlinedraw(canvas, 3, y*Height/Ymax , (PixelType) SDL_MapRGB(canvas->canvas->format,  220, 220,  220));
  }
  c_vlinedraw(canvas, 0, 1, (PixelType) SDL_MapRGB(canvas->canvas->format,  255, 255,  255));
  for (x=10;x<Xmax;x+=10) {
    c_vlinedraw(canvas, x*Width/Xmax, 3, (PixelType) SDL_MapRGB(canvas->canvas->format,  220, 220,  220));
  }

  return canvas;
}



Tdataholder *datainit(int Width, int Height, double Xmax, double Ymax, double Lcurrent, double INcurrent, double OUTcurrent) {
  Tdataholder *data = malloc(sizeof(Tdataholder));


  data->canvas=c_open(Width, Height, Xmax, Ymax);
  data->Tcurrent=0;
  data->Lcurrent=Lcurrent;
  data->Lcolor= (PixelType) SDL_MapRGB(data->canvas->canvas->format,  255, 180,  0);
  data->INcurrent=INcurrent;
  data->INcolor=(PixelType) SDL_MapRGB(data->canvas->canvas->format,  180, 255,  0);
  data->OUTcurrent=OUTcurrent;
  data->OUTcolor=(PixelType) SDL_MapRGB(data->canvas->canvas->format,  0, 180,  255);


  return data;
}

void setdatacolors(Tdataholder *data, PixelType Lcolor, PixelType INcolor, PixelType OUTcolor) {
  data->Lcolor=Lcolor;
  data->INcolor=INcolor;
  data->OUTcolor=OUTcolor;
}




void datadraw(Tdataholder *data, double time, double level, double inangle, double outangle) {
  c_linedraw(data->canvas,data->Tcurrent,data->Lcurrent,time,level,data->Lcolor);
  c_linedraw(data->canvas,data->Tcurrent,data->INcurrent,time,inangle,data->INcolor);
  c_linedraw(data->canvas,data->Tcurrent,data->OUTcurrent,time,outangle,data->OUTcolor);
  data->Tcurrent = time;
  data->Lcurrent = level;
  data->INcurrent = inangle;
  data->OUTcurrent = outangle;

  SDL_Flip(data->canvas->canvas);
}

void quitevent() {
  SDL_Event event;

  while(SDL_PollEvent(&event)) {
    if(event.type == SDL_QUIT) {
      // close files, etc...

      SDL_Quit();
      exit(1); // this will terminate all threads !
    }
  }

}
////////////////////////////////////////////////////////////////////
//função que controla tempo de chamada
void delay(int texec)
{
  long int elapsed = 0;//tempo relativo

  struct timespec antes;//inicio da contagem
  struct timespec depois;//fim da contagem

  clock_gettime(CLOCK_MONOTONIC_RAW,&antes);//tempo inicial
  clock_gettime(CLOCK_MONOTONIC_RAW,&depois);

  while(elapsed < texec*1000) //teste para ver se estourou o tempo
    {

      if((depois.tv_sec - antes.tv_sec) > 0)//Comparação caso o segundo seja diferente
	{
	  elapsed = (((depois.tv_nsec/1000)+1000000) - antes.tv_nsec/1000);//faz (1s + tdepois)-tantes
	}
      else//comparação padrão
	{
	  elapsed =  ((depois.tv_nsec - antes.tv_nsec)/1000);//faz (tdepois - tantes)
	}

      clock_gettime(CLOCK_MONOTONIC_RAW,&depois); //atualização fim da contagem
    }//end while

}

long texec(struct timespec antes)
{
  long int elapsed;
  struct timespec depois;//fim da contagem
  clock_gettime(CLOCK_MONOTONIC_RAW,&depois);

  if((depois.tv_sec - antes.tv_sec) > 0)//Comparação caso o segundo seja diferente
    {
      elapsed = (((depois.tv_nsec/1000)+1000000) - antes.tv_nsec/1000);//faz (1s + tdepois)-tantes
    }
  else//comparação padrão
    {
      elapsed =  ((depois.tv_nsec - antes.tv_nsec)/1000);//faz (tdepois - tantes)
    }

  return elapsed;
}

////////////////////////////////////////////////////////////////////

//exibição gráfica
void* graphS(void* args)
{

  struct model_Planta *Planta;
  Planta = (struct model_Planta*)args;
  int startaux = 0;
  Tdataholder *data;


  while(1)
  {
    if(startG == 1)
    {
      startaux = 1;
      pthread_mutex_lock( &mtxStartG );
      startG = 0;
      pthread_mutex_unlock( &mtxStartG );
      Planta->startG = 0;//reseta variável de start
      data = datainit(1000,500,150,120,(double)40,(double)0,(double)0);

    }
    else if(startaux == 1)
    {
      //printf("teste grafico \n");
      datadraw(data,(double)Planta->tempoSimulacao/1000.0,(double)Planta->level*100,(double)Planta->angulo_Entrada,(double)Planta->angulo_Saida);
      quitevent();
    }

    delay(50);
  }//end while(1)

}
//simulação da planta

double att_Angulo_Saida(long int tempoSimulacao){

    //double aux = 0;     //Auxilia no cálculo do angulo de saída e no retorno da função

    if(tempoSimulacao < 0){
        return 50;
    }

    if(tempoSimulacao < 20000){

        return (50 + (tempoSimulacao/400));

    }

    if(tempoSimulacao < 30000){

        return 100;

    }

    if(tempoSimulacao < 50000){

        return (100 - ((tempoSimulacao-30000)/250));

    }

    if(tempoSimulacao < 70000){

        return (20 + ((tempoSimulacao - 50000)/1000));

    }

    if(tempoSimulacao < 100000){

        return (40 + 20*cos((tempoSimulacao - 70000)*2*pi/10000));

    }


    return 100;
}

void* func_planta(void* args)
{

  int startaux = 0;

  long int elapsed = 0;
  struct timespec tinicio;

  struct model_Planta *Planta;
  Planta = (struct model_Planta*)args;

  long int dT = 10;             //período da simulacao ms?
  int MAX;                    //Máximo

  float value;            //Valor de controle abertura/fechamento
  float level;         //Nivel do tanque
  float delta=0;          //delta abrir/fechar

  float entrada;         //Valor atual do angulo da valvula de entrada
  float influx;         //Fluxo de liquido entrando
  float outflux;         //Fluxo de liquido saindo

  //long int bgn,end;      //variaveis para controlar o tempo do laco

   entrada = 0;            //zerando
  (*Planta).level = 0.4;
  (*Planta).angulo_Saida = 0;

  while(1) //loop infinito da planta
  {
    clock_gettime(CLOCK_MONOTONIC_RAW,&tinicio);//atualiza inicio para comparação

    if(startP == 1)
    {
      //printf("teste Start \n");
      startaux = 1;
      pthread_mutex_lock( &mtxStartP );
      startP = 0;
      pthread_mutex_unlock( &mtxStartP );
     //reseta variável de start

      //Resetar var da planta
      Planta->startP = 0;
      Planta->aux = 5;
      Planta->tempoSimulacao = 0;
      Planta->angulo_Saida = 0;
      Planta->angulo_Entrada = 50;
      pthread_mutex_lock( &mtxDelta );
      Planta->deltaValvula = 0;
      pthread_mutex_unlock( &mtxDelta );

      Planta->level = 0.4;
      MAX = Planta->max;

    }
    else if(startaux == 1)
    {
      //printf("teste planta \n");
      pthread_mutex_lock( &mtxDelta );
	  value = Planta->deltaValvula;
      pthread_mutex_unlock( &mtxDelta );

      level = Planta->level;
      entrada = Planta->angulo_Entrada;

      if(value != 0)
	{
	  delta += value;
	  pthread_mutex_lock( &mtxDelta );
	  Planta->deltaValvula = 0;
	  pthread_mutex_unlock( &mtxDelta );
	}

      if (delta > 0)  //Abrindo
	{
	  if(delta < 0.01*dT)
	    {
	      entrada += delta;
	      delta = 0;
	    }
	  else
	    {
	      entrada = entrada + 0.01*dT;
	      delta -= 0.01*dT;
	    }
	}
      else if(delta < 0) //Fechando
	{
	  if(delta > -0.01*dT)
	    {
	      entrada += delta;
	      delta = 0;
	    }
	  else
	    {
	      entrada = entrada - 0.01*dT;
	      delta += 0.01*dT;
	    }
	}

      influx = 1*sin((pi/2)*(entrada/100));    //Fluxo de entrada
      outflux = (MAX/100.0)*((level/1.25)+0.2)*(sin((pi/2)*(att_Angulo_Saida(Planta->tempoSimulacao))/100));  //Fluxo de saída

      level = level + 0.00002*dT*(influx - outflux);//Nivel atual
      //printf("%f \n", level);
      //printf("%f \n", influx);
      //printf("%f \n", outflux);


      if(level > 1)   //Limite maximo de nivel do liquido
	{
	  level = 1;
	}
      else if(level < 0)   //Limite minimo de nivel do liquido
	{
	  level = 0;
	}

      /*if(entrada > 100)    //Limite maximo do angulo de entrada da valvula
	{
	  entrada = 100;
	}
      else if(entrada < 0) //Limite minimo do angulo de entrada da valvula
	{
	  entrada = 0;
	  }*/

	//Atualiza os membros da estrutura de controle da planta
      Planta->tempoSimulacao += dT;
      pthread_mutex_lock( &mtxLevel );
      Planta->level = level;
      pthread_mutex_unlock( &mtxLevel );
      Planta->angulo_Entrada = entrada;
      Planta->angulo_Saida = att_Angulo_Saida(Planta->tempoSimulacao);

    }//end else if


    //delay(10); //chama delay de 10ms
    while(elapsed < dT*1000)//trava 10ms
	{
	  elapsed = texec(tinicio);
	}
    dT = elapsed/1000;
    //printf("%d \n", dT);
    elapsed = 0;
  }//end while(1)

}
//comunicação servidor///////////////////////////////////////////

void serverComm(int sock, struct sockaddr_in echoclient, struct model_Planta *Planta)
{
  char buffer[BUFFSIZE];
  unsigned int echolen, clientlen;
  int received = 0;
  float valnvl=0;
  int valint=0;

  //VETORES QUE SEPARAM A INFORMAÇÃO VINDA DO CLIENTE
  char com[BUFFSIZE];
  char val[BUFFSIZE];
  int valor;
  int i = 0; int j = 0; //auxiliares pra percorrer o buffer
/* Run until cancelled */
  while (1) {
    i=0;
    j=0;
    /* Receive a message from the client */
    clientlen = sizeof(echoclient);
    if ((received = recvfrom(sock, buffer, BUFFSIZE, 0,
			     (struct sockaddr *) &echoclient,
			     &clientlen)) < 0) {
      Die("Failed to receive message");
    }
    fprintf(stderr,
	    "Client connected: %s\n", inet_ntoa(echoclient.sin_addr));
    //MENSAGEM DE ERRO QUANDO NÃO RECEBE '!'
    if(strchr(buffer, '!') == NULL){
      char buffret[BUFFSIZE] = "Err!";
      Smsg(sock, buffret, echoclient);
    }

    else{
      //MENSAGENS SEM '#'
      if(strchr(buffer, '#') == NULL){
	//Varredura
	while(buffer[j] != '!') {
	  com[j] = buffer[j];
	  j++;
	}
	com[j] = '\0';
	if(strcmp(com,"CommTest")==0){
	  //montagem string de retorno
	  char buffret[BUFFSIZE] = "Comm#OK!";
	  Smsg(sock, buffret, echoclient);
	}
	else if(strcmp(com,"Start")==0){
	  //montagem string de retorno
	  char buffret[BUFFSIZE] = "Start#OK!";
	  pthread_mutex_lock( &mtxStartG );
	  startP = 1;
	  pthread_mutex_unlock( &mtxStartG );
	  pthread_mutex_lock( &mtxStartP );
	  startG = 1;
	  pthread_mutex_unlock( &mtxStartP );
	  Planta->startP = 1;//Aciona chave de inicio da simulação da planta
	  Planta->startG = 1;//Aciona chave de inicio da simulação do gráfico
	  Smsg(sock, buffret, echoclient);
	}
	else if(strcmp(com,"GetLevel")==0){

	  valnvl = 100 * (Planta->level);

          valint = (int)valnvl;
          sprintf(val,"%d",valint);

	  //montagem string de retorno
	  char buffret[BUFFSIZE] = "Level#";
	  strcat(buffret, val);
	  strcat(buffret, "!");
	  Smsg(sock, buffret, echoclient);
	}
	else {
	  char buffret[BUFFSIZE] = "Err!";
	  Smsg(sock, buffret, echoclient);
	}
      }//endif #
      //MENSAGEM COM # e !
      else {
	//Varredura
	while(buffer[j] != '#') {
	  com[j] = buffer[j];
	  j++;
	}
	com[j] = '\0';

	j++;
	i=j;

	while(buffer[j] != '!') {
	  val[j-i] = buffer[j];
	  j++;
	}
	val[j-i]='\0'; //coloca fim na string
	valor = atof(val); //transforma o valor em número


	//Compara com os códigos disponíveis
	if(strcmp(com,"OpenValve")==0){

	  if(valor > 100)
            {
	      valor = 100;
	      }
	  pthread_mutex_lock( &mtxDelta );
	  Planta->deltaValvula = valor;
	  pthread_mutex_unlock( &mtxDelta );

	  //montagem string de retorno
	  char buffret[BUFFSIZE] = "Open#";
	  strcat(buffret, val);
	  strcat(buffret, "!");
	  Smsg(sock, buffret, echoclient);
	}
	else if(strcmp(com,"CloseValve")==0){

	  if(valor > 100)
            {
	      valor = 100;
            }
	  pthread_mutex_lock( &mtxDelta );
	  Planta->deltaValvula = -1*valor;
	  pthread_mutex_unlock( &mtxDelta );

	  //montagem string de retorno
	  char buffret[BUFFSIZE] = "Close#";
	  strcat(buffret, val);
	  strcat(buffret, "!");
	  Smsg(sock, buffret, echoclient);
	}

	else if(strcmp(com,"SetMax")==0){
	  pthread_mutex_lock( &mtxMax );
	  Planta->max = valor;
	  pthread_mutex_unlock( &mtxMax );

	  //montagem string de retorno
	  char buffret[BUFFSIZE] = "Max#";
	  strcat(buffret, val);
	  strcat(buffret, "!");
	  Smsg(sock, buffret, echoclient);
	}
	else {
	  char buffret[BUFFSIZE] = "Err!";
	  printf("Erro#!");
	  Smsg(sock, buffret, echoclient);
	}
      }//end else

      //Limpa buffer
      j=0;
      while(buffer[j] != '\0') {
	buffer[j] = '\0';
	j++;
      }

    }//end else
    delay(10);
  }//end while(1)
}
  ////////////////////////////////////////////////////////////////

//****************************************************************************
//****************************************************************************
int main(int argc, char *argv[])
{
  int sock;
  struct sockaddr_in echoserver;
  struct sockaddr_in echoclient;
  unsigned int serverlen;

  pthread_t tgraph, tplant;
  int thr1, thr2;

  struct model_Planta Planta;

  Planta.startP = 0;
  Planta.startG = 0;
  Planta.angulo_Saida = 0;
  Planta.angulo_Entrada = 0;
  Planta.level = 0.4;
  Planta.aux = 5;
  Planta.tempoSimulacao = 0;
  Planta.max = 0;
  Planta.deltaValvula = 0;

  /////////////////////////////////////////////////////////////////////////
  //Recepção de parâmetros de porta do terminal
  if (argc != 2) {
    fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
    exit(1);
  }

  /* Create the UDP socket */
  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    Die("Failed to create socket");
  }
  /* Construct the server sockaddr_in structure */
  memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
  echoserver.sin_family = AF_INET;                  /* Internet/IP */
  echoserver.sin_addr.s_addr = htonl(INADDR_ANY);   /* Any IP address */
  echoserver.sin_port = htons(atoi(argv[1]));       /* server port */

  /* Bind the socket */
  serverlen = sizeof(echoserver);
  if (bind(sock, (struct sockaddr *) &echoserver, serverlen) < 0) {
    Die("Failed to bind server socket");
  }

////////////////////////////////////////////////////////////////////


/* Create independent threads each of which will execute functionC */
  if( (thr1=pthread_create( &tplant, NULL, &func_planta, &Planta)) )
    {
      printf("Thread creation failed: %d\n", rc1);
    }
  if( (thr2=pthread_create( &tgraph, NULL, &graphS, &Planta)) )
    {
      printf("Thread creation failed: %d\n", rc2);
    }
  serverComm(sock,echoclient, &Planta);

}
