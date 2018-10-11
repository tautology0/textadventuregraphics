/* Routine to draw the PC graphics from Claymorgue

   Code by David Lodge 29/04/2005
*/
#include <stdio.h>
#include <stdlib.h>
#include <allegro.h>

int x=0,y=0,count=0;
BITMAP *savescr;
int xlen=280, ylen=158;
int xoff=0, yoff=0;
int size;
int ycount=0;
int skipy=1;
FILE *infile,*outfile;

void drawpixels(pattern)
int pattern;
{
   int pix1,pix2,pix3,pix4;
   // Now get colours
   pix1=(pattern & 0xc0)>>6;
   pix2=(pattern & 0x30)>>4;
   pix3=(pattern & 0x0c)>>2;
   pix4=(pattern & 0x03);

   putpixel(savescr,x,y, pix1);
   x++;
   if (!skipy) { putpixel(savescr,x,y, pix1); x++; }
   putpixel(savescr,x,y, pix2);
   x++;
   if (!skipy) { putpixel(savescr,x,y, pix2); x++; }
   putpixel(savescr,x,y, pix3);
   x++;
   if (!skipy) { putpixel(savescr,x,y, pix3); x++; }
   putpixel(savescr,x,y, pix4);
   x++;
   if (!skipy) { putpixel(savescr,x,y, pix4); x++; }
   if (x>=xlen+xoff)
   {
      y+=2;
      x=xoff;
      ycount++;
   }
   if (ycount>ylen)
   {
      y=yoff+1;
      count++;
      ycount=0;
   }
}


int main(int argc, char **argv)
{
   int work,outpic;
   int c;
   int i,j;
   char filename[256];
   char *ptr;
   int readcount=0;
   int linelen;
   int rawoffset;
   PALETTE pal;
   RGB black = { 0,0,0 };
   RGB blue = { 0,0,63 };
   RGB red = { 63,0,0 };
   RGB magenta = { 63,0,63 };
   RGB green = { 0,63,0 };
   RGB cyan = { 0,63,63 };
   RGB yellow = { 63,63,0 };
   RGB white = { 63,63,63 };

   allegro_init();
   install_keyboard();
   savescr=create_bitmap(280,158);

   /* set up the palette */
   set_color(0,&black);
   set_color(1,&cyan);
   set_color(2,&magenta);
   set_color(3,&white);
   get_palette(pal);

   infile=fopen(argv[1],"rb");

   // Get the size of the graphics chuck
   fseek(infile, 0x05, SEEK_SET);
   work=fgetc(infile);
   size=work+(fgetc(infile)*256);

   // Get whether it is lined
   fseek(infile,0x0d,SEEK_SET);
   work=fgetc(infile);
   if (work == 0xff) skipy=0;

   // Get the offset
   fseek(infile,0x0f,SEEK_SET);
   work=fgetc(infile);
   rawoffset=work+fgetc(infile)*256;
   xoff=((rawoffset % 80)*4)-24;
   yoff=rawoffset / 80;
   x=xoff;
   y=yoff;

   // Get the y length
   fseek(infile,0x11,SEEK_SET);
   work=fgetc(infile);
   ylen=work+(fgetc(infile)*256);
   ylen-=rawoffset;
   ylen/=80;

   // Get the x length
   fseek(infile,0x13,SEEK_SET);
   xlen=fgetc(infile)*4;

   fseek(infile,0x17,SEEK_SET);
   j=0;
   while (!feof(infile) && ftell(infile)<size)
   {
      // First get count
      c=fgetc(infile);

      if ((c & 0x80) == 0x80)
      { // is a counter
         work=fgetc(infile);
         c &= 0x7f;
         for (i=0;i<c+1;i++)
         {
            drawpixels(work);
         }
      }
      else
      {
         // Don't count on the next j characters
         for (i=0;i<c+1;i++)
         {
            work=fgetc(infile);
            drawpixels(work);
         }
      }
      if (count>1) break;
   }
   ptr=strrchr(argv[1],'.');
   strcpy(ptr,".bmp");
   sprintf(filename,"Output\\%s",argv[1]);
   save_bmp(filename, savescr, pal);
   fclose(infile);
   allegro_exit();
}



