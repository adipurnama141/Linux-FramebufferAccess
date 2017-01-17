 #include <stdlib.h>
 #include <unistd.h>
 #include <stdio.h>
 #include <fcntl.h>
 #include <linux/fb.h>
 #include <sys/mman.h>
 #include <sys/ioctl.h>
#include <time.h>

typedef struct {
  int *array;
  size_t used;
  size_t size;
} Array;

int fbfd = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize = 0;
char *fbp = 0;
int x = 0, y = 0;
long int location = 0;
Array block;



void initArray(Array *a, size_t initialSize) {
  a->array = (int *)malloc(initialSize * sizeof(int));
  a->used = 0;
  a->size = initialSize;
}

void insertArray(Array *a, int element) {
  // a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
  // Therefore a->used can go up to a->size 
  if (a->used == a->size) {
    a->size *= 2;
    a->array = (int *)realloc(a->array, a->size * sizeof(int));
  }
  a->array[a->used++] = element;
}

void freeArray(Array *a) {
  free(a->array);
  a->array = NULL;
  a->used = a->size = 0;
}


void init() {
     // Buka framebuffer
     fbfd = open("/dev/fb0", O_RDWR);
     if (fbfd == -1) {
         perror("Error: cannot open framebuffer device");
         exit(1);
     }
     printf("The framebuffer device was opened successfully.\n");

     // Get fixed screen information
     if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
         perror("Error reading fixed information");
         exit(2);
     }

     // Get variable screen information
     if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
         perror("Error reading variable information");
         exit(3);
     }

     //print resolusi layar dan jumlah bit per piksel
     printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

     // Hitung 
     screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

     // Map the device to memory
     fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fbfd, 0);
     if ((int)fbp == -1) {
         perror("Error: failed to map framebuffer device to memory");
         exit(4);
     }
     printf("The framebuffer device was mapped to memory successfully.\n");


     //init block array
     initArray(&block, 5);

}



void clearScreen() {
    for (y = 0; y < vinfo.yres - 15 ;y++)
         for (x = 0; x < vinfo.xres  ; x++) {

             location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
                        (y+vinfo.yoffset) * finfo.line_length;

             if (vinfo.bits_per_pixel == 32) {
                 *(fbp + location) = 255;        //blue
                 *(fbp + location + 1) = 255;    //green
                 *(fbp + location + 2) = 255;    //red
                 *(fbp + location + 3) = 0;      //transparansi
             } else  { //asumsi mode 16 bit per piksel
                 int b = 100;   // blue
                 int g = 100;   // green
                 int r = 100;   // red
                 unsigned short int t = r<<11 | g << 5 | b;
                 *((unsigned short int*)(fbp + location)) = t;
             }

         }


}



void loadBlockToMemory(char * blockfile, Array * blockmem) {
    FILE * inputFile;
    int pix;
    char see;
    int stop = 0;

    inputFile = fopen(blockfile,"r");

    while (!stop) {
        fscanf(inputFile, "%d", &pix);
        insertArray(blockmem, pix);
        see = getc(inputFile);
         if (see == '\n') {
            insertArray(&block, -1);
         }
         else if( see == EOF) {
            stop = 1;
         }
    }

}



void drawBlock(Array * blockmem, int xCoord , int yCoord ){
    int i;
    int xCanvas = xCoord;
    int yCanvas = yCoord;
    int pix;
    for (i =0 ; i < (*blockmem).used ; i++) {
        pix = (*blockmem).array[i];
        //printf("%d ", pix);
        if ( pix == -1 ) {
            xCanvas = xCoord;
            yCanvas++;
            //printf("\n");
        }
        else {
            location = (xCanvas+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (yCanvas+vinfo.yoffset) * finfo.line_length;
            *(fbp + location) = pix;        //blue
            *(fbp + location + 1) = pix;    //green
            *(fbp + location + 2) = pix;    //red
            *(fbp + location + 3) = 0;      //transparansi
        }
        xCanvas++;
    }

}


void drawText(int xCoord, int yCoord){
    int i;
    int xCanvas = xCoord;
    int yCanvas = yCoord;
    int pix;
    for (i =0 ; i < block.used ; i++) {
        pix = block.array[i];
        //printf("%d ", pix);
        if ( pix == -1 ) {
            xCanvas = xCoord;
            yCanvas++;
            //printf("\n");
        }
        else {
            location = (xCanvas+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (yCanvas+vinfo.yoffset) * finfo.line_length;
            *(fbp + location) = pix;        //blue
            *(fbp + location + 1) = pix;    //green
            *(fbp + location + 2) = pix;    //red
            *(fbp + location + 3) = 0;      //transparansi
        }
        xCanvas++;
    }
}

void removeText(int xCoord, int yCoord){
    int i;
    int xCanvas = xCoord;
    int yCanvas = yCoord;
    int pix;
    for (i =0 ; i < block.used ; i++) {
        pix = block.array[i];
        //printf("%d ", pix);
        if ( pix == -1 ) {
            xCanvas = xCoord;
            yCanvas++;
            //printf("\n");
        }
        else {
            location = (xCanvas+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (yCanvas+vinfo.yoffset) * finfo.line_length;
            *(fbp + location) = 255;        //blue
            *(fbp + location + 1) = 255;    //green
            *(fbp + location + 2) = 255;    //red
            *(fbp + location + 3) = 0;      //transparansi
        }
        xCanvas++;
    }


}


 int main()
 {
     
     init();
     clearScreen();
     loadBlockToMemory("k.txt",&block);   

     int j = 0 ;
     for ( j = 0 ; j < 300 ; j = j + 2) {
        removeText(j,j);
        //drawText(j,j);
        drawBlock(&block , j , j);
        usleep(50000);
        
     }
     

     


     munmap(fbp, screensize);
     close(fbfd);
     return 0;
 }