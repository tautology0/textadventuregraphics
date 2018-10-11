/* Routine to draw the C64 RLE graphics

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

   //printf("Plotting at %d %d: %x %x\n",x,y,pattern,pattern2);
   // Now get colours
   //if (x>(xlen)*8)
    //  return;
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
   FILE *tabptr;
   PALETTE pal;
   RGB black = { 0, 0, 0 };
   RGB white = { 63, 63, 63 };
   RGB red = { 26, 13, 10 };
   RGB cyan = { 28, 41, 44 };
   RGB purple = { 27, 15, 33 };
   RGB green = { 22, 35, 16 };
   RGB blue = { 13, 10, 30 };
   RGB yellow = { 46, 49, 27 };
   RGB orange = { 27, 19, 9 };
   RGB brown = { 16, 14, 0 };
   RGB lred = { 38, 25, 22 };
   RGB dgrey = { 17, 17, 17 };
   RGB grey = { 27, 27, 27 };
   RGB lgreen = { 38, 52, 33 };
   RGB lblue = { 27, 23, 45 };
   RGB lgrey = { 37, 37, 37 };

   colours[0]=black;
   colours[1]=white;
   colours[2]=red;
   colours[3]=cyan;
   colours[4]=purple;
   colours[5]=green;
   colours[6]=blue;
   colours[7]=yellow;
   colours[8]=orange;
   colours[9]=brown;
   colours[10]=lred;
   colours[11]=dgrey;
   colours[12]=grey;
   colours[13]=lgreen;
   colours[14]=lblue;
   colours[15]=lgrey;

   allegro_init();
   install_keyboard();
   savescr=create_bitmap(280,78);
   /* set up the palette */
   //for (i=0;i<256;i++)
   //{
   //   colours[i].r=0; colours[i].g=0;colours[i].b=0;
   //}
   //setuppalette();

   infile=fopen(argv[1],"rb");
   tabptr=fopen(argv[1],"rb");
   offset=strtol(argv[2],NULL,0);
   count=strtol(argv[3],NULL,0);
   //if (strcmp(argv[4],"y")==0) countflag=1;
   //printf("%d\n",countflag);

   // Get the size of the graphics chuck
   fseek(tabptr, offset, SEEK_SET);
   work=fgetc(tabptr);
   size=work+(fgetc(tabptr)*256);
   size+=0x80;
   printf("Size: %4x\n",size);

   // Now loop round for each image
   while (outpic<count)
   {
      printf("\nRendering Image %d\n",outpic);
      x=0;y=0;
      fseek(infile, size, SEEK_SET);
      work=fgetc(tabptr);
      size=work+(fgetc(tabptr)*256);
      size+=0x80;
      printf("Next size: %4x\n",size);

      // Get the offset
      xoff=fgetc(infile)-3;
      if (xoff<0) xoff=8;
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
      for (i=0;i<4;i++)
      {
         work=fgetc(infile);
         set_color(i,&colours[work]);
         set_color(i,&colours[work]);
         printf("%d ",work);
      }
      //work=fgetc(infile);
      //set_color(0,&colours[work]);
      printf("\n");
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
      sprintf(filename,"Output\\Output%2i.bmp",outpic);
      outpic++;
      save_bmp(filename, savescr, pal);

      // Now go to the next image
      printf("Current Offset: %4x\n",ftell(infile));
      //fseek(infile,size,SEEK_SET);
      set_color(0,&black);
      rectfill(savescr, 0, 0, 280, 159, 0);
   }
   fclose(infile);
   allegro_exit();
}

END_OF_MAIN();



