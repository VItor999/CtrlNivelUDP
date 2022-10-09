/**
 * @file graph.h
 * @author Lucas Esteves  e Vitor Carvalho
 * @brief Parte Gráfica do código. Adaptado do código fornecdo por
 *  Alceu Heinke Frigeri (professor da disciplina)
 * @version FINAL
 * @date 2022-10-06
 * 
 */
#ifndef graph
//===================== Bibliotecas utilizadas =====================//
#include <stdio.h>
#include <SDL/SDL.h>
#include <math.h>

//====================== Definições efetuadas ======================//
#define SCREEN_W 640                            /* Largura da janela */
#define SCREEN_H 640                            /* Altura da janela */
#define BPP 32

typedef Uint32 PixelType;

/**
*@brief Estrutura que define os elementos necessário de um canvas 
*
**/
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

/**
*@brief  Define os Campos que a estrutura gráfica proposta precisa
*
**/
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

//===================== Cabeçalhos de Funções =====================//

void c_pixeldraw(Tcanvas *canvas, int x, int y, PixelType color);
void c_hlinedraw(Tcanvas *canvas, int xstep, int y, PixelType color);
void c_vlinedraw(Tcanvas *canvas, int x, int ystep, PixelType color);
void c_linedraw(Tcanvas *canvas, double x0, double y0, double x1, double y1, PixelType color);
Tcanvas *c_open(int Width, int Height, double Xmax, double Ymax);
Tdataholder *datainit(int Width, int Height, double Xmax, double Ymax, double Lcurrent, double INcurrent, double OUTcurrent);
void setdatacolors(Tdataholder *data, PixelType Lcolor, PixelType INcolor, PixelType OUTcolor);
void datadraw(Tdataholder *data, double time, double level, double inangle, double outangle);
void quitevent(char *OUT);
void printGrid(Tcanvas *canvas,int Width, int Height, double Xmax, double Ymax);
void Restart(int Width, int Height, double Xmax, double Ymax,double Lcurrent, double INcurrent, double OUTcurrent, Tdataholder *data);

//#################################################################//
//########################   FUNÇÕES   ############################//
//#################################################################//

/**
*@brief Cria o grid 
*
*@param canvas Endereço do Canvas em que se opera
*@param Width  Largura
*@param Height Alura 
*@param Xmax   X máximo
*@param Ymax   Y máximo
**/
void printGrid(Tcanvas *canvas,int Width, int Height, double Xmax, double Ymax){
  int x,y;
  canvas->Xoffset = 10;
  canvas->Yoffset = Height;
  canvas->Xext = 10;
  canvas->Yext = 10;
  canvas->Height = Height;
  canvas->Width  = Width; 
  canvas->Xmax   = Xmax;
  canvas->Ymax   = Ymax;
  canvas->Xstep  = Xmax/(double)Width/2;
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
}

/**
*@brief        Desenha um ponto em uma coordenada
*
*@param canvas Endereço da área de denho
*@param x      Coordenada X 
*@param y      Coordenada Y
*@param color  Cor
**/
 void c_pixeldraw(Tcanvas *canvas, int x, int y, PixelType color)
{
  *( ((PixelType*)canvas->canvas->pixels) + ((-y+canvas->Yoffset) * canvas->canvas->w + x+ canvas->Xoffset)) = color;
}

/**
*@brief Desenha as linhas horizontais pontilhadas do grid 
*
*@param canvas Endereço da área de desenho
*@param xstep  Passo em X 
*@param y      Valor em Y
*@param color  Cor
**/
 void c_hlinedraw(Tcanvas *canvas, int xstep, int y, PixelType color)
{
  int offset =  (-y+canvas->Yoffset) * canvas->canvas->w;
  int x;

  for (x = 0; x< canvas->Width+canvas->Xoffset ; x+=xstep) {
        *( ((PixelType*)canvas->canvas->pixels) + (offset + x)) = color;
  }
}

/**
*@brief Desenha as linhas verticais pontilhadas do grid 
*
*@param canvas Endereço da área de desenho
*@param x      Valor em X
*@param ystep  Valor em Y
*@param color  Cor
**/
 void c_vlinedraw(Tcanvas *canvas, int x, int ystep, PixelType color)
{
  int offset = x+canvas->Xoffset;
  int y;
  int Ystep = ystep*canvas->canvas->w;

  for (y = 0; y< canvas->Height+canvas->Yext ; y+=ystep) {
    *( ((PixelType*)canvas->canvas->pixels) + (offset + y*canvas->canvas->w)) = color;
  }
}

/**
*@brief  Desenha uma linha sólida no gráfico
*
*@param canvas  Endereço da área de desenho
*@param x0      X inicial
*@param y0      Y inicial
*@param x1      X final
*@param y1      Y incial
*@param color  Cor
**/
 void c_linedraw(Tcanvas *canvas, double x0, double y0, double x1, double y1, PixelType color) {
  double x;

  for (x=x0; x<=x1; x+=canvas->Xstep) {
    c_pixeldraw(canvas, (int)(x*canvas->Width/canvas->Xmax+0.5), (int)((double)canvas->Height/canvas->Ymax*(y1*(x1-x)+y1*(x-x0))/(x1-x0)+0.5),color);
  }
}

/**
*@brief Abre/Cria a área de desenho
*
*@param Width Largura da janela
*@param Height Altura da janela
*@param Xmax  X máximo
*@param Ymax  Y máximo
*@return Tcanvas*  Retorna o endereço da estrutura de desenho criada
**/
Tcanvas *c_open(int Width, int Height, double Xmax, double Ymax)
{
  
  Tcanvas *canvas;
  canvas = malloc(sizeof(Tcanvas));
  printGrid(canvas, Width,  Height,  Xmax,  Ymax);
  return canvas;
}

/**
*@brief Reinicia o gráfico 
*
*@param Width  Largura da janela
*@param Height Altura da janela
*@param Xmax  X máximo
*@param Ymax  Y máximo
*@param Lcurrent  Nível atual
*@param INcurrent Abertura de entrada atual
*@param OUTcurrent Abertura de saída atual
*@param data      Endereço Estrutura que contém todas as infos da parte gráfica
**/
void Restart(int Width, int Height, double Xmax, double Ymax,double Lcurrent, double INcurrent, double OUTcurrent, Tdataholder *data) {
  int x,y;
  data->Tcurrent=0;
  bzero(data->canvas, sizeof(Tcanvas));
  printGrid(data->canvas, Width,  Height,  Xmax,  Ymax);
  data->Lcurrent=Lcurrent;
  data->Lcolor= (PixelType) SDL_MapRGB(data->canvas->canvas->format,  255, 180,  0);
  data->INcurrent=INcurrent;
  data->INcolor=(PixelType) SDL_MapRGB(data->canvas->canvas->format,  180, 255,  0);
  data->OUTcurrent=OUTcurrent;
  data->OUTcolor=(PixelType) SDL_MapRGB(data->canvas->canvas->format,  0, 180,  255);
}

/**
*@brief Inicia toda a estrutura gráfica;
*
*@param Width Largura da janela
*@param Height  Altura da janela
*@param Xmax  X máximo
*@param Ymax  Y máximo
*@param Lcurrent Nível atual
*@param INcurrent Abertura de entrada atual
*@param OUTcurrent Abertura de saída atual
*@return Tdataholder*  Retorna o enderço que contém todas as informaçoes gráficas
**/
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

/**
*@brief Define as cores e cada curva
*
*@param data Endereço da estrutura global do plot
*@param Lcolor Cor do nível
*@param INcolor Cor da abertura da válvula de entraa
*@param OUTcolor Cor da abertura da válvula de saida
**/
void setdatacolors(Tdataholder *data, PixelType Lcolor, PixelType INcolor, PixelType OUTcolor) {
  data->Lcolor=Lcolor;
  data->INcolor=INcolor;
  data->OUTcolor=OUTcolor;
}

/**
*@brief Desenha, de fato, um novo ponto na janela gráfica 
*
*@param data Endereço da estrutura que contém todos os dados da parte gráfica
*@param time Tempo atual
*@param level Nível atual
*@param inangle Ângulo de entrada atual 
*@param outangle Ângulo de saída atual
**/
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

/**
*@brief Verifica quando o X da janela foi pressionado e deve-se encerrar a parete gráfica
*
*@param OUT Recebe o endereço da váriavel utilizada para notificar o encerramento do programa
**/
void quitevent(char *OUT) {
  SDL_Event event;

  while(SDL_PollEvent(&event)) { 
    if(event.type == SDL_QUIT) { 
      // close files, etc...

      SDL_Quit();
      *OUT = 27;
     //exit(1); // this will terminate all threads !
    }
  }

}

//int main( int argc, const char* argv[] ) {
//  Tdataholder *data;
//  double t=0;
//
//  data = datainit(640,480,55,110,45,0,0);
//
//  for (t=0;t<50;t+=0.1) {
//    datadraw(data,t,(double)(50+20*cos(t/5)),(double)(70+10*sin(t/10)),(double)(20+5*cos(t/2.5)));
//  }
//
//  while(1) {
//    quitevent();
//  }
//}
#define graph
#endif