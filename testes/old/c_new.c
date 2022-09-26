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
#include <sys/poll.h>

#define BUFFSIZE 32
#define pi 3.141592653
#define ki 0.008
#define kp 800
#define kd 2
#define ref 0.8

float plot[3] = {0,0,0};

int startG = 0;
int posV = 50;
int erro = 0;
int rv;
struct pollfd ufds[1];


//////////////////////////////////função que controla tempo de chamada//////////////////////////////////////////

void espera(int texec)
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////funções auxiliares grafico////////////////////////////////////////////////////////////////////////////////


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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////graph/////////////////////////////////////////////////////////////////////////////////////////////
void* graphS(void* args)
{
  
  int startaux = 0;
  Tdataholder *data;

  
  while(1)
  {
    if(startG == 1)
    {
      startaux = 1;     
      startG = 0;
      
      data = datainit(1000,500,150,120,(double)40,(double)0,(double)0);
      
    }
    else if(startaux == 1)
    {
       
      datadraw(data,(double)plot[0]/1000.0,(double)plot[1],(double)plot[2],(double)0);
      quitevent();   
    }

    espera(200);
  }//end while(1)
  
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////comunicação////////////////////////////////////////////////////////////////////////////////////

void Die(char *mess) { perror(mess); exit(1); }

void Smsg(int sock, char *buffer, struct sockaddr_in echoserver){
  erro = 0;
  if (sendto(sock, buffer, strlen(buffer), 0,
	     (struct sockaddr *) &echoserver,
	     sizeof(echoserver)) != strlen(buffer)) {
    Die("Mismatch in number of sent bytes");
  }
}

void Rmsg(int sock, char *buffer, struct sockaddr_in echoclient, struct sockaddr_in echoserver){
  int received = 0;
  unsigned int clientlen;

  /*ufds[0].fd = sock;
  ufds[0].events = POLLIN | POLLOUT;
  rv = poll(ufds, 1, 1500);
  if(rv == -1){
    perror("poll");
  } else if(rv == 0){
    printf("Timeout ocurred \n");
    } else{
  /* Receive the word back from the server */
  fprintf(stdout, "Received: ");
  clientlen = sizeof(echoclient);
  
  if ((received = recvfrom(sock, buffer, BUFFSIZE, 0/*MSG_DONTWAIT*/,
			   (struct sockaddr *) &echoclient,
			   &clientlen)) < 0/*BUFFSIZE*//*strlen(buffer)*/) {
    //Die("Mismatch in number of received bytes");
    printf("Erro na recepção da resposta do servidor \n");
    erro = 1;
  }
  /* Check that client and server are using same socket */
  if (echoserver.sin_addr.s_addr != echoclient.sin_addr.s_addr) {
    //Die("Received a packet from an unexpected server");
    printf("Received a packet from an unexpected server \n");
    erro = 1;
  }
  //if(erro == 0){
    buffer[received] = '\0';        /* Assure null terminated string */
    fprintf(stdout, buffer);
    fprintf(stdout, "\n");
    // }
}//}

float getNum(char msg[BUFFSIZE])
{
  int j = 0;
  int i = 0;
  float valor = 0;
  char val[10];

  while(msg[j] != '#')
  {
    j++;
  }

  j++;
  i = j;

  while(msg[j] != '!')
  {
    val[j-i] = msg[j];
    j++;
  }

  val[j-i] = '\0';
  valor = atof(val);

  return valor;

}

void openv(float numero, char *comando)
{
    int x;
    x=(int)numero;
    char aux[4];
    sprintf(comando,"OpenValve#");
    sprintf(aux,"%d",x);
    strcat(comando,aux);
    strcat(comando,"!");
}

void closev(float numero, char *comando)
{
    int x;
    x=(int)numero;
    char aux[4];
    sprintf(comando,"CloseValve#");
    sprintf(aux,"%d",x);
    strcat(comando,aux);
    strcat(comando,"!");
}   

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////MAIN//////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
  int sock;
  struct sockaddr_in echoserver;
  struct sockaddr_in echoclient;
  char buffer[BUFFSIZE];
  char bufferl[BUFFSIZE];
  int level = 0;
  unsigned int echolen, clientlen;
  int received = 0;
  pthread_t tgraph;
  int rc1;
  int t = 0;
  long int elapsed = 0;
  struct timespec tinicio;
  int dT = 10;
  // int rv;

  //float ref = 0.8;
  float e;
  float eant = 0;
  float u;
  float uDelta = 0;
  float I = 0;
  float D = 0;
  float Dant = 0;
  float Iant = 0;
  char controle[BUFFSIZE];
  
  if (argc != 3) {
    fprintf(stderr, "USAGE: %s <server_ip> <port> \n", argv[0]);
    exit(1);
  }

  /* Create the UDP socket */
  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    Die("Failed to create socket");
  }
  /* Construct the server sockaddr_in structure */
  memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
  echoserver.sin_family = AF_INET;                  /* Internet/IP */
  echoserver.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
  echoserver.sin_port = htons(atoi(argv[2]));       /* server port */

  {
    espera(100); 
 while(startG == 0){ //Condição para iniciar o controle
   erro = 0;
    Smsg(sock, "CommTest!", echoserver);
    Rmsg(sock, buffer, echoclient, echoserver);
    espera(10);

    Smsg(sock, "SetMax#100!", echoserver);
    Rmsg(sock, buffer, echoclient, echoserver);
    espera(10);
  
    Smsg(sock, "Start!", echoserver);
    Rmsg(sock, buffer, echoclient, echoserver);
    if(erro == 0){startG = 1;}
    espera(10);
    }
  
  if( (rc1=pthread_create( &tgraph, NULL, &graphS, &plot)) )	   
    {	      
      printf("Thread creation failed: %d\n", rc1);	   
      }
  espera(100);

  while(1){
    clock_gettime(CLOCK_MONOTONIC_RAW,&tinicio);//atualiza inicio para comparação
    Smsg(sock, "GetLevel!", echoserver);
    Rmsg(sock, bufferl, echoclient, echoserver);
    level = getNum(bufferl);
    //printf("%d \n",level);
    //espera(150);
    
    plot[0] = t;
    // if(erro == 0){
    plot[1] = level;//}
    plot[2] = posV;
    // if(erro == 0){
    e = ref - level/100.0;//}
    //printf("%f \n", e);
   
    I = Iant +ki*(e+eant)*dT;
    //Anti wind up
    if(I>30 && t<60000){I=30;}
    if(I<-50){I=-50;}
    D = Dant + kd*(e-eant)/dT;

    u = I + kp*e + D;
    //printf("%f \n", u);
    //Saturação do Controle
    if(u>95){u=95;}
    else if(u<5){u=5;}

    uDelta = u - posV;
    //printf("%f \n", uDelta);
   
    //Saturação dos comandos de valvula
    if(uDelta > 3){uDelta = 3;}
    else if(uDelta < -3){uDelta = -3;}
    //Comandos de abertura e fechamento
     if(uDelta != 0){
      if(uDelta>0)
	{
	  openv(uDelta,controle);
	  Smsg(sock, controle, echoserver);
	  Rmsg(sock, buffer, echoclient, echoserver);
	  /*if(erro == 0)*/{posV += (int)uDelta;}
	}
      else if(uDelta<0){ 
	closev(-uDelta,controle);
	Smsg(sock, controle, echoserver);
	Rmsg(sock, buffer, echoclient, echoserver);
	/*if(erro == 0)*/{posV += (int)uDelta;}
      }
    }

    erro = 0;
    
    eant = e;
    Iant = I;
    Dant = D;

    //Controle temporal
    while(elapsed < 300*1000)//trava 300ms
	{
	  elapsed = texec(tinicio);
	}
    dT = elapsed/1000;
    elapsed = 0;
    t = t + dT; 
    
  }}

  close(sock);
  exit(0);
}
