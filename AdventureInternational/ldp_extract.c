/* 
  ldp_extract.c

  creates linedraw gfx (and gfx scripts) for Brian Howarth's
  Mysterious Adventures

  Original code by David Lodge 19/01/2000

  Reworked by Paul David Doherty

  1.0:   9 Oct 2004
  1.1:  13 Oct 2004  log output and palettes fixed
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allegro.h>

FILE *infile, *oldoutfile, *xmloutfile;
unsigned long int startpos = 0;

#define ERRORS_BEFORE_EXIT 100
int errorcount = 0;

#define NONE 0
#define ZX 1
#define C64 2
#define VGA 3
int palchosen = NONE;
PALETTE pal;
#define INVALIDCOLOR 16

void
getstart (char *startposs)
{
  int i;
  unsigned long j;

  if (strlen (startposs) != 4)
    {
      fprintf (stderr, "wrong start format\n");
      exit (5);
    }
  for (i = 0; i < 4; i++)
    {
      if ((startposs[i] > 47) && (startposs[i] < 58))
	j = startposs[i] - 48;
      else if ((startposs[i] > 64) && (startposs[i] < 71))
	j = startposs[i] - 55;
      else if ((startposs[i] > 96) && (startposs[i] < 103))
	j = startposs[i] - 87;
      else
	{
	  fprintf (stderr, "wrong start format\n");
	  exit (5);
	}

      j = j << (4 * (3 - i));
      startpos += j;
    }
}

void
colrange (int c)
{
  if ((c < 0) || (c > 15))
    {
      fprintf (stderr, "# col out of range: %d\n", c);
      fprintf (oldoutfile, "# col out of range: %d\n", c);
      errorcount++;
    }
}

void
checkrange (int x, int y)
{
  if ((x < 0) || (x > 254))
    {
      fprintf (stderr, "# x out of range: %d\n", x);
      fprintf (oldoutfile, "# x out of range: %d\n", x);
      errorcount++;
    }
  if ((y < 96) || (y > 191))
    {
      fprintf (stderr, "# y out of range: %d\n", y);
      fprintf (oldoutfile, "# y out of range: %d\n", y);
      errorcount++;
    }
}

void
do_palette (char *palname)
{

  if (strcmp ("zx", palname) == 0)
    palchosen = ZX;
  else if (strcmp ("c64", palname) == 0)
    palchosen = C64;
  else if (strcmp ("vga", palname) == 0)
    palchosen = VGA;
}

void
define_palette (void)
{
  /* set up the palette */
  /* actually only colors 0-7 used, but what the ... */
  if (palchosen == VGA)
    {
      RGB black = { 0, 0, 0 };
      RGB blue = { 0, 0, 63 };
      RGB red = { 63, 0, 0 };
      RGB magenta = { 63, 0, 63 };
      RGB green = { 0, 63, 0 };
      RGB cyan = { 0, 63, 63 };
      RGB yellow = { 63, 63, 0 };
      RGB white = { 63, 63, 63 };
      RGB brblack = { 0, 0, 0 };
      RGB brblue = { 0, 0, 63 };
      RGB brred = { 63, 0, 0 };
      RGB brmagenta = { 63, 0, 63 };
      RGB brgreen = { 0, 63, 0 };
      RGB brcyan = { 0, 63, 63 };
      RGB bryellow = { 63, 63, 0 };
      RGB brwhite = { 63, 63, 63 };

      set_color (0, &black);
      set_color (1, &blue);
      set_color (2, &red);
      set_color (3, &magenta);
      set_color (4, &green);
      set_color (5, &cyan);
      set_color (6, &yellow);
      set_color (7, &white);
      set_color (8, &brblack);
      set_color (9, &brblue);
      set_color (10, &brred);
      set_color (11, &brmagenta);
      set_color (12, &brgreen);
      set_color (13, &brcyan);
      set_color (14, &bryellow);
      set_color (15, &brwhite);
    }
  else if (palchosen == ZX)
    {
/* corrected Sinclair ZX palette */
      RGB black = { 0, 0, 0 };
      RGB blue = { 0, 0, 38 };
      RGB red = { 38, 0, 0 };
      RGB magenta = { 38, 0, 38 };
      RGB green = { 0, 38, 0 };
      RGB cyan = { 0, 38, 38 };
      RGB yellow = { 38, 38, 0 };
      RGB white = { 38, 38, 38 };
      RGB brblack = { 0, 0, 0 };
      RGB brblue = { 0, 0, 42 };
      RGB brred = { 46, 0, 0 };
      RGB brmagenta = { 51, 0, 51 };
      RGB brgreen = { 0, 51, 0 };
      RGB brcyan = { 0, 55, 55 };
      RGB bryellow = { 59, 59, 0 };
      RGB brwhite = { 63, 63, 63 };

      set_color (0, &black);
      set_color (1, &blue);
      set_color (2, &red);
      set_color (3, &magenta);
      set_color (4, &green);
      set_color (5, &cyan);
      set_color (6, &yellow);
      set_color (7, &white);
      set_color (8, &brblack);
      set_color (9, &brblue);
      set_color (10, &brred);
      set_color (11, &brmagenta);
      set_color (12, &brgreen);
      set_color (13, &brcyan);
      set_color (14, &bryellow);
      set_color (15, &brwhite);
    }
  else if (palchosen == C64)
    {
/* and now: experimental C64 palette :-) */
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

      set_color (0, &black);
      set_color (1, &white);
      set_color (2, &red);
      set_color (3, &cyan);
      set_color (4, &purple);
      set_color (5, &green);
      set_color (6, &blue);
      set_color (7, &yellow);
      set_color (8, &orange);
      set_color (9, &brown);
      set_color (10, &lred);
      set_color (11, &dgrey);
      set_color (12, &grey);
      set_color (13, &lgreen);
      set_color (14, &lblue);
      set_color (15, &lgrey);
    }

  text_mode (-1);
  get_palette (pal);
}

int
remap (int color)
{
/* todo: determine mapping scheme */

  if (palchosen == ZX)
    {
      /* nothing to remap here; shows that the gfx were created on a ZX */
      return (((color >= 0) && (color <= 15)) ? color : INVALIDCOLOR);
    }
  else if (palchosen == C64)
    {
      /* remaps for 8-15 can't be determined, so set to white */
      int c64remap[] =
	{ 0, 6, 2, 4, 5, 3, 7, 1, /* end */ 1, 1, 1, 1, 1, 1, 1, 1, };
      return (((color >= 0)
	       && (color <= 15)) ? c64remap[color] : INVALIDCOLOR);
    }
  else
    return (((color >= 0) && (color <= 15)) ? color : INVALIDCOLOR);
}

char *
colortext (int col)
{
  char *zxcolorname[] =
    { "black", "blue", "red", "magenta", "green", "cyan", "yellow", "white",
    "bright black", "bright blue", "bright red", "bright magenta",
    "bright green", "bright cyan", "bright yellow", "bright white", "invalid",
  };

  char *c64colorname[] =
    { "black", "white", "red", "cyan", "purple", "green", "blue", "yellow",
    "orange", "brown blue", "light red", "dark grey",
    "grey", "light green", "light blue", "light grey", "invalid",
  };

  if (palchosen == C64)
    return (c64colorname[col]);
  else
    return (zxcolorname[col]);
}

int
main (int argc, char **argv)
{
  char filename[256];
  int count, work;
  int x, y, c;
  int ox = 0, oy = 0, oc = 0;
  int rooms;

  BITMAP *savescr;
  int SWIDTH, SHEIGHT;

  if (argc != 5)
    {
      fprintf (stderr,
	       "usage: ldp_extract filename $startaddr #ofrooms palette\n  e.g. ldp_extract baton.sna 5ba7 31 zx\nAccepted palettes: zx, c64, vga");
      exit (5);
    }

  getstart (argv[2]);
  rooms = atoi (argv[3]);
  do_palette (argv[4]);

  if (palchosen == NONE)
    {
      fprintf (stderr, "unknown palette\n");
      exit (5);
    }

  define_palette ();

  infile = fopen (argv[1], "rb");

  strcpy (filename, argv[1]);
  oldoutfile = fopen (strcat (filename, ".dat"), "wa");

  strcpy (filename, argv[1]);
  xmloutfile = fopen (strcat (filename, ".xml"), "wa");

  fseek (infile, startpos, SEEK_SET);

  allegro_init ();
  install_keyboard ();

//   set_gfx_mode(GFX_AUTODETECT , 320, 200, 0, 0);
  SWIDTH = 255;
  SHEIGHT = 192;
  savescr = create_bitmap (SWIDTH, SHEIGHT);

// xml header stuff
  fprintf (xmloutfile, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
  fprintf (xmloutfile, "<!DOCTYPE game SYSTEM \"game.dtd\">\n\n");
  fprintf (xmloutfile, "<game name=\"???\">\n");

  count = 0;
  do
    {
      work = fgetc (infile);
      switch (work)
	{
	case 0xc0:
	  y = fgetc (infile);
	  x = fgetc (infile);
	  // rendering
	  ox = x;
	  oy = y;
	  // logging
	  fprintf (oldoutfile, "MOVE %d,%d\n", x, y);
	  // xml logging
	  fprintf (xmloutfile, "    <move x=\"%d\" y=\"%d\"/>\n", x, y);
	  // check range
	  checkrange (x, y);
	  break;

	case 0xc1:
	  c = fgetc (infile);
	  y = fgetc (infile);
	  x = fgetc (infile);
	  // rendering
	  floodfill (savescr, x, SHEIGHT - y, remap (c));
	  // logging
	  fprintf (oldoutfile, "FILL %d,%d,%d\n", remap (c), x, y);
	  // xml logging
	  fprintf (xmloutfile, "    <fill c=\"%s\" x=\"%d\" y=\"%d\"/>\n",
		   colortext (remap (c)), x, y);
	  // check range
	  checkrange (x, y);
	  colrange (c);
	  break;

	case 0xff:
	  c = fgetc (infile);
	  // finish old picture
	  if (count != 0)
	    {
	      sprintf (filename, "image%02d.bmp", count);
	      fprintf (stderr, "Saving image as %s\n", filename);
	      save_bmp (filename, savescr, pal);
	      // finish pic's xml logging
	      fprintf (xmloutfile, "</picture>\n");
	    }
	  count++;
	  // rendering
	  clear (savescr);
	  /* set up the line colour */
	  oc = (c == 0) ? 7 : 0;
	  rectfill (savescr, 0, SHEIGHT - 191, 254, SHEIGHT - 96, remap (c));
	  /* Produce a border so that the fills work properly! */
	  line (savescr, 0, SHEIGHT - 191, 254, SHEIGHT - 191, remap (oc));
	  line (savescr, 254, SHEIGHT - 191, 254, SHEIGHT - 96, remap (oc));
	  line (savescr, 254, SHEIGHT - 96, 0, SHEIGHT - 96, remap (oc));
	  line (savescr, 0, SHEIGHT - 96, 0, SHEIGHT - 191, remap (oc));
	  // logging
	  fprintf (oldoutfile, "\nPICTURE %d\n", count);
	  fprintf (oldoutfile, "BGROUND %d\n", remap (c));
	  // xml logging
	  if (count < (rooms + 1))
	    fprintf (xmloutfile,
		     "\n<picture id=\"%d\" bground=\"%s\" fground=\"???\">\n",
		     count, colortext (remap (c)));
	  // check range
	  colrange (c);
	  break;

	default:
	  y = work;
	  x = fgetc (infile);
	  // rendering
	  line (savescr, ox, SHEIGHT - oy, x, SHEIGHT - y, remap (oc));
	  ox = x;
	  oy = y;
	  // logging
	  fprintf (oldoutfile, "DRAW %d,%d\n", x, y);
	  // xml logging
	  fprintf (xmloutfile, "    <draw x=\"%d\" y=\"%d\"/>\n", x, y);
	  // check range
	  checkrange (x, y);
	  break;
	}
    }
  while ((count < (rooms + 1)) && (errorcount < ERRORS_BEFORE_EXIT));

  // xml footer stuff
  fprintf (xmloutfile, "\n</game>\n");

  fclose (infile);
  allegro_exit ();
  return (0);
}

END_OF_MAIN ();
