#include <stdio.h>
#include <allegro.h>

#define XMIN 0
#define XMAX 255
#define YMIN 0
#define YMAX 127
#define LOOKTABLE 0x8c1b

int main(int argc, char **argv)
{
   FILE *infile,*ptrfile,*debugfile;
   unsigned int action,cx,cy,x=0,y,tx,ty,memory;
   int lengthaxis, decx, decy, axis, length;
   int dontread=0;
   int i,j,k,pcolour;
   int chptr=0;
   int colour, fcolour, bcolour;
   int colcount, coldir, coltemp;
   int image=0, finished, offset;
   char filename[255];
   BITMAP *work;
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

   /* set up the palette */

   //set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0);
   work=create_bitmap(XMAX-XMIN,YMAX-YMIN);
   set_color(0,&black);
   set_color(1,&blue);
   set_color(2,&red);
   set_color(3,&magenta);
   set_color(4,&green);
   set_color(5,&cyan);
   set_color(6,&yellow);
   set_color(7,&white);
   get_palette(pal);

   ptrfile=fopen(argv[1],"rb");
   infile=fopen(argv[1],"rb");
   debugfile=fopen("debug.txt","w");
   /* Seek to start of graphics data */

   // Draw box
   fseek(ptrfile,LOOKTABLE,SEEK_SET);

   /* Produce a border so that the fills work properly! */
   rectfill(work, 0, 0, XMAX, YMAX, 7);

   do
   {
      // Anybody know why this is little endian?
      // Don't know what this byte is;
      printf("Drawing Image %d\n",image);
      fprintf(debugfile,"Drawing Image %d %x\n",image, ftell(ptrfile));
      action=fgetc(ptrfile);
      if (action == 0xff) break;
      offset=fgetc(ptrfile)+(fgetc(ptrfile)*256);
      fprintf(debugfile,"Offset (absolute): %x\n",offset);
      offset-=0x3fe5;
      printf("Offset: %x\n",offset);
      fprintf(debugfile,"Offset: %x\n",offset);
      fseek(infile,offset,SEEK_SET);

      // ignore border
      action=fgetc(infile);
      tx=fgetc(infile);
      fcolour=(tx & 0x07);
      bcolour=(tx & 0x38)>>3;
      if (fcolour == bcolour) fcolour=7;
      fprintf(debugfile,"Setting colours to %x %d %d\n", tx, bcolour, fcolour);
      rectfill(work, XMIN, YMIN, XMAX, YMAX, bcolour);
      finished=0;

      do {
         action=fgetc(infile);
         if (action == 0x08)
         {
            /* Move */

            x=fgetc(infile);
            y=fgetc(infile);
            fprintf(debugfile,"Moving to %d %d\n", x, y);
         }
         else if (action & 0x80)
         {
            /* Draw */
            lengthaxis = (action & 0x78) >> 1;
            decx = (action & 0x04) >> 2;
            decy = (action & 0x02) >> 1;
            axis = (action & 0x01);
            action=fgetc(infile);
            lengthaxis+=((action & 0xc0) >> 6);
            length=(action & 0x3f);
            fprintf(debugfile,"Drawing: %d %d %d %d %d\n", lengthaxis, decx, decy, axis, length);
            // Now try and draw it
            j=lengthaxis;
            for (i=0; i <= length; i++)
            {
               putpixel(work,x+XMIN,YMAX-y,fcolour);
               if (axis)
               { // We mangle y
                  y+=(decy==1)?-1:1;
               }
               else
               {
                  x+=(decx==1)?-1:1;
                  if (x<0 || x==255) break;
               }
               j--;
               if (j<0)
               {
                  if (axis)
                  {
                     x+=(decx==1)?-1:1;
                  if (x<0 || x==255) break;
                  }
                  else
                  { // We mangle y
                     y+=(decy==1)?-1:1;
                  }
                  j=lengthaxis;
               }
            }
         }
         else if (action == 0x00)
         {
            // Do nothing
            fprintf(debugfile,"Image Finished %x\n",ftell(infile));
            finished=1;
         }
         else if (action & 0x40)
         {
            // Flood Fill
            colour = action & 0x00f;
            cx = fgetc(infile);
            cy = fgetc(infile);
            if (cy == 0) cy=1;
            if (cx == 0) cx=1;
            floodfill(work,cx+XMIN,YMAX-cy,colour);
            fprintf(debugfile,"Colour %x %x %d %d %d\n", ftell(infile), action, colour, cx, cy);
         }
         else if (action & 0x20)
         {
            // Mangle blocks directly
            // Note as these are memory blocks we don't need to invert y
            colour = action & 0x0f;
            memory = (fgetc(infile)*256+fgetc(infile)) - 0x5800;
            cy=((memory / 32))*8;
            cx=(memory % 32)*8;
            fprintf(debugfile,"Memory Location: %x %d %d\n", memory, cx, cy);
            do
            {
               coltemp = fgetc(infile);
               if (coltemp == 0xff) break;
               colcount = ((coltemp>>2)&0x3f);
               coldir = (coltemp & 0x03);
               fprintf(debugfile,"Count and dir: %d %d\n", colcount, coldir);
               for (k=0; k<=colcount; k++)
               {
                  for (i=cx; i<cx+8; i++)
                  {
                     for (j=cy; j<cy+8; j++)
                     {
                        pcolour=getpixel(work,i+XMIN, j);
                        if (fcolour == colour && pcolour == colour) putpixel(work,i+XMIN, j, fcolour ^ 0x07);
                        if (pcolour == bcolour) putpixel(work,i+XMIN, j, colour);
                     }
                  }
                  switch(coldir)
                  {
                     case 0 : cy-=8; break;
                     case 1 :
                     {
                        cx+=8;
                        if (cx > 255)
                        {
                           cy+=8;
                           cx=0;
                        }
                        break;
                     }
                     case 2 : cy+=8; break;
                     case 3 : cx-=8; break;
                  }
               }
            } while (coltemp != 0xff);
         }
         else if (action == 0x00)
         {
            finished=1;
         }
         else
         { // Unknown directive - finish the image and write to debug
            fprintf(debugfile,"Unknown opcode: %x at %x\n",action,ftell(infile));
            finished=1;
         }
      } while (!finished);
      fprintf(debugfile,"Finished image\n");
      sprintf(filename,"image%d.bmp",image);
      printf("Saving image %d as %s\n",image,filename);
      save_bmp(filename, work, pal);
      image++;
   } while (action!=0xff);
   allegro_exit();
   fclose(infile);
   fclose(ptrfile);
   fclose(debugfile);
}

END_OF_MAIN();
