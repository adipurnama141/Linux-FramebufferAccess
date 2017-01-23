#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h>
#include <pthread.h>




int fbfd = 0;                       //Filebuffer Filedescriptor
struct fb_var_screeninfo vinfo;     //Struct Variable Screeninfo
struct fb_fix_screeninfo finfo;     //Struct Fixed Screeninfo
long int screensize = 0;            //Ukuran data framebuffer
char *fbp = 0;                      //Framebuffer di memori internal


void *movePlane(void *);






// STRUKTUR DATA BLOCK CHARACTER
// --------------------------------------------------------------------------------------------------------- //

typedef struct {
  int *array;
  size_t used;
  size_t size;
  int x;
  int y;
} Block;

void initArray(Block *a, size_t initialSize) {
  a->array = (int *)malloc(initialSize * sizeof(int));
  a->used = 0;
  a->size = initialSize;
}

void insertArray(Block *a, int element) {
  // a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
  // Therefore a->used can go up to a->size 
  if (a->used == a->size) {
    a->size *= 2;
    a->array = (int *)realloc(a->array, a->size * sizeof(int));
  }
  a->array[a->used++] = element;
}

void freeArray(Block *a) {
  free(a->array);
  a->array = NULL;
  a->used = a->size = 0;
}

void setCoordinate(Block *a , int _x , int _y){
    a->x = _x;
    a->y = _y;
}

// --------------------------------------------------------------------------------------------------------- //



void initScreen() {
//Mapping framebuffer ke memori internal

     // Buka framebuffer
     fbfd = open("/dev/fb0", O_RDWR);
     if (fbfd == -1) {
         perror("Error: cannot open framebuffer device");
         exit(1);
     }
     printf("The framebuffer device was opened successfully.\n");

     // Ambil data informasi screen
     if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
         perror("Error reading fixed information");
         exit(2);
     }
     if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
         perror("Error reading variable information");
         exit(3);
     }

     // Informasi resolusi, dan bit per pixel
     printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

     // Hitung ukuran memori untuk menyimpan framebuffer
     screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

     // Masukkan framebuffer ke memory, untuk kita ubah-ubah
     fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fbfd, 0);
     if ((int)fbp == -1) {
         perror("Error: failed to map framebuffer device to memory");
         exit(4);
     }
     printf("The framebuffer device was mapped to memory successfully.\n");

}


void plotPixelRGBA(int _x , int _y , int r , int g , int b, int a) {
//Plot pixel (x,y) dengan warna (r,g,b,a)
    long int location = (_x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (_y+vinfo.yoffset) * finfo.line_length;
    *(fbp + location) = b;        //blue
    *(fbp + location + 1) = g;    //green
    *(fbp + location + 2) = r;    //red
    *(fbp + location + 3) = a;      //
}

void plotPixelRGB(int _x , int _y , int r , int g , int b) {
//plot pixel (x,y) dengan warna (r,g,b)
    long int location = (_x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (_y+vinfo.yoffset) * finfo.line_length;
    unsigned short int t = r<<11 | g << 5 | b;
    *((unsigned short int*)(fbp + location)) = t;
}


void clearScreen() {
//Mewarnai latar belakang screen dengan warna putih
    int x;
    int y;
    for (y = 0; y < vinfo.yres - 15 ;y++)
         for (x = 0; x < vinfo.xres  ; x++) {
             if (vinfo.bits_per_pixel == 32) {
                 plotPixelRGBA(x,y,255,255,255,0);
             } else  { //asumsi mode 16 bit per piksel
                 plotPixelRGB(x,y,255,255,255);
             }

         }
}



void initBlock(char * blockfile, Block * blockmem , int _x , int _y) {
//Membaca file block character kedalam memori, tempatkan di koordinaat (x,y) layar

    FILE * inputFile;
    int pix;
    char see;
    int stop = 0;

    initArray(blockmem, 5);
    inputFile = fopen(blockfile,"r");
    (*blockmem).x = _x;
    (*blockmem).y = _y;

    while (!stop) {
        fscanf(inputFile, "%d", &pix);
        insertArray(blockmem, pix);
        //printf("%d ", pix);
        see = getc(inputFile);
        //printf("isee : %d \n", see );
         if (see == 13) {
            insertArray(blockmem, -1);
            //printf("\n");
         }
         else if( see == EOF) {
            stop = 1;
         }
    }
}

int isOverflow(int _x , int _y){
//Cek apakah kooordinat (x,y) sudah melewati batas layar
    int result;
    if ( _x > vinfo.xres ||  _y > vinfo.yres -20 ) {
        result = 1;
    }
    else {
        result = 0;
    }
    return result;
}



void drawBlock(Block * blockmem){
//Menggambar block character pada layar

    int i;
    int xCanvas = blockmem->x;
    int yCanvas = blockmem->y;
    int pix;
    for (i =0 ; i < (*blockmem).used ; i++) {
        pix = (*blockmem).array[i];
        if ( pix == -1 ) {
            xCanvas = blockmem->x;
            yCanvas++;
        }
        else {
            if (!isOverflow(xCanvas,yCanvas)){
                plotPixelRGBA(xCanvas,yCanvas,pix,pix,pix,0);
            }
        }
        xCanvas++;
    }
}










void drawLinePositive(int x1, int y1, int x2, int y2, int clear){
    plotPixelRGBA(x1,y1,0,0,0,0);

    int dy = y2-y1;
    int dx = x2-x1;
    int p = (dy + dy) - dx;
    int x = x1;
    int y = y1;

    while ( x != x2) {
        if (p < 0) {
            p = p + dy + dy;
            x++;
        }
        else if (p > 0){
            p = p + dy + dy - dx - dx;
            x++;
            y++;
        }
        if (clear) {
          plotPixelRGBA(x,y,255,255,255,0);
        }
        else {
          plotPixelRGBA(x,y,0,0,0,0);
        }
    }
}


void drawLineNegative(int x1, int y1, int x2, int y2,int clear){
    plotPixelRGBA(x1,y1,0,0,0,0);
    int dy = y1-y2;
    int dx = x2-x1;
    int p = (dy + dy) - dx;
    int x = x1;
    int y = y1;
    while ( x != x2) {
        if (p < 0) {
            p = p + dy + dy;
            x++;
        }
        else if (p > 0){
            p = p + dy + dy - dx - dx;
            x++;
            y--;
        }
        if (clear) {
          plotPixelRGBA(x,y,255,255,255,0);
        }
        else {
          plotPixelRGBA(x,y,0,0,0,0);
        }
    }
}



void drawHorizontalLine(int x1,int y1,int x2, int y2,int clear){
    plotPixelRGBA(x1,y1,0,0,0,0);
    int i;
    for(i = x1 ; i <= x2; i++){
        if (clear) {
          plotPixelRGBA(i,y1,255,255,255,0);
        }
        else {
          plotPixelRGBA(i,y1,0,0,0,0);
        }
    }
}

void drawVerticalLine(int x1,int y1,int x2, int y2,int clear){
    plotPixelRGBA(x1,y1,0,0,0,0);
    int i;
    for(i = y1 ; i <= y2; i++){
        if (clear) {
          plotPixelRGBA(x1,i,255,255,255,0);
        }
        else {
          plotPixelRGBA(x1,i,0,0,0,0);
        }

    }
}


//x harus naik, y boleh turun
void drawLine(int x1,int y1, int x2, int y2,int clear){
    if (x1 > x2) {
        drawLine(x2,y2,x1,y1,clear);
    }
    else if (x1 == x2) {
        drawVerticalLine(x1,y1,x2,y2,clear);
    }
    else if (y1 == y2){
        drawHorizontalLine(x1,y1,x2,y2,clear);
    }
    else if ((x2 > x1) && (y2 > y1)) {
        drawLinePositive(x1,y1,x2,y2,clear);
    }
    else if ((x2 > x1) && (y1 > y2)) {
        drawLineNegative(x1,y1,x2,y2,clear);
    }
}



void removeBlock(Block * blockmem){
//Menghapus block character pada layar

    int i;
    int xCanvas = blockmem->x;
    int yCanvas = blockmem->y;
    int pix;
    for (i =0 ; i < (*blockmem).used ; i++) {
        pix = (*blockmem).array[i];
        if ( pix == -1 ) {
            xCanvas =  blockmem->x;
            yCanvas++;
        }
        else {
            if (!isOverflow(xCanvas,yCanvas)){
                plotPixelRGBA(xCanvas,yCanvas,255,255,255,0);
            }
        }
        xCanvas++;
    }
}


void moveUp(Block * blockmem, int px){
//Animasi gerak ke atas sebesar x pixel

    int j = blockmem -> y;
    int initPx = j;
    for (j ; j > initPx - px ; j = j - 4){
        setCoordinate(blockmem , blockmem->x , j);
        drawBlock(blockmem);
        usleep(5000);
        removeBlock(blockmem);
    }
    drawBlock(blockmem);
}


void terminate() {
//Pengakhiran program.
     munmap(fbp, screensize);
     close(fbfd);
}


void drawPlane(int xMove){
     //garis atas
    drawLine(1100-xMove,50,1250-xMove,50,0);

    //garis bawah
    drawLine(1054-xMove,95,1285-xMove,95,0);

    //garis atas
    drawLine(1285-xMove,25,1285-xMove,95,0);

    //sayap belakang
    drawLine(1285-xMove,25,1250-xMove,50,0);

    //moncong depan
    drawLine(1100-xMove,50,1050-xMove,100,0);

    drawLine(1140-xMove,80,1240-xMove,120,0);
    drawLine(1240-xMove,120,1270-xMove,120,0);
    drawLine(1270-xMove,120,1180-xMove,80,0);
}

void clearPlane(int xMove){
   //garis atas
    drawLine(1100-xMove,50,1250-xMove,50,1);

    //garis bawah
    drawLine(1054-xMove,95,1285-xMove,95,1);

    //garis atas
    drawLine(1285-xMove,25,1285-xMove,95,1);

    //sayap belakang
    drawLine(1285-xMove,25,1250-xMove,50,1);

    //moncong depan
    drawLine(1100-xMove,50,1050-xMove,100,1);

    drawLine(1140-xMove,80,1240-xMove,120,1);
    drawLine(1240-xMove,120,1270-xMove,120,1);
    drawLine(1270-xMove,120,1180-xMove,80,1);
}


void * movePlane(void * unused){
  int i = 0;

  while(1) {
  
    
  for (i = 0 ; i <= 1366 ; i++){
    drawPlane(i);
    usleep(5000);
    clearPlane(i);
  }
  }
}

void * IOhandler(void * u){
int ch;

while(1) {
    ch = getchar();
    if (ch == 10) {
      clearScreen();
      drawLine(550,650,750,650,0);
      drawLine(550,651,750,651,0);
      drawLine(550,652,750,652,0);
      drawLine(550,653,750,653,0);
      launchUpMissile();
    }
}
}

void drawMissileLauncher(){
  while (1) {

    drawLine(570,570,650,650,0);
    usleep(50000);
    drawLine(570,570,650,650,1);
   

    drawLine(650,550,650,650,0);
    usleep(50000);
    drawLine(650,550,650,650,1);

    drawLine(730,470,650,650,0);
    usleep(50000);
    drawLine(730,470,650,650,1);

    
  }
}


void drawMissileLauncherBase(){
  drawLine(550,650,750,650,0);
  drawLine(550,651,750,651,0);
  drawLine(550,652,750,652,0);
  drawLine(550,653,750,653,0);
}


void drawUpMissile(int dy, int clear){
  drawLine(648,550-dy,648,600-dy,clear);
  drawLine(649,550-dy,649,600-dy,clear);
  drawLine(650,550-dy,650,600-dy,clear);
  drawLine(651,550-dy,651,600-dy,clear);
  drawLine(652,550-dy,652,600-dy,clear);
}



void launchUpMissile(){
  int i = 0;
  for ( i= 0 ; i < 500 ; i++){
    drawUpMissile(i,0);
    usleep(5000);
    drawUpMissile(i,1);

  }
}


int main()
{
    initScreen();
    clearScreen();
 

    drawLine(648,550,648,600,0);
    drawLine(649,550,649,600,0);
    drawLine(650,550,650,600,0);
    drawLine(651,550,651,600,0);
    drawLine(652,550,652,600,0);



     

    drawMissileLauncherBase();

    

    pthread_t planeThread;
    pthread_t IOthread;
    pthread_create(&planeThread,NULL,movePlane,NULL);
    pthread_create(&IOthread,NULL,IOhandler,NULL);


    drawMissileLauncher();


    while(1){

    }


 


    terminate();

    return 0;
 }