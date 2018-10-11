#include <stdio.h>
#include <strings.h>
#include <allegro.h>

BITMAP *work_sprite,*work_sprite2;
FILE *outfile;
unsigned char sprite[255][8];
unsigned char screenchars[600][8];
int previouschar, previousflip;
BITMAP *savescr;

void flip(unsigned char character[])
{
   int i,j;
   unsigned char work2[8]={0,0,0,0,0,0,0,0};

   for (i=0;i<8;i++)
      for (j=0;j<8;j++)
         if ((character[i] & (1<<j)) != 0) work2[i]+=1<<(7-j);
   for (i=0;i<8;i++) character[i]=work2[i];
}

void rot90(unsigned char character[])
{
   int i,j;
   unsigned char work2[8]={0,0,0,0,0,0,0,0};

   for (i=0;i<8;i++)
      for (j=0;j<8;j++)
         if ((character[j] & (1<<i)) != 0) work2[7-i]+=1<<j;

   for (i=0;i<8;i++) character[i]=work2[i];
}

void rot270(unsigned char character[])
{
   int i,j;
   unsigned char work2[8]={0,0,0,0,0,0,0,0};

   for (i=0;i<8;i++)
      for (j=0;j<8;j++)
         if ((character[j] & (1<<i)) != 0) work2[i]+=1<<(7-j);

   for (i=0;i<8;i++) character[i]=work2[i];
}

void rot180(unsigned char character[])
{
   int i,j;
   unsigned char work2[8]={0,0,0,0,0,0,0,0};

   for (i=0;i<8;i++)
      for (j=0;j<8;j++)
         if ((character[i] & (1<<j)) != 0) work2[7-i]+=1<<(7-j);

   for (i=0;i<8;i++) character[i]=work2[i];
}

void background(int x, int y, int ink)
{
   /* Draw the background */
   rectfill(savescr, x*8, y*8, (x+1)*8-1, (y+1)*8-1, ink);
}

void transform(int character, int flip_mode, int ptr)
{
   unsigned char work[8];
   int i;

   printf("Plotting char: %d with flip: %x at %d: %d,%d\n",character,flip_mode,ptr, ptr%0x20, ptr/0x20);

   // first copy the character into work
   for (i=0; i<8; i++) work[i]=sprite[character][i];

   // Now flip it
   if ((flip_mode & 0x30) == 0x10) {rot90(work); /*printf("rot 90 character %d\n",character)*/;}
   if ((flip_mode & 0x30) == 0x20) {rot180(work); /*printf("rot 180 character %d\n",character)*/;}
   if ((flip_mode & 0x30) == 0x30) {rot270(work); /*printf("rot 270 character %d\n",character)*/;}
   if ((flip_mode & 0x40) != 0) { flip(work); /*printf("flipping character %d\n",character)*/; }
   flip(work);

   // Now mask it onto the previous character

   for (i=0;i<8;i++)
   {
      if ((flip_mode & 0x0c) == 12 ) screenchars[ptr][i]^=work[i];
      else if ((flip_mode & 0x0c) == 8) screenchars[ptr][i]&=work[i];
      else if ((flip_mode & 0x0c) == 4) screenchars[ptr][i]|=work[i];
      else screenchars[ptr][i]=work[i];
   }
}

void plotsprite(int character, int x, int y, int pen, int ink)
{
    int i,j;
    //background(x,y,ink);
    for (i=0;i<8;i++)
       for (j=0;j<8;j++)
          if ((screenchars[character][i] & (1<<j)) != 0) putpixel(savescr,x*8+j,y*8+i, 7 /*pen*/);
}


int main(int argc, char **argv)
{
   FILE *infile;
   char filename[256];
   int ptr,cont=0;
   int i,x,y,mask_mode;
   int graphics, CHAR_START, OFFSET_TAB, numgraphics;
   signed int TAB_OFFSET;
   int xsize,ysize,xoff,yoff;
   int starttab, version;
   unsigned char data, data2, old=0;
   int pen[0x22][12],ink[0x22][12];

   PALETTE pal;

   if (argc != 3)
   {
      printf("Not enough parameters %d: file game\n", argc);
      exit(1);
   }
   infile=fopen(argv[1],"rb");

   TAB_OFFSET=0x3fe5;
   if (strcmp(argv[2],"s1") == 0)
   {
      CHAR_START=0x631e; // Adventureland
      numgraphics=41;
      version=1;
   }
   else if (strcmp(argv[2],"s3") == 0)
   {
      CHAR_START=0x625d; // Mission Impossible
      numgraphics=44;
      version=1;
   }
   else if (strcmp(argv[2],"q2") == 0)
   {
      CHAR_START=0x6296; // Spiderman
      numgraphics=41;
      version=2;
   }
   else if (strcmp(argv[2],"s13") == 0)
   {
      CHAR_START=0x6007; // Sorceror of Claymorgue Castle
      numgraphics=38;
      version=2;
   }
   else if (strcmp(argv[2],"s10") == 0)
   {
      CHAR_START=0x570f; // Savage Island 1
      numgraphics=37;
      TAB_OFFSET=0;
      version=3;
   }
   else if (strcmp(argv[2],"s11") == 0)
   {
      CHAR_START=0x57d0; // Savage Island 2
      numgraphics=21;
      TAB_OFFSET=0;
      version=3;
   }
   else if (strcmp(argv[2],"o1") == 0)
   {
      CHAR_START=0x5a4e; // Supergran
      numgraphics=47;
      TAB_OFFSET=0;
      version=3;
   }
   else if (strcmp(argv[2],"o2") == 0)
   {
      CHAR_START=0x5be1; // Gremlins
      numgraphics=78;
      TAB_OFFSET=0;
      version=3;
   }
   else
   {
      printf("Unknown game: %s\n",argv[2]);
      exit(1);
   }

   OFFSET_TAB=CHAR_START+0x800;

   RGB black = { 0,0,0 };
   RGB blue = { 0,0,63 };
   RGB red = { 63,0,0 };
   RGB magenta = { 63,0,63 };
   RGB green = { 0,63,0 };
   RGB cyan = { 0,63,63 };
   RGB yellow = { 63,63,0 };
   RGB white = { 63,63,63 };
   RGB dullblack = { 0,0,0 };
   RGB dullblue = { 0,0,53 };
   RGB dullred = { 53,0,0 };
   RGB dullmagenta = { 53,0,53 };
   RGB dullgreen = { 0,53,0 };
   RGB dullcyan = { 0,53,53 };
   RGB dullyellow = { 53,53,0 };
   RGB dullwhite = { 53,53,53 };

   allegro_init();
   install_keyboard();
   //set_gfx_mode(GFX_VGA , 320, 200, 0, 0);

   /* set up the palette */
   set_color(0,&dullblack);
   set_color(1,&dullblue);
   set_color(2,&dullred);
   set_color(3,&dullmagenta);
   set_color(4,&dullgreen);
   set_color(5,&dullcyan);
   set_color(6,&dullyellow);
   set_color(7,&dullwhite);
   set_color(8,&black);
   set_color(9,&blue);
   set_color(10,&red);
   set_color(11,&magenta);
   set_color(12,&green);
   set_color(13,&cyan);
   set_color(14,&yellow);
   set_color(15,&white);
   text_mode(-1);
   get_palette(pal);

   savescr=create_bitmap(255,95);
   fseek(infile,CHAR_START,SEEK_SET);
   work_sprite=create_bitmap(8,8);
   work_sprite2=create_bitmap(8,8);
   clear(work_sprite);
   clear(work_sprite2);
   outfile=fopen("debug.txt","wb");

   printf("Grabbing Character details\n");
   printf("Character Offset: %lx\n",CHAR_START);
   for (i=0; i<254; i++)
   {
      for (y=0;y<8; y++)
         sprite[i][y]=fgetc(infile);
   }

   // first build up the screen graphics
   printf("Working out colours details\n");
   for (graphics=0; graphics<numgraphics;graphics++)
   {
      printf("Rending graphics: %d\n", graphics);

      fseek(infile,OFFSET_TAB + (graphics * 2),SEEK_SET);
      starttab=(fgetc(infile)+(fgetc(infile)*256));
      printf("TAB_OFFSET: %lx\n", TAB_OFFSET);
      if (TAB_OFFSET)
      {
         starttab-=TAB_OFFSET;
      }
      else
      {
         starttab+=OFFSET_TAB+0x100;
      }
      printf("starttab: %lx\n", starttab);
      fseek(infile,starttab,SEEK_SET);

      printf("File offset %lx\n", ftell(infile));
      xsize=fgetc(infile); ysize=fgetc(infile);
      xoff=fgetc(infile); yoff=fgetc(infile);
      printf("Header: %d %d %d %d\n",xsize,ysize,xoff,yoff);

      ptr=0;
      do
      {
         int count=1,character;

         /* first handle mode */
         data=fgetc(infile);
         if (data < 0x80)
         {
            character=data;
            printf("******* SOLO CHARACTER: %lx\n",character);
            printf("Location: %lx\n",ftell(infile));
            transform(character, 0, ptr);
            ptr++;
         }
         else
         {
            // first check for a count
            if ((data & 2) == 2)
            {
               count=fgetc(infile) + 1;
            }

            // Next get character and plot (count) times
            character=fgetc(infile);

            // Plot the initial character
            if ((data & 1) == 1) character +=128;
            printf("Location: %lx\n",ftell(infile));
            for ( i = 0; i < count; i++)
               transform(character, (data & 0x0c)?(data & 0xf3):data, ptr + i);

            // Now check for overlays
            if ((data & 0xc) != 0)
            {
               // We have overlays so grab each member of the stream and work out what to
               // do with it

               mask_mode=(data & 0xc);
               data2=fgetc(infile);
               old=data;
               do
               {
                  cont=0;
                  if (data2 < 0x80)
                  {
                     if ((old & 1) == 1) data2 +=128;
                     printf("Plotting %d directly (overlay) at %d\n",data2,ptr);
                     printf("Location: %lx\n",ftell(infile));
                     for ( i = 0; i < count; i++)
                        transform(data2, old & 0x0c, ptr + i);
                  }
                  else
                  {
                     character=fgetc(infile);
                     if ((data2 & 1) == 1) character +=128;
                     printf("Plotting %d with flip %lx at %d %d\n",character,data2 | mask_mode,ptr,count);
                     printf("Location: %lx\n",ftell(infile));
                     for ( i = 0; i < count; i++)
                        transform(character, (data2 & 0xf3) | mask_mode, ptr + i);
                     if ((data2 & 0x0c) != 0) { mask_mode=(data2 & 0x0c); old=data2; cont=1; data2=fgetc(infile); }
                  }
               } while (cont != 0);
            }
            ptr+=count;
         }
      } while (ptr < xsize*ysize);

      printf("Start of colours: %lx\n", ftell(infile));
      y=0; x=0;
      while (y<ysize)
      {
         int count,colour;
         data=fgetc(infile);
         if ((data & 0x80))
         {
            count=(data & 0x7f) + 1;
            colour=fgetc(infile);
         }
         else
         {
            count=1;
            colour=data;
         }
         while (count > 0)
         {
            ink[x][y]=colour & 0x07;
            pen[x][y]=((colour & 0x70) >> 4);

            if ((colour & 0x08) == 0x08 || version==1)
            {
               pen[x][y]+=8;
               ink[x][y]+=8;
            }

            x++;
            if (x==xsize)
            {
               x=0; y++;
            }
            count--;
         }
      }

      ptr=0;
      for (y=0; y<ysize; y++)
         for (x=0; x<xsize;x++)
         {
            int xoff2=xoff;
            if (TAB_OFFSET) xoff2=xoff-4;
            plotsprite(ptr,x+xoff2,y+yoff,pen[x][y],ink[x][y]);
            ptr++;
         }

      fprintf(outfile,"Resulting Offset: %lx\n",ftell(infile));
      sprintf(filename,"image%s-%d.bmp",argv[2],graphics);
      printf("Saving image as %s\n",filename);
      save_bmp(filename, savescr, pal);
      rectfill(savescr, 0, 0, 255, 95, 0);
   }
   fclose(infile);
   fclose(outfile);
   allegro_exit();
   return(0);
}
