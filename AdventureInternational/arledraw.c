/* Routine to draw the Atari RLE graphics

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
FILE *infile,*outfile;
RGB colours[255];
RGB wibble;
int countflag;

void setuppalette(void)
{
   int i;
   FILE *palfile;
   palfile=fopen("default.act","rb");

   if (palfile == NULL)
   {
      printf("Could not open palette file\n");
      exit(1);
   }

   for (i=0;i<256;i++)
   {
      colours[i].r=fgetc(palfile)/4;
      colours[i].g=fgetc(palfile)/4;
      colours[i].b=fgetc(palfile)/4;
   }
   fclose(palfile);
}



void drawpixels(pattern,pattern2)
int pattern,pattern2;
{
   int pix1,pix2,pix3,pix4;

   printf("Plotting at %d %d: %x %x\n",x,y,pattern,pattern2);
   // Now get colours
   if (x>(xlen+xoff)*8)
      return;
   pix1=(pattern & 0xc0)>>6;
   pix2=(pattern & 0x30)>>4;
   pix3=(pattern & 0x0c)>>2;
   pix4=(pattern & 0x03);

   putpixel(savescr,x,y, pix1); x++;
   putpixel(savescr,x,y, pix1); x++;
   putpixel(savescr,x,y, pix2); x++;
   putpixel(savescr,x,y, pix2); x++;
   putpixel(savescr,x,y, pix3); x++;
   putpixel(savescr,x,y, pix3); x++;
   putpixel(savescr,x,y, pix4); x++;
   putpixel(savescr,x,y, pix4); x++;
   y++;
   x-=8;

   pix1=(pattern2 & 0xc0)>>6;
   pix2=(pattern2 & 0x30)>>4;
   pix3=(pattern2 & 0x0c)>>2;
   pix4=(pattern2 & 0x03);

   putpixel(savescr,x,y, pix1); x++;
   putpixel(savescr,x,y, pix1); x++;
   putpixel(savescr,x,y, pix2); x++;
   putpixel(savescr,x,y, pix2); x++;
   putpixel(savescr,x,y, pix3); x++;
   putpixel(savescr,x,y, pix3); x++;
   putpixel(savescr,x,y, pix4); x++;
   putpixel(savescr,x,y, pix4); x++;
   y++;
   x-=8;

   if (y>ylen)
   {
      x+=8;
      y=yoff;
   }
}


int main(int argc, char **argv)
{
   int work,work2, outpic=0;
   int c,count;
   int i,j;
   char filename[256];
   char *ptr;
   int readcount=0;
   int linelen;
   int rawoffset;
   int offset;
   int curpos;
   PALETTE pal;
   RGB black = { 0, 0, 0 };

   allegro_init();
   install_keyboard();
   savescr=create_bitmap(280,159);
   /* set up the palette */
   for (i=0;i<256;i++)
   {
      colours[i].r=0; colours[i].g=0;colours[i].b=0;
   }
   setuppalette();

   infile=fopen(argv[1],"rb");
   offset=strtol(argv[2],NULL,0);
   count=strtol(argv[3],NULL,0);
   if (strcmp(argv[4],"y")==0) countflag=1;
   printf("%d\n",countflag);

   // Get the size of the graphics chuck
   fseek(infile, offset, SEEK_SET);

   // Now loop round for each image
   while (outpic<count)
   {
      x=0;y=0;
      work2=fgetc(infile);
      do
      {
         work=work2;
         work2=fgetc(infile);
         size=(work2*256)+work;
         printf("%x\n",size);
      } while ( size == 0 || size > 0x1600 || work==0);
      printf("%d: %x\n",outpic,ftell(infile));
      curpos=ftell(infile)-2;
      //size=work+(fgetc(infile)*256)+curpos;
      size+=curpos;
      printf("Size: %x\n",size);
      // Get the offset
      xoff=fgetc(infile)-3;
      yoff=fgetc(infile);
      x=xoff*8;
      y=yoff;
      printf("xoff: %d yoff: %d\n",xoff,yoff);

      // Get the x length
      xlen=fgetc(infile);
      ylen=fgetc(infile);
      if (countflag) ylen-=2;
      printf("xlen: %x ylen: %x\n",xlen, ylen);

      // Get the palette
      printf("Colours: ");
      for (i=1;i<4;i++)
      {
         work=fgetc(infile);
         set_color(i,&colours[work]);
         set_color(i,&colours[work]);
         printf("%d ",work);
      }
      work=fgetc(infile);
      set_color(0,&colours[work]);
      printf("%d\n",work);
      get_palette(pal);

      //fseek(infile,offset+10,SEEK_SET);
      j=0;
      while (!feof(infile) && ftell(infile)<size)
      {
         // First get count
         c=fgetc(infile);

         if (((c & 0x80) == 0x80) || countflag)
         { // is a counter
            if (!countflag) c &= 0x7f;
            if (countflag) c-=1;
            work=fgetc(infile);
            work2=fgetc(infile);
            for (i=0;i<c+1;i++)
            {
               drawpixels(work,work2);
            }
         }
         else
         {
            // Don't count on the next j characters

            for (i=0;i<c+1;i++)
            {
               work=fgetc(infile);
               work2=fgetc(infile);
               drawpixels(work,work2);
            }
         }
      }
      sprintf(filename,"Output\\Output%d.bmp",outpic);
      outpic++;
      save_bmp(filename, savescr, pal);

      // Now go to the next image
      fseek(infile,size,SEEK_SET);
      set_color(0,&black);
      rectfill(savescr, 0, 0, 280, 159, 0);
   }
   fclose(infile);
   allegro_exit();
}



