 #include <stdlib.h>
 #include <unistd.h>
 #include <stdio.h>
 #include <fcntl.h>
 #include <linux/fb.h>
 #include <sys/mman.h>
 #include <sys/ioctl.h>
#include <time.h>

int fbfd = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize = 0;
char *fbp = 0;
int x = 0, y = 0;
long int location = 0;


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


void drawText(int xCoord , int yCoord) {
    FILE * inputFile;
    int pix;
    char see;
    int stop = 0;
    int xCanvas= xCoord;
    int yCanvas = yCoord;

    inputFile = fopen("k.txt","r");
    

    while (!stop) {
        fscanf(inputFile, "%d", &pix);
        location = (xCanvas+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (yCanvas+vinfo.yoffset) * finfo.line_length;
         *(fbp + location) = pix;        //blue
         *(fbp + location + 1) = pix;    //green
         *(fbp + location + 2) = pix;    //red
         *(fbp + location + 3) = 0;      //transparansi
        //printf("%d ", pix);
        xCanvas++;


        see = getc(inputFile);

         if (see == '\n') {
            //printf("\n");
            xCanvas = 0;
            yCanvas++;

         }
         else if( see == EOF) {
            stop = 1;
         }
    }

}


 int main()
 {
     
     init();
     clearScreen();
     int j;    
     for (j = 0 ; j < 100 ; j++) {
        drawText(j,0);
        usleep(50000);
        clearScreen();
     }

     





    



     munmap(fbp, screensize);
     close(fbfd);
     return 0;
 }