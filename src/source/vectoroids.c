/*
  vectoroids.c
  
  An asteroid shooting game with vector graphics.
  Based on "Agendaroids."
  
  by Bill Kendrick
  bill@newbreedsoftware.com
  http://www.newbreedsoftware.com/vectoroids/
  
  November 30, 2001 - April 20, 2002
  
  Wii port by MiniK
  February 19th, 2009
*/

#define VER_VERSION "1.1.0"
#define VER_DATE "2002.04.20"

#ifndef EMBEDDED
#define STATE_FORMAT_VERSION "2001.12.01"
#else
#define STATE_FORMAT_VERSION "2001.12.01e"
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef VITA
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#else
#include <SDL.h>
#include <SDL_image.h>
#endif

#ifndef NOSOUND

#ifdef VITA
#include <SDL/SDL_mixer.h>
#else
#include <SDL_mixer.h>
#endif
#else
#include <SDL_mixer.h>
#endif

#ifndef DATA_PREFIX
#define DATA_PREFIX "data/"
#endif


/* Constraints: */

#ifndef EMBEDDED
  #define NUM_BULLETS 2
#else
  #define NUM_BULLETS 3
#endif

#ifndef EMBEDDED
  #define NUM_ASTEROIDS 15
  #define NUM_BITS 25
#else
  #define NUM_ASTEROIDS 15
  #define NUM_BITS 25
#endif

#define AST_SIDES 6
#ifndef EMBEDDED
  #define AST_RADIUS 5
  #define SHIP_RADIUS 10
#else
  #define AST_RADIUS 7
  #define SHIP_RADIUS 12
#endif

#define ZOOM_START 40
#define ONEUP_SCORE 10000
#define FPS 60

#ifndef EMBEDDED
  #define WIDTH 320
  #define HEIGHT 240
#else
  #define WIDTH 240
  #define HEIGHT 320
#endif

enum { FALSE, TRUE };

#define LEFT_EDGE   0x0001
#define RIGHT_EDGE  0x0002
#define TOP_EDGE    0x0004
#define BOTTOM_EDGE 0x0008


#ifdef VITA

#define VITA_BTN_TRIANGLE 0
#define VITA_BTN_CIRCLE 1
#define VITA_BTN_CROSS 2
#define VITA_BTN_SQUARE 3
#define VITA_BTN_LTRIGGER 4
#define VITA_BTN_RTRIGGER 5
#define VITA_BTN_DOWN 6
#define VITA_BTN_LEFT 7
#define VITA_BTN_UP 8
#define VITA_BTN_RIGHT 9
#define VITA_BTN_SELECT 10
#define VITA_BTN_START 11

#else
#include <wiiuse/wpad.h>
#endif
/* Types: */

typedef struct letter_type {
  int x, y;
  int xm, ym;
} letter_type;

typedef struct bullet_type {
  int timer;
  int x, y;
  int xm, ym;
} bullet_type;

typedef struct shape_type {
  int radius;
  int angle;
} shape_type;

typedef struct asteroid_type {
  int alive, size;
  int x, y;
  int xm, ym;
  int angle, angle_m;
  shape_type shape[AST_SIDES];
} asteroid_type;

typedef struct bit_type {
  int timer;
  int x, y;
  int xm, ym;
} bit_type;

typedef struct color_type {
  Uint8 r;
  Uint8 g;
  Uint8 b;
} color_type;


/* Data: */

enum {
  SND_BULLET,
  SND_AST1,
  SND_AST2,
  SND_AST3,
  SND_AST4,
  SND_THRUST,
  SND_EXPLODE,
  SND_GAMEOVER,
  SND_EXTRALIFE,
  NUM_SOUNDS
};

char * sound_names[NUM_SOUNDS] = {
  DATA_PREFIX "sounds/bullet.wav",
  DATA_PREFIX "sounds/ast1.wav",
  DATA_PREFIX "sounds/ast2.wav",
  DATA_PREFIX "sounds/ast3.wav",
  DATA_PREFIX "sounds/ast4.wav",
  DATA_PREFIX "sounds/thrust.wav",
  DATA_PREFIX "sounds/explode.wav",
  DATA_PREFIX "sounds/gameover.wav",
  DATA_PREFIX "sounds/extralife.wav"
};

#define CHAN_THRUST 0

char * mus_game_name = DATA_PREFIX "music/decision.s3m";


#ifdef JOY_YES
#ifdef VITA
#define JOY_A VITA_BTN_CIRCLE
#define JOY_B VITA_BTN_CROSS
#else
#define JOY_A 0
#define JOY_B 1
#endif
#define JOY_X 0
#define JOY_Y 1
#endif


/* Globals: */

SDL_Surface * screen, * bkgd;
#ifndef NOSOUND
Mix_Chunk * sounds[NUM_SOUNDS];
Mix_Music * game_music;
#endif
#ifdef JOY_YES
SDL_Joystick *js;
#endif
bullet_type bullets[NUM_BULLETS];
asteroid_type asteroids[NUM_ASTEROIDS];
bit_type bits[NUM_BITS];
int use_sound, use_joystick, fullscreen, text_zoom;
char zoom_str[24];
int x, y, xm, ym, angle;
int player_alive, player_die_timer;
int lives, score, high, level, game_pending;


/* Trig junk:  (thanks to Atari BASIC for this) */

int trig[12] = {
  1024,
  1014,
  984,
  935,
  868,
  784,
  685,
  572,
  448,
  316,
  117,
  0
};


/* Characters: */

int char_vectors[36][5][4] = {
  {
    /* 0 */
    { 0, 0, 1, 0 },
    { 1, 0, 1, 2 },
    { 1, 2, 0, 2 },
    { 0, 2, 0, 0 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* 1 */
    { 1, 0, 1, 2 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },

  {
    /* 2 */
    { 1, 0, 0, 0 },
    { 1, 0, 1, 1 },
    { 0, 1, 1, 1 },
    { 0, 1, 0, 2 },
    { 1, 2, 0, 2 },
  },

  {
    /* 3 */
    { 0, 0, 1, 0 },
    { 1, 0, 1, 2 },
    { 0, 1, 1, 1 },
    { 0, 2, 1, 2 },
    { -1, -1, -1, -1 }
  },

  {
    /* 4 */
    { 1, 0, 1, 2 },
    { 0, 0, 0, 1 },
    { 0, 1, 1, 1 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },

  {
    /* 5 */
    { 1, 0, 0, 0 },
    { 0, 0, 0, 1 },
    { 0, 1, 1, 1 },
    { 1, 1, 1, 2 },
    { 1, 2, 0, 2 }
  },

  {
    /* 6 */
    { 1, 0, 0, 0 },
    { 0, 0, 0, 2 },
    { 0, 2, 1, 2 },
    { 1, 2, 1, 1 },
    { 1, 1, 0, 1 }
  },

  {
    /* 7 */
    { 0, 0, 1, 0 },
    { 1, 0, 1, 2 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },

  {
    /* 8 */
    { 0, 0, 1, 0 },
    { 0, 0, 0, 2 },
    { 1, 0, 1, 2 },
    { 0, 2, 1, 2 },
    { 0, 1, 1, 1 }
  },

  {
    /* 9 */
    { 1, 0, 1, 2 },
    { 0, 0, 1, 0 },
    { 0, 0, 0, 1 },
    { 0, 1, 1, 1 },
    { -1, -1, -1, -1 }
  },

  {
    /* A */
    { 0, 2, 0, 1 },
    { 0, 1, 1, 0 },
    { 1, 0, 1, 2 },
    { 0, 1, 1, 1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* B */
    { 0, 2, 0, 0 },
    { 0, 0, 1, 0 },
    { 1, 0, 0, 1 },
    { 0, 1, 1, 2 },
    { 1, 2, 0, 2 }
  },

  {
    /* C */
    { 1, 0, 0, 0 },
    { 0, 0, 0, 2 },
    { 0, 2, 1, 2 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* D */
    { 0, 0, 1, 1 },
    { 1, 1, 0, 2 },
    { 0, 2, 0, 0 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* E */
    { 1, 0, 0, 0 },
    { 0, 0, 0, 2 },
    { 0, 2, 1, 2 },
    { 0, 1, 1, 1 },
    { -1, -1, -1, -1 }
  },

  {
    /* F */
    { 1, 0, 0, 0 },
    { 0, 0, 0, 2 },
    { 0, 1, 1, 1 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }    
  },
  
  {
    /* G */
    { 1, 0, 0, 0 },
    { 0, 0, 0, 2 },
    { 0, 2, 1, 2 },
    { 1, 2, 1, 1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* H */
    { 0, 0, 0, 2 },
    { 1, 0, 1, 2 },
    { 0, 1, 1, 1 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* I */
    { 1, 0, 1, 2 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* J */
    { 1, 0, 1, 2 },
    { 1, 2, 0, 2 },
    { 0, 2, 0, 1 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* K */
    { 0, 0, 0, 2 },
    { 1, 0, 0, 1 },
    { 0, 1, 1, 2 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* L */
    { 0, 0, 0, 2 },
    { 0, 2, 1, 2 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* M */
    { 0, 0, 0, 2 },
    { 1, 0, 1, 2 },
    { 0, 0, 1, 1 },
    { 0, 1, 1, 0 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* N */
    { 0, 2, 0, 0 },
    { 0, 0, 1, 2 },
    { 1, 2, 1, 0 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* O */
    { 0, 0, 1, 0 },
    { 1, 0, 1, 2 },
    { 1, 2, 0, 2 },
    { 0, 2, 0, 0 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* P */
    { 0, 2, 0, 0 },
    { 0, 0, 1, 0 },
    { 1, 0, 1, 1 },
    { 1, 1, 0, 1 },
    { -1, -1, -1, -1 }
  },
  
  { 
    /* Q */
    { 0, 0, 1, 0 },
    { 1, 0, 1, 2 },
    { 1, 2, 0, 2 },
    { 0, 2, 0, 0 },
    { 0, 1, 1, 2 }
  },

  {
    /* R */
    { 0, 2, 0, 0 },
    { 0, 0, 1, 0 },
    { 1, 0, 1, 1 },
    { 1, 1, 0, 1 },
    { 0, 1, 1, 2 }
  },
  
  {
    /* S */
    { 1, 0, 0, 0 },
    { 0, 0, 0, 1 },
    { 0, 1, 1, 1 },
    { 1, 1, 1, 2 },
    { 1, 2, 0, 2 }
  },

  {
    /* T */
    { 0, 0, 1, 0 },
    { 1, 0, 1, 2 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* U */
    { 0, 0, 0, 2 },
    { 0, 2, 1, 2 },
    { 1, 2, 1, 0 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* V */
    { 0, 0, 0, 1 },
    { 0, 1, 1, 2 },
    { 1, 2, 1, 0 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* W */
    { 0, 0, 0, 2 },
    { 1, 0, 1, 2 },
    { 0, 1, 1, 2 },
    { 0, 2, 1, 1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* X */
    { 0, 0, 1, 2 },
    { 0, 2, 1, 0 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* Y */
    { 0, 0, 1, 1 },
    { 1, 0, 1, 2 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  },
  
  {
    /* Z */
    { 0, 0, 1, 0 },
    { 1, 0, 0, 2 },
    { 0, 2, 1, 2 },
    { -1, -1, -1, -1 },
    { -1, -1, -1, -1 }
  }
};



/* Local function prototypes: */

int title(void);
int game(void);
void finish(void);
void setup(int argc, char * argv[]);
void seticon(void);
int fast_cos(int v);
int fast_sin(int v);
void draw_line(int x1, int y1, color_type c1,
	       int x2, int y2, color_type c2);
int clip(int * x1, int * y1, int * x2, int * y2);
color_type mkcolor(int r, int g, int b);
void sdl_drawline(int x1, int y1, color_type c1,
		  int x2, int y2, color_type c2);
unsigned char encode(float x, float y);
void drawvertline(int x, int y1, color_type c1,
                  int y2, color_type c2);
void putpixel(SDL_Surface * surface, int x, int y, Uint32 pixel);
void draw_segment(int r1, int a1,
		  color_type c1,
		  int r2, int a2,
		  color_type c2,
		  int cx, int cy, int ang);
void add_bullet(int x, int y, int a, int xm, int ym);
void add_asteroid(int x, int y, int xm, int ym, int size);
void add_bit(int x, int y, int xm, int ym);
void draw_asteroid(int size, int x, int y, int angle, shape_type * shape);
void playsound(int snd);
void hurt_asteroid(int j, int xm, int ym, int exp_size);
void add_score(int amount);
void draw_char(char c, int x, int y, int r, color_type cl);
void draw_text(char * str, int x, int y, int s, color_type c);
void draw_thick_line(int x1, int y1, color_type c1,
		     int x2, int y2, color_type c2);
void reset_level(void);
void show_version(void);
void show_usage(FILE * f, char * prg);
SDL_Surface * set_vid_mode(unsigned flags);
void draw_centered_text(char * str, int y, int s, color_type c);


/* --- MAIN --- */

int main(int argc, char * argv[])
{
printf("DATA_PREFIX %s\n\n\n", DATA_PREFIX);
  #ifndef VITA
  WPAD_Init();
  #endif

  int done;

  setup(argc, argv);
  

  /* Set defaults: */
  
  score = 0;
  high = 0;
  game_pending = 0;

  /* Main app loop! */
  
  do
  {
    done = title();

    if (!done)
    {
      done = game();
    }
  }
  while (!done);

  finish();

  return(0);
}


/* Title screen: */

int title(void)
{
  int done, quit;
  int i, snapped, angle, size, counter, x, y, xm, ym, z1, z2, z3;
  SDL_Event event;
  SDLKey key;
  Uint32 now_time, last_time;
  char * titlestr = "VECTOROIDS";
  char str[20];
  letter_type letters[11];


  /* Reset letters: */

  snapped = 0;
  
  for (i = 0; i < strlen(titlestr); i++)
  {
    letters[i].x = (rand() % WIDTH);
    letters[i].y = (rand() % HEIGHT);
    letters[i].xm = 0;
    letters[i].ym = 0;
  }

  x = (rand() % WIDTH);
  y = (rand() % HEIGHT);
  xm = (rand() % 4) + 2;
  ym = (rand() % 10) - 5;

  counter = 0; 
  angle = 0;
  size = 40;

  done = 0;
  quit = 0;

  do
  {
    last_time = SDL_GetTicks();
    
    counter++;


    /* Rotate rock: */
    
    angle = ((angle + 2) % 360);


    /* Make rock grow: */

    if ((counter % 3) == 0)
    {
      if (size > 1)
        size--;
    }

    
    /* Move rock: */

    x = x + xm;

    if (x >= WIDTH)
      x = x - WIDTH;

    y = y + ym;
    
    if (y >= HEIGHT)
      y = y - HEIGHT;
    else if (y < 0)
      y = y + HEIGHT;


    /* Handle events: */
	
	#ifndef VITA
	WPAD_ScanPads();
	if (WPAD_ButtonsDown(0) & (WPAD_BUTTON_2 | WPAD_BUTTON_1 | WPAD_BUTTON_PLUS)) {
		done = 1;
	} 
	if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) {
		done = 1;
		quit = 1;
	}
	#endif
	
    while (SDL_PollEvent(&event) > 0)
    {
      if (event.type == SDL_QUIT)
      {
	done = 1;
	quit = 1;
      }
      else if (event.type == SDL_KEYDOWN)
      {
        key = event.key.keysym.sym;

	if (key == SDLK_SPACE)
        {
	  done = 1;
	}
	else if (key == SDLK_ESCAPE)
	{
	  done = 1;
	  quit = 1;
	}
      }
#ifdef JOY_YES
      else if (event.type == SDL_JOYBUTTONDOWN)
	{
	  done = 1;
	}
#endif
      else if (event.type == SDL_MOUSEBUTTONDOWN)
	{
	  if (event.button.x >= (WIDTH - 50) / 2 &&
	      event.button.x <= (WIDTH + 50) / 2 &&
	      event.button.y >= 180 && event.button.y <= 195)
	    {
	      /* Start! */
	
	      game_pending = 0;
  	      done = 1;
	    }
	  else if (event.button.x >= (WIDTH - 80) / 2 &&
		   event.button.x <= (WIDTH + 80) / 2 &&
	           event.button.y >= 200 && event.button.y <= 215 &&
		   game_pending)
	    {
	      done = 1;
	    }
	}
    }


    /* Move title characters: */

    if (snapped < strlen(titlestr))
    {
      for (i = 0; i < strlen(titlestr); i++)
      {
        letters[i].x = letters[i].x + letters[i].xm;
        letters[i].y = letters[i].y + letters[i].ym;

      
        /* Home in on final spot! */
      
        if (letters[i].x > ((WIDTH - (strlen(titlestr) * 14)) / 2 +
			    (i * 14)) &&
	    letters[i].xm > -4)
  	  letters[i].xm--;
        else if (letters[i].x < ((WIDTH - (strlen(titlestr) * 14)) / 2 +
				 (i * 14)) &&
		 letters[i].xm < 4)
  	  letters[i].xm++;

        if (letters[i].y > 100 && letters[i].ym > -4)
          letters[i].ym--;
        else if (letters[i].y < 100 && letters[i].ym < 4)
          letters[i].ym++;


        /* Snap into place: */

        if (letters[i].x >= ((WIDTH - (strlen(titlestr) * 14)) / 2 +
			     (i * 14)) - 8 &&
            letters[i].x <= ((WIDTH - (strlen(titlestr) * 14)) / 2 +
			     (i * 14)) + 8 &&
  	    letters[i].y >= 92 &&
	    letters[i].y <= 108 &&
	    (letters[i].xm != 0 ||
	     letters[i].ym != 0))
        {
  	  letters[i].x = ((WIDTH - (strlen(titlestr) * 14)) / 2 + (i * 14));
   	  letters[i].xm = 0;
        
 	  letters[i].y = 100;
          letters[i].ym = 0;

	  snapped++;
        }
      }
    }


    /* Draw screen: */
    
    /* (Erase first) */
   
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
    
    
    /* (Title) */
    
    if (snapped != strlen(titlestr))
      {
	for (i = 0; i < strlen(titlestr); i++)
	  {
	    draw_char(titlestr[i], letters[i].x, letters[i].y, 10,
		      mkcolor(255, 255, 255));
	  }
      }
    else
      {
	for (i = 0; i < strlen(titlestr); i++)
	  {
	    z1 = (i + counter) % 255;
	    z2 = ((i + counter + 128) * 2) % 255;
	    z3 = ((i + counter) * 5) % 255;
	    
	    draw_char(titlestr[i], letters[i].x, letters[i].y, 10,
		      mkcolor(z1, z2, z3));
	  }
      }
    
    
    /* (Credits) */
    
    if (snapped == strlen(titlestr))
    {
      draw_centered_text("BY BILL KENDRICK", 125, 5,
		mkcolor(128, 128,128));
      draw_centered_text("NEW BREED SOFTWARE", 140, 5,
		mkcolor(96, 96, 96));
	  draw_centered_text("WII PORT BY MINIK", 155, 5,
		mkcolor(96, 96, 96)); 
		
      sprintf(str, "HIGH %.6d", high);
      draw_text(str, (WIDTH - 110) / 2, 5, 5, mkcolor(128, 255, 255));
      draw_text(str, (WIDTH - 110) / 2 + 1, 6, 5, mkcolor(128, 255, 255));

      if (score != 0 && (score != high || (counter % 20) < 10))
      {
	if (game_pending == 0)
          sprintf(str, "LAST %.6d", score);
	else
          sprintf(str, "SCR  %.6d", score);
        draw_text(str, (WIDTH - 110) / 2, 25, 5, mkcolor(128, 128, 255));
        draw_text(str, (WIDTH - 110) / 2 + 1, 26, 5, mkcolor(128, 128, 255));
      }
    }

    
    draw_text("START", (WIDTH - 50) / 2, 180, 5, mkcolor(0, 255, 0));
    
    if (game_pending)
      draw_text("CONTINUE", (WIDTH - 80) / 2, 200, 5, mkcolor(0, 255, 0));


    /* (Giant rock) */

    draw_segment(40 / size, 0, mkcolor(255, 255, 255),
		 30 / size, 30, mkcolor(255, 255, 255),
		 x, y, angle);
    draw_segment(30 / size, 30, mkcolor(255, 255, 255),
		 40 / size, 55, mkcolor(255, 255, 255),
		 x, y, angle);
    draw_segment(40 / size, 55, mkcolor(255, 255, 255),
		 25 / size, 90, mkcolor(255, 255, 255),
		 x, y, angle);
    draw_segment(25 / size, 90, mkcolor(255, 255, 255),
		 40 / size, 120, mkcolor(255, 255, 255),
		 x, y, angle);
    draw_segment(40 / size, 120, mkcolor(255, 255, 255),
		 35 / size, 130, mkcolor(255, 255, 255),
		 x, y, angle);
    draw_segment(35 / size, 130, mkcolor(255, 255, 255),
		 40 / size, 160, mkcolor(255, 255, 255),
		 x, y, angle);
    draw_segment(40 / size, 160, mkcolor(255, 255, 255),
		 30 / size, 200, mkcolor(255, 255, 255),
		 x, y, angle);
    draw_segment(30 / size, 200, mkcolor(255, 255, 255),
		 45 / size, 220, mkcolor(255, 255, 255),
		 x, y, angle);
    draw_segment(45 / size, 220, mkcolor(255, 255, 255),
		 25 / size, 265, mkcolor(255, 255, 255),
		 x, y, angle);
    draw_segment(25 / size, 265, mkcolor(255, 255, 255),
		 30 / size, 300, mkcolor(255, 255, 255),
		 x, y, angle);
    draw_segment(30 / size, 300, mkcolor(255, 255, 255),
		 45 / size, 335, mkcolor(255, 255, 255),
		 x, y, angle);
    draw_segment(45 / size, 335, mkcolor(255, 255, 255),
		 40 / size, 0, mkcolor(255, 255, 255),
		 x, y, angle);


    /* Flush and pause! */

    SDL_Flip(screen);
    
    now_time = SDL_GetTicks();
    
    if (now_time < last_time + (1000 / FPS))
      {
	SDL_Delay(last_time + 1000 / FPS - now_time);
      }
  }
  while (!done);

  return(quit);
}



/* --- GAME --- */

int game(void)
{
  int done, quit, counter;
  int i, j;
  int num_asteroids_alive;
  SDL_Event event;
  SDLKey key;
  int left_pressed, right_pressed, up_pressed, shift_pressed;
  char str[10];
  Uint32 now_time, last_time;
  
  
  done = 0;
  quit = 0;
  counter = 0;
  
  left_pressed = 0;
  right_pressed = 0;
  up_pressed = 0;
  shift_pressed = 0;

  if (game_pending == 0)
  {  
    lives = 3;
    score = 0;
  
    player_alive = 1;
    player_die_timer = 0;
    angle = 90;
    x = (WIDTH / 2) << 4;
    y = (HEIGHT / 2) << 4;
    xm = 0;
    ym = 0;

    level = 1;
    reset_level();
  }
 
  game_pending = 1; 
  
  
  
  /* Play music: */
      
#ifndef NOSOUND
      if (use_sound)
	{
	  if (!Mix_PlayingMusic())
	    Mix_PlayMusic(game_music, -1);
	}
#endif
      

  do
    {
      last_time = SDL_GetTicks();
      counter++;
      
      
      /* Handle events: */
	#ifndef VITA  
	WPAD_ScanPads();
	if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) {
		done = 1;
		quit = 1;
	}
	else if (WPAD_ButtonsDown(0) & WPAD_BUTTON_PLUS) {
		done = 1;
	}
	else if (WPAD_ButtonsDown(0) & WPAD_BUTTON_DOWN) {
		left_pressed = 0;
		right_pressed = 1;
	}
	else if (WPAD_ButtonsDown(0) & WPAD_BUTTON_UP) {
		left_pressed = 1;
		right_pressed = 0;
	}
	else if (WPAD_ButtonsDown(0) & WPAD_BUTTON_2) {
		up_pressed = 1;
	}
	else if (WPAD_ButtonsDown(0) & WPAD_BUTTON_1) {
		add_bullet(x >> 4, y >> 4, angle, xm, ym);
	}
	
	if (WPAD_ButtonsUp(0) & WPAD_BUTTON_DOWN)
		right_pressed = 0;
	if (WPAD_ButtonsUp(0) & WPAD_BUTTON_UP)
		left_pressed = 0;
	if (WPAD_ButtonsUp(0) & WPAD_BUTTON_2)
		up_pressed = 0;
	#endif  
      
      while (SDL_PollEvent(&event) > 0)
	{
	  if (event.type == SDL_QUIT)
	    {
	      /* Quit! */
	      
	      done = 1;
	      quit = 1;
	    }
	  else if (event.type == SDL_KEYDOWN ||
		   event.type == SDL_KEYUP)
	    {
	      key = event.key.keysym.sym;
	      
	      if (event.type == SDL_KEYDOWN)
		{
		  if (key == SDLK_ESCAPE)
		  {
	            /* Return to menu! */

	            done = 1;
		  }
		  
		  
		  /* Key press... */
		 
		  if (key == SDLK_RIGHT)
		    {
		      /* Rotate CW */
		      
  		      left_pressed = 0;
		      right_pressed = 1;
		    }
		  else if (key == SDLK_LEFT)
		    {
		      /* Rotate CCW */
		      
		      left_pressed = 1;
		      right_pressed = 0;
		    }
		  else if (key == SDLK_UP)
		    {
		      /* Thrust! */
		      
		      up_pressed = 1;
		    }
		  else if ((key == SDLK_SPACE) &&
		           player_alive)
		    {
		      /* Fire a bullet! */
		     
		      add_bullet(x >> 4, y >> 4, angle, xm, ym);
		    }
		  
		  if (key == SDLK_LSHIFT ||
		      key == SDLK_RSHIFT)
		    {
		      /* Respawn now (if applicable) */

		      shift_pressed = 1;
		    }
		}
	      else if (event.type == SDL_KEYUP)
		{
		  /* Key release... */
		  
		  if (key == SDLK_RIGHT)
		    {
		      right_pressed = 0;
		    }
		  else if (key == SDLK_LEFT)
		    {
                      left_pressed = 0;
		    }
		  else if (key == SDLK_UP)
		    {
		      up_pressed = 0;
		    }

		  if (key == SDLK_LSHIFT ||
		      key == SDLK_RSHIFT)
		    {
		      /* Respawn now (if applicable) */

		      shift_pressed = 0;
		    }
		}
	    }
#ifdef JOY_YES
	  else if (event.type == SDL_JOYBUTTONDOWN &&
		   player_alive)
	    {
	      if (event.jbutton.button == JOY_B)
		{
		  /* Fire a bullet! */
		  
		  add_bullet(x >> 4, y >> 4, angle, xm, ym);
		}
	      else if (event.jbutton.button == JOY_A)
		{
		  /* Thrust: */
		  
		  up_pressed = 1;
		}
		#ifdef VITA
		else if (event.jbutton.button == VITA_BTN_START) done = 1;
		#endif
	      else
		{
		  shift_pressed = 1;
		} 
		
	    }
	  else if (event.type == SDL_JOYBUTTONUP)
	    {
	      if (event.jbutton.button == JOY_A)
		{
		  /* Stop thrust: */
		  
		  up_pressed = 0;
		}
	      else if (event.jbutton.button != JOY_B)
		{
		  shift_pressed = 0;
		}
	    }
	  else if (event.type == SDL_JOYAXISMOTION)
	    {
	      if (event.jaxis.axis == JOY_X)
		{
		  if (event.jaxis.value < -256)
		    {
		      left_pressed = 1;
		      right_pressed = 0;
		    }
		  else if (event.jaxis.value > 256)
		    {
		      left_pressed = 0;
		      right_pressed = 1;
		    }
		  else
		    {
		      left_pressed = 0;
		      right_pressed = 0;
		    }
		}
	    }
#endif
	}

      
      /* Rotate ship: */
      
      if (right_pressed)
	{
	  angle = angle - 8;
	  if (angle < 0)
	    angle = angle + 360;
	}
      else if (left_pressed)
	{
	  angle = angle + 8;
	  if (angle >= 360)
	    angle = angle - 360;
	}


      /* Thrust ship: */
      
      if (up_pressed && player_alive)
	{
	  /* Move forward: */
	  
	  xm = xm + ((fast_cos(angle >> 3) * 3) >> 10);
	  ym = ym - ((fast_sin(angle >> 3) * 3) >> 10);
	  
	  
	  /* Start thruster sound: */
#ifndef NOSOUND
	  if (use_sound)
	    {
	      if (!Mix_Playing(CHAN_THRUST))
		{
#ifndef EMBEDDED
		  Mix_PlayChannel(CHAN_THRUST, sounds[SND_THRUST], -1);
#else
		  Mix_PlayChannel(-1, sounds[SND_THRUST], 0);
#endif
		}
	    }
#endif
	}
      else
        {
	  /* Slow down (unrealistic, but.. feh!) */
	  
          if ((counter % 20) == 0)
	  {
            xm = (xm * 7) / 8;
	    ym = (ym * 7) / 8;
	  }

	  
	  /* Stop thruster sound: */

#ifndef NOSOUND
	  if (use_sound)
	    {
	      if (Mix_Playing(CHAN_THRUST))
		{
#ifndef EMBEDDED
		  Mix_HaltChannel(CHAN_THRUST);
#endif
		}
	    }
#endif
	}
      
      
      /* Handle player death: */
      
      if (player_alive == 0)
	{
	  player_die_timer--;
	  
	  if (player_die_timer <= 0)
	    {
	      if (lives > 0)
	      {
	        /* Reset player: */
	      
  	        player_die_timer = 0;
	        angle = 90;
	        x = (WIDTH / 2) << 4;
	        y = (HEIGHT / 2) << 4;
	        xm = 0;
	        ym = 0;
	      
	      
	        /* Only bring player back when it's alright to! */
	      
	        player_alive = 1;
	     
	        if (!shift_pressed)
		{	
	          for (i = 0; i < NUM_ASTEROIDS && player_alive; i++)
	    	    {
	 	      if (asteroids[i].alive)
		        {
		          if (asteroids[i].x >= (x >> 4) - (WIDTH / 5) &&
			      asteroids[i].x <= (x >> 4) + (WIDTH / 5) &&
			      asteroids[i].y >= (y >> 4) - (HEIGHT / 5) &&
	 		      asteroids[i].y <= (y >> 4) + (HEIGHT / 5))
			    {
			      /* If any asteroid is too close for comfort,
			         don't bring ship back yet! */
			  
			      player_alive = 0;
			    }
		        }
	 	    }
		}
	      }
	      else
	      {
	        done = 1;
		game_pending = 0;
	      }
	    }
	}
      
      
      /* Erase screen: */

      SDL_BlitSurface(bkgd, NULL, screen, NULL);


      /* Move ship: */
      
      x = x + xm;
      y = y + ym;
      
      
      /* Wrap ship around edges of screen: */
      
      if (x >= (WIDTH << 4))
	x = x - (WIDTH << 4);
      else if (x < 0)
	x = x + (WIDTH << 4);
      
      if (y >= (HEIGHT << 4))
	y = y - (HEIGHT << 4);
      else if (y < 0)
	    y = y + (HEIGHT << 4);
      
      
      /* Move bullets: */
      
      for (i = 0; i < NUM_BULLETS; i++)
	{
	  if (bullets[i].timer >= 0)
	    {
	      /* Bullet wears out: */
	      
	      bullets[i].timer--;
	      
	      
	      /* Move bullet: */
	      
	      bullets[i].x = bullets[i].x + bullets[i].xm;
	      bullets[i].y = bullets[i].y + bullets[i].ym;
	      
	      
	      /* Wrap bullet around edges of screen: */
	      
	      if (bullets[i].x >= WIDTH)
		bullets[i].x = bullets[i].x - WIDTH;
	      else if (bullets[i].x < 0)
		bullets[i].x = bullets[i].x + WIDTH;
	      
	      if (bullets[i].y >= HEIGHT)
		bullets[i].y = bullets[i].y - HEIGHT;
	      else if (bullets[i].y < 0)
		bullets[i].y = bullets[i].y + HEIGHT;
	      
	      
	      /* Check for collision with any asteroids! */
	      
	      for (j = 0; j < NUM_ASTEROIDS; j++)
		{
		  if (bullets[i].timer > 0 && asteroids[j].alive)
		    {
		      if ((bullets[i].x + 5 >=
			   asteroids[j].x - asteroids[j].size * AST_RADIUS) &&
			  (bullets[i].x - 5<=
			   asteroids[j].x + asteroids[j].size * AST_RADIUS) &&
			  (bullets[i].y + 5 >=
			   asteroids[j].y - asteroids[j].size * AST_RADIUS) &&
			  (bullets[i].y - 5 <=
			   asteroids[j].y + asteroids[j].size * AST_RADIUS))
			{
			  /* Remove bullet! */
			  
			  bullets[i].timer = 0;
			  
			  
			  hurt_asteroid(j, bullets[i].xm, bullets[i].ym,
					asteroids[j].size * 3);
			}
		    }
		}
	    }
	}
      
      
      /* Move asteroids: */
      
      num_asteroids_alive = 0;
      
      for (i = 0; i < NUM_ASTEROIDS; i++)
	{
	  if (asteroids[i].alive)
	    {
	      num_asteroids_alive++;
	      
	      /* Move asteroid: */
	      
	      if ((counter % 4) == 0)
		{
		  asteroids[i].x = asteroids[i].x + asteroids[i].xm;
		  asteroids[i].y = asteroids[i].y + asteroids[i].ym;
		}
	      
	      
	      /* Wrap asteroid around edges of screen: */
	      
	      if (asteroids[i].x >= WIDTH)
		asteroids[i].x = asteroids[i].x - WIDTH;
	      else if (asteroids[i].x < 0)
		asteroids[i].x = asteroids[i].x + WIDTH;
	      
	      if (asteroids[i].y >= HEIGHT)
		asteroids[i].y = asteroids[i].y - HEIGHT;
	      else if (asteroids[i].y < 0)
		asteroids[i].y = asteroids[i].y + HEIGHT;
	      
	      
	      /* Rotate asteroid: */
	      
	      asteroids[i].angle = (asteroids[i].angle +
				    asteroids[i].angle_m);
	      
	      
	      /* Wrap rotation angle... */
	      
	      if (asteroids[i].angle < 0)
		asteroids[i].angle = asteroids[i].angle + 360;
	      else if (asteroids[i].angle >= 360)
		asteroids[i].angle = asteroids[i].angle - 360;
	      
	      
	      /* See if we collided with the player: */
	      
	      if (asteroids[i].x >= (x >> 4) - SHIP_RADIUS &&
		  asteroids[i].x <= (x >> 4) + SHIP_RADIUS &&
		  asteroids[i].y >= (y >> 4) - SHIP_RADIUS &&
		  asteroids[i].y <= (y >> 4) + SHIP_RADIUS &&
		  player_alive)
		{
		  hurt_asteroid(i, xm >> 4, ym >> 4, NUM_BITS);
		  
		  player_alive = 0;
		  player_die_timer = 30;
		  
		  playsound(SND_EXPLODE);

		  /* Stop thruster sound: */
		  
#ifndef NOSOUND
		  if (use_sound)
		    {
		      if (Mix_Playing(CHAN_THRUST))
			{
#ifndef EMBEDDED
			  Mix_HaltChannel(CHAN_THRUST);
#endif
			}
		    }
#endif
		  
		  lives--;

		  if (lives == 0)
		  {
#ifndef NOSOUND
		    if (use_sound)
		      {
			playsound(SND_GAMEOVER);
			playsound(SND_GAMEOVER);
			playsound(SND_GAMEOVER);
			/* Mix_PlayChannel(CHAN_THRUST,
			   sounds[SND_GAMEOVER], 0); */
		      }
#endif
	            player_die_timer = 100;
		  }
		}
	    }
	}
      
      
      /* Move bits: */
      
      for (i = 0; i < NUM_BITS; i++)
	{
	  if (bits[i].timer > 0)
	    {
	      /* Countdown bit's lifespan: */
	      
	      bits[i].timer--;
	      
	      
	      /* Move the bit: */
	      
	      bits[i].x = bits[i].x + bits[i].xm;
	      bits[i].y = bits[i].y + bits[i].ym;


	      /* Wrap bit around edges of screen: */
	      
	      if (bits[i].x >= WIDTH)
		bits[i].x = bits[i].x - WIDTH;
	      else if (bits[i].x < 0)
		bits[i].x = bits[i].x + WIDTH;
	      
	      if (bits[i].y >= HEIGHT)
		bits[i].y = bits[i].y - HEIGHT;
	      else if (bits[i].y < 0)
		bits[i].y = bits[i].y + HEIGHT;
	    }
	}


      /* Draw ship: */
      
      if (player_alive)
	{
	  draw_segment(SHIP_RADIUS, 0, mkcolor(128, 128, 255),
		       SHIP_RADIUS / 2, 135, mkcolor(0, 0, 192),
		       x >> 4, y >> 4,
		       angle);
	  
	  draw_segment(SHIP_RADIUS / 2, 135, mkcolor(0, 0, 192),
		       0, 0, mkcolor(64, 64, 230),
		       x >> 4, y >> 4,
		       angle);
	  
	  draw_segment(0, 0, mkcolor(64, 64, 230),
		       SHIP_RADIUS / 2, 225, mkcolor(0, 0, 192),
		       x >> 4, y >> 4,
		       angle);
	  
	  draw_segment(SHIP_RADIUS / 2, 225, mkcolor(0, 0, 192),
		       SHIP_RADIUS, 0, mkcolor(128, 128, 255),
		       x >> 4, y >> 4,
		       angle);
	  
	  
	  /* Draw flame: */
	  
	  if (up_pressed)
	    {
#ifndef EMBEDDED
	      draw_segment(0, 0, mkcolor(255, 255, 255),
			   (rand() % 20), 180, mkcolor(255, 0, 0),
			   x >> 4, y >> 4,
			   angle);
#else
	      i = (rand() % 128) + 128;

	      draw_segment(0, 0, mkcolor(255, i, i),
			   (rand() % 20), 180, mkcolor(255, i, i),
			   x >> 4, y >> 4,
			   angle);
#endif
	    }
	}
      
      
      /* Draw bullets: */
      
      for (i = 0; i < NUM_BULLETS; i++)
	{
	  if (bullets[i].timer >= 0)
	    {
	      draw_line(bullets[i].x - (rand() % 3) - bullets[i].xm * 2,
			bullets[i].y - (rand() % 3) - bullets[i].ym * 2,
			mkcolor((rand() % 3) * 128,
				(rand() % 3) * 128,
				(rand() % 3) * 128),
			bullets[i].x + (rand() % 3) - bullets[i].xm * 2,
			bullets[i].y + (rand() % 3) - bullets[i].ym * 2,
			mkcolor((rand() % 3) * 128,
				(rand() % 3) * 128,
				(rand() % 3) * 128));
	      
	      draw_line(bullets[i].x + (rand() % 3) - bullets[i].xm * 2,
			bullets[i].y - (rand() % 3) - bullets[i].ym * 2,
			mkcolor((rand() % 3) * 128,
				(rand() % 3) * 128,
				(rand() % 3) * 128),
			bullets[i].x - (rand() % 3) - bullets[i].xm * 2,
			bullets[i].y + (rand() % 3) - bullets[i].ym * 2,
			mkcolor((rand() % 3) * 128,
				(rand() % 3) * 128,
				(rand() % 3) * 128));
	      
	      
	      
	      draw_thick_line(bullets[i].x - (rand() % 5),
			      bullets[i].y - (rand() % 5),
			      mkcolor((rand() % 3) * 128 + 64,
				      (rand() % 3) * 128 + 64,
				      (rand() % 3) * 128 + 64),
			      bullets[i].x + (rand() % 5),
			      bullets[i].y + (rand() % 5),
			      mkcolor((rand() % 3) * 128 + 64,
				      (rand() % 3) * 128 + 64,
				      (rand() % 3) * 128 + 64));
	      
	      draw_thick_line(bullets[i].x + (rand() % 5),
			      bullets[i].y - (rand() % 5),
			      mkcolor((rand() % 3) * 128 + 64,
				      (rand() % 3) * 128 + 64,
				      (rand() % 3) * 128 + 64),
			      bullets[i].x - (rand() % 5),
			      bullets[i].y + (rand() % 5),
			      mkcolor((rand() % 3) * 128 + 64,
				      (rand() % 3) * 128 + 64,
				      (rand() % 3) * 128 + 64));
	    }
	}
      
      
      /* Draw asteroids: */
      
      for (i = 0; i < NUM_ASTEROIDS; i++)
	{
	  if (asteroids[i].alive)
	    {
	      draw_asteroid(asteroids[i].size,
			    asteroids[i].x, asteroids[i].y,
			    asteroids[i].angle,
			    asteroids[i].shape);
	    }
	}


      /* Draw bits: */
      
      for (i = 0; i < NUM_BITS; i++)
	{
	  if (bits[i].timer > 0)
	    {
	      draw_line(bits[i].x, bits[i].y, mkcolor(255, 255, 255),
	                bits[i].x + bits[i].xm,
		       	bits[i].y + bits[i].ym, mkcolor(255, 255, 255));
	    }
	}

      
      /* Draw score: */
     
#ifndef EMBEDDED
      sprintf(str, "%.6d", score);
      draw_text(str, 3, 3, 14, mkcolor(255, 255, 255));
      draw_text(str, 4, 4, 14, mkcolor(255, 255, 255));
#else
      sprintf(str, "%.6d", score);
      draw_text(str, 3, 3, 10, mkcolor(255, 255, 255));
      draw_text(str, 4, 4, 10, mkcolor(255, 255, 255));
#endif


      /* Level: */
      
#ifndef EMBEDDED
      sprintf(str, "%d", level);
      draw_text(str, (WIDTH - 14) / 2, 3, 14, mkcolor(255, 255, 255));
      draw_text(str, (WIDTH - 14) / 2 + 1, 4, 14, mkcolor(255, 255, 255));
#else
      sprintf(str, "%d", level);
      draw_text(str, (WIDTH - 14) / 2, 3, 10, mkcolor(255, 255, 255));
      draw_text(str, (WIDTH - 14) / 2 + 1, 4, 10, mkcolor(255, 255, 255));
#endif

      
      /* Draw lives: */
      
      for (i = 0; i < lives; i++)
	{
	  draw_segment(16, 0, mkcolor(255, 255, 255),
		       4, 135, mkcolor(255, 255, 255),
		       WIDTH - 10 - i * 10, 20,
		       90);
	  
	  draw_segment(8, 135, mkcolor(255, 255, 255),
		       0, 0, mkcolor(255, 255, 255),
		       WIDTH - 10 - i * 10, 20,
		       90);
	  
	  draw_segment(0, 0, mkcolor(255, 255, 255),
		       8, 225, mkcolor(255, 255, 255),
		       WIDTH - 10 - i * 10, 20,
		       90);
	  
	  draw_segment(8, 225, mkcolor(255, 255, 255),
		       16, 0, mkcolor(255, 255, 255),
		       WIDTH - 10 - i * 10, 20,
		       90);
	}
      
      
      if (player_die_timer > 0)
	{
	  if (player_die_timer > 30)
	    j = 30;
	  else
	    j = player_die_timer;
	  
	  draw_segment((16 * j) / 30, 0, mkcolor(255, 255, 255),
		       (4 * j) / 30, 135, mkcolor(255, 255, 255),
		       WIDTH - 10 - i * 10, 20,
		       90);
	  
	  draw_segment((8 * j) / 30, 135, mkcolor(255, 255, 255),
		       0, 0, mkcolor(255, 255, 255),
		       WIDTH - 10 - i * 10, 20,
		       90);
	  
	  draw_segment(0, 0, mkcolor(255, 255, 255),
		       (8 * j) / 30, 225, mkcolor(255, 255, 255),
		       WIDTH - 10 - i * 10, 20,
		       90);
	  
	  draw_segment((8 * j) / 30, 225, mkcolor(255, 255, 255),
		       (16 * j) / 30, 0, mkcolor(255, 255, 255),
		       WIDTH - 10 - i * 10, 20,
		       90);

	}


      /* Zooming level effect: */

      if (text_zoom > 0)
	{
	  if ((counter % 2) == 0)
	    text_zoom--;

#ifndef EMBEDDED 
	  draw_text(zoom_str, (WIDTH - (strlen(zoom_str) * text_zoom)) / 2,
		    (HEIGHT - text_zoom) / 2,
		    text_zoom, mkcolor(text_zoom * (256 / ZOOM_START), 0, 0));
#else
	  draw_text(zoom_str, (WIDTH - (strlen(zoom_str) * text_zoom)) / 2,
		    (HEIGHT - text_zoom) / 2,
		    text_zoom, mkcolor(text_zoom * (256 / ZOOM_START), 128, 128));
#endif
	}


      /* Game over? */

      if (player_alive == 0 && lives == 0)
      {
	if (player_die_timer > 14)
	{
	  draw_text("GAME OVER",
	            (WIDTH - 9 * player_die_timer) / 2,
	  	    (HEIGHT - player_die_timer) / 2,
		    player_die_timer,
		    mkcolor(rand() % 255,
			    rand() % 255,
			    rand() % 255));
	}
	else
	{
	  draw_text("GAME OVER",
	            (WIDTH - 9 * 14) / 2,
                    (HEIGHT - 14) / 2,
                    14,
		    mkcolor(255, 255, 255));
	  
	}
      }

      
      /* Go to next level? */
      
      if (num_asteroids_alive == 0)
	{
	  level++;
	  
	  reset_level();
	}
      
      
      /* Flush and pause! */
      
      SDL_Flip(screen);
      
      now_time = SDL_GetTicks();
      
      if (now_time < last_time + (1000 / FPS))
	{
	  SDL_Delay(last_time + 1000 / FPS - now_time);
	}
    }
  while (!done);


  /* Record, if a high score: */

  if (score >= high)
  {
    high = score;
  }




  return(quit);
}


void finish(void)
{
  SDL_Quit();
}


void setup(int argc, char * argv[])
{
  int i;
  SDL_Surface * tmp;
  
  
  /* Options: */

  score = 0;
  use_sound = TRUE;
  fullscreen = FALSE;
  
  
  /* Check command-line options: */
  
  for (i = 1; i < argc; i++)
    {
      if (strcmp(argv[i], "--fullscreen") == 0 ||
	  strcmp(argv[i], "-f") == 0)
	{
	  fullscreen = TRUE;
	}
      else if (strcmp(argv[i], "--nosound") == 0 ||
	       strcmp(argv[i], "-q") == 0)
	{
	  use_sound = FALSE;
	}
      else if (strcmp(argv[i], "--help") == 0 ||
	       strcmp(argv[i], "-h") == 0)
	{
	  show_version();
	  
	  printf("\n"
		 "Programming: Bill Kendrick, New Breed Software - bill@newbreedsoftware.com\n"
		 "Music:       Mike Faltiss (Hadji/Digital Music Kings) - deadchannel@hotmail.com\n"
		 "\n"
		 "Keyboard controls:\n"
		 "  Left/Right - Rotate ship\n"
		 "  Up         - Thrust engines\n"
		 "  Space      - Fire weapons\n"
		 "  Shift      - Respawn after death (or wait)\n"
		 "  Escape     - Return to title screen\n"
		 "\n"
		 "Joystick controls:\n"
		 "  Left/Right - Rotate ship\n"
		 "  Fire-A     - Thrust engines\n"
		 "  Fire-B     - Fire weapons\n"
		 "\n"
		 "Run with \"--usage\" for command-line options...\n"
		 "Run with \"--copying\" for copying information...\n"
		 "\n");
	  
	  exit(0);
	}
      else if (strcmp(argv[i], "--version") == 0 ||
	       strcmp(argv[i], "-v") == 0)
	{
	  show_version();
	  printf("State format file version " STATE_FORMAT_VERSION "\n");
	  exit(0);
	}
      else if (strcmp(argv[i], "--copying") == 0 ||
	       strcmp(argv[i], "-c") == 0)
	{
	  show_version();
	  printf("\n"
		 "This program is free software; you can redistribute it\n"
		 "and/or modify it under the terms of the GNU General Public\n"
		 "License as published by the Free Software Foundation;\n"
		 "either version 2 of the License, or (at your option) any\n"
		 "later version.\n"
		 "\n"
		 "This program is distributed in the hope that it will be\n"
		 "useful and entertaining, but WITHOUT ANY WARRANTY; without\n"
		 "even the implied warranty of MERCHANTABILITY or FITNESS\n"
		 "FOR A PARTICULAR PURPOSE.  See the GNU General Public\n"
		 "License for more details.\n"
		 "\n");
	  printf("You should have received a copy of the GNU General Public\n"
		 "License along with this program; if not, write to the Free\n"
		 "Software Foundation, Inc., 59 Temple Place, Suite 330,\n"
		 "Boston, MA  02111-1307  USA\n"
		 "\n");
	  exit(0);
	}
      else if (strcmp(argv[i], "--usage") == 0 ||
	       strcmp(argv[i], "-u") == 0)
	{
	  show_usage(stdout, argv[0]);
	  exit(0);
	}
      else
	{
	  show_usage(stderr, argv[0]);
	  exit(1);
	}
    }
  
  
  /* Seed random number generator: */

  srand(SDL_GetTicks());
  
  
  /* Init SDL video: */
  
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
      fprintf(stderr,
              "\nError: I could not initialize video!\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n", SDL_GetError());
      exit(1);
    }
  
  SDL_ShowCursor(0);
  /* Init joysticks: */

#ifdef JOY_YES
  use_joystick = 1;
  
  if (SDL_Init(SDL_INIT_JOYSTICK) < 0)
    {
      fprintf(stderr,
              "\nWarning: I could not initialize joystick.\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n", SDL_GetError());
      
      use_joystick = 0;
    }
  else
    {
      /* Look for joysticks: */
      
      if (SDL_NumJoysticks() <= 0)
        {
          fprintf(stderr,
                  "\nWarning: No joysticks available.\n");
          
          use_joystick = 0;
        }
      else
	{
          /* Open joystick: */
          
          js = SDL_JoystickOpen(0);
          
          if (js == NULL)
            {
              fprintf(stderr,
                      "\nWarning: Could not open joystick 1.\n"
                      "The Simple DirectMedia error that occured was:\n"
                      "%s\n\n", SDL_GetError());
              
              use_joystick = 0;
            }
	  else
	    {
              /* Check for proper stick configuration: */
              
              if (SDL_JoystickNumAxes(js) < 2)
                {
                  fprintf(stderr,
                          "\nWarning: Joystick doesn't have enough axes!\n");
                  
                  use_joystick = 0;
                }
              else
                {
                  if (SDL_JoystickNumButtons(js) < 2)
                    {
                      fprintf(stderr,
                              "\nWarning: Joystick doesn't have enough "
                              "buttons!\n");
                      
                      use_joystick = 0;
                    }
                }
	    }
	}
    }
#else
  use_joystick = 0;
#endif
  
  
  /* Open window: */

  if (fullscreen)
    {
      screen = set_vid_mode(SDL_FULLSCREEN | SDL_HWSURFACE);
      
      if (screen == NULL)
        {
          fprintf(stderr,
                  "\nWarning: I could not set up fullscreen video for "
                  "%dx%d mode.\n"
                  "The Simple DirectMedia error that occured was:\n"
                  "%s\n\n", WIDTH, HEIGHT, SDL_GetError());
          fullscreen = 0;
        }
    }
  
  if (!fullscreen)
    {
      screen = set_vid_mode(0);
      
      if (screen == NULL)
	{
	  fprintf(stderr,
		  "\nError: I could not open the display.\n"
		  "The Simple DirectMedia error that occured was:\n"
		  "%s\n\n", SDL_GetError());
	  exit(1);
	}
    }
  
  	#ifdef VITA
	SDL_SetVideoModeScaling(120, 0, 720, 540);
	#endif
  /* Load background image: */

#ifndef EMBEDDED
  tmp = IMG_Load(DATA_PREFIX "images/redspot.jpg");

  if (tmp == NULL)
    {
      fprintf(stderr,
	      "\nError: I could not open the background image:\n"
	      DATA_PREFIX "images/redspot.jpg\n"
	      "The Simple DirectMedia error that occured was:\n"
	      "%s\n\n", SDL_GetError());
      exit(1);
    }
  
  bkgd = SDL_DisplayFormat(tmp);
  if (bkgd == NULL)
    {
      fprintf(stderr,
	      "\nError: I couldn't convert the background image"
	      "to the display format!\n"
	      "The Simple DirectMedia error that occured was:\n"
	      "%s\n\n", SDL_GetError());
      exit(1);
    }
  
  SDL_FreeSurface(tmp);

#else
  
  tmp = SDL_LoadBMP(DATA_PREFIX "images/redspot-e.bmp");

  if (tmp == NULL)
    {
      fprintf(stderr,
	      "\nError: I could not open the background image:\n"
	      DATA_PREFIX "images/redspot-e.bmp\n"
	      "The Simple DirectMedia error that occured was:\n"
	      "%s\n\n", SDL_GetError());
      exit(1);
    }
  
  bkgd = SDL_DisplayFormat(tmp);
  if (bkgd == NULL)
    {
      fprintf(stderr,
	      "\nError: I couldn't convert the background image"
	      "to the display format!\n"
	      "The Simple DirectMedia error that occured was:\n"
	      "%s\n\n", SDL_GetError());
      exit(1);
    }
  
  SDL_FreeSurface(tmp);
#endif


#ifndef NOSOUND
  /* Init sound: */
  
  if (use_sound)
    {
      if (Mix_OpenAudio(22050, AUDIO_S16, 2, 512) < 0)
	{
	  fprintf(stderr,
                  "\nWarning: I could not set up audio for 22050 Hz "
                  "16-bit stereo.\n"
                  "The Simple DirectMedia error that occured was:\n"
                  "%s\n\n", SDL_GetError());
          use_sound = FALSE;
	}
    }
  
  
  /* Load sound files: */
  
  if (use_sound)
    {
      for (i = 0; i < NUM_SOUNDS; i++)
	{
	  sounds[i] = Mix_LoadWAV(sound_names[i]);
          if (sounds[i] == NULL)
            {
              fprintf(stderr,
                      "\nError: I could not load the sound file:\n"
                      "%s\n"
                      "The Simple DirectMedia error that occured was:\n"
                      "%s\n\n", sound_names[i], SDL_GetError());
              exit(1);
            }
	}
      
      
      game_music = Mix_LoadMUS(mus_game_name);
      if (game_music == NULL)
	{
	  fprintf(stderr,
		  "\nError: I could not load the music file:\n"
		  "%s\n"
		  "The Simple DirectMedia error that occured was:\n"
		  "%s\n\n", mus_game_name, SDL_GetError());
	  exit(1);
	}
    }
#endif
  
  
  //seticon();
  SDL_WM_SetCaption("Vectoroids", "Vectoroids");
}


/* Set the window's icon: */

void seticon(void)
{
#ifndef EMBEDDED
  int masklen;
  Uint8 * mask;
  SDL_Surface * icon;
  
  
  /* Load icon into a surface: */
  
  icon = IMG_Load(DATA_PREFIX "images/icon.png");
  if (icon == NULL)
    {
      fprintf(stderr,
              "\nError: I could not load the icon image: %s\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n", DATA_PREFIX "images/icon.png", SDL_GetError());
      exit(1);
    }
  
  
  /* Create mask: */
  
  masklen = (((icon -> w) + 7) / 8) * (icon -> h);
  mask = malloc(masklen * sizeof(Uint8));
  memset(mask, 0xFF, masklen);
  
  
  /* Set icon: */
  
  SDL_WM_SetIcon(icon, mask);
  
  
  /* Free icon surface & mask: */
  
  free(mask);
  SDL_FreeSurface(icon);
#endif
}


/* Fast approximate-integer, table-based cosine! Whee! */

int fast_cos(int angle)
{
  angle = (angle % 45);
  
  if (angle < 12)
    return(trig[angle]);
  else if (angle < 23)
    return(-trig[10 - (angle - 12)]);
  else if (angle < 34)
    return(-trig[angle - 22]);
  else
    return(trig[45 - angle]);
}


/* Sine based on fast cosine... */

int fast_sin(int angle)
{
  return(- fast_cos((angle + 11) % 45));
}


/* Draw a line: */

void draw_line(int x1, int y1, color_type c1,
	       int x2, int y2, color_type c2)
{
  sdl_drawline(x1, y1, c1, x2, y2, c2);
 
  if (x1 < 0 || x2 < 0)
    {
      sdl_drawline(x1 + WIDTH, y1, c1, x2 + WIDTH, y2, c2);
    }
  else if (x1 >= WIDTH || x2 >= WIDTH)
    {
      sdl_drawline(x1 - WIDTH, y1, c1, x2 - WIDTH, y2, c2);
    }
  
  if (y1 < 0 || y2 < 0)
    {
      sdl_drawline(x1, y1 + HEIGHT, c1, x2, y2 + HEIGHT, c2);
    }
  else if (y1 >= HEIGHT || y2 >= HEIGHT)
    {
      sdl_drawline(x1, y1 - HEIGHT, c1, x2, y2 - HEIGHT, c2);
    }
}


/* Create a color_type struct out of RGB values: */

color_type mkcolor(int r, int g, int b)
{
  color_type c;
  
  if (r > 255)
    r = 255;
  if (g > 255)
    g = 255;
  if (b > 255)
    b = 255;

  c.r = (Uint8) r;
  c.g = (Uint8) g;
  c.b = (Uint8) b;
  
  return c;
}


/* Draw a line on an SDL surface: */

void sdl_drawline(int x1, int y1, color_type c1,
		  int x2, int y2, color_type c2)
{
  int dx, dy;
#ifndef EMBEDDED
  float cr, cg, cb, rd, gd, bd;
#endif
  float m, b;

  
  if (clip(&x1, &y1, &x2, &y2))
    {
      dx = x2 - x1;
      dy = y2 - y1;
      
      if (dx != 0)
        {
          m = ((float) dy) / ((float) dx);
          b = y1 - m * x1;
          
          if (x2 >= x1)
            dx = 1;
          else
            dx = -1;
         
#ifndef EMBEDDED
          cr = c1.r;
          cg = c1.g;
          cb = c1.b;
          
          rd = (float) (c2.r - c1.r) / (float) (x2 - x1) * dx;
          gd = (float) (c2.g - c1.g) / (float) (x2 - x1) * dx;
          bd = (float) (c2.b - c1.b) / (float) (x2 - x1) * dx;
#endif
          
          while (x1 != x2)
            {
              y1 = m * x1 + b;
              y2 = m * (x1 + dx) + b;
              
#ifndef EMBEDDED
              drawvertline(x1, y1, mkcolor(cr, cg, cb),
                           y2, mkcolor(cr + rd, cg + gd, cb + bd));
#else
              drawvertline(x1, y1, mkcolor(c1.r, c1.g, c1.b),
                           y2, mkcolor(c1.r, c1.g, c1.b));
#endif
	      
              x1 = x1 + dx;
              

#ifndef EMBEDDED
              cr = cr + rd;
              cg = cg + gd;
              cb = cb + bd;
#endif
            }
        }
      else
        drawvertline(x1, y1, c1, y2, c2);
    }
}


/* Clip lines to window: */

int clip(int * x1, int * y1, int * x2, int * y2)
{
#ifndef EMBEDDED

  float fx1, fx2, fy1, fy2, tmp;
  float m;
  unsigned char code1, code2;
  int done, draw, swapped;
  unsigned char ctmp;
  fx1 = (float) *x1;
  fy1 = (float) *y1;
  fx2 = (float) *x2;
  fy2 = (float) *y2;

  
  done = FALSE;
  draw = FALSE;
  m = 0;
  swapped = FALSE;

  
  while (!done)
    {
      code1 = encode(fx1, fy1);
      code2 = encode(fx2, fy2);
      
      if (!(code1 | code2))
        {
          done = TRUE;
          draw = TRUE;
        }
      else if (code1 & code2)
        {
          done = TRUE;
        }
      else
        {
          if (!code1)
            {
              swapped = TRUE;
              tmp = fx1;
              fx1 = fx2;
              fx2 = tmp;
              
              tmp = fy1;
              fy1 = fy2;
              fy2 = tmp;
              
              ctmp = code1;
              code1 = code2;
              code2 = ctmp;
            }
          
          
          if (fx2 != fx1)
            m = (fy2 - fy1) / (fx2 - fx1);
          else
            m = 1;
          
          if (code1 & LEFT_EDGE)
            {
              fy1 += ((0 - (fx1)) * m);
              fx1 = 0;
            }
          else if (code1 & RIGHT_EDGE)
            {
              fy1 += (((WIDTH - 1) - (fx1)) * m);
              fx1 = (WIDTH - 1);
            }
          else if (code1 & TOP_EDGE)
            {
              if (fx2 != fx1)
                fx1 += ((0 - (fy1)) / m);
              fy1 = 0;
            }
          else if (code1 & BOTTOM_EDGE)
            {
              if (fx2 != fx1)
                fx1 += (((HEIGHT - 1) - (fy1)) / m);
              fy1 = (HEIGHT - 1);
            }
        }
    }
  
  
  if (swapped)
    {
      tmp = fx1;
      fx1 = fx2;
      fx2 = tmp;
      
      tmp = fy1;
      fy1 = fy2;
      fy2 = tmp;
    }
  
 
  *x1 = (int) fx1;
  *y1 = (int) fy1;
  *x2 = (int) fx2;
  *y2 = (int) fy2;

  return(draw);
#else

  if (*x1 < 0 || *x1 >= WIDTH ||
      *y1 < 0 || *y1 >= HEIGHT ||
      *x2 < 0 || *x2 >= WIDTH ||
      *y2 < 0 || *y2 >= HEIGHT)
    return FALSE;
  else
    return TRUE;
      

#endif
}


/* Where does this line clip? */

unsigned char encode(float x, float y)
{
  unsigned char code;
  
  code = 0x00;
  
  if (x < 0.0)
    code = code | LEFT_EDGE;
  else if (x >= (float) WIDTH)
    code = code | RIGHT_EDGE;
  
  if (y < 0.0)
    code = code | TOP_EDGE;
  else if (y >= (float) HEIGHT)
    code = code | BOTTOM_EDGE;
  
  return code;
}


/* Draw a verticle line: */

void drawvertline(int x, int y1, color_type c1,
                  int y2, color_type c2)
{
  int tmp, dy;
#ifndef EMBEDDED
  float cr, cg, cb, rd, gd, bd;
#else
  int cr, cg, cb;
#endif
  
  if (y1 > y2)
    {
      tmp = y1;
      y1 = y2;
      y2 = tmp;
      
#ifndef EMBEDDED
      tmp = c1.r;
      c1.r = c2.r;
      c2.r = tmp;
      
      tmp = c1.g;
      c1.g = c2.g;
      c2.g = tmp;
      
      tmp = c1.b;
      c1.b = c2.b;
      c2.b = tmp;
#endif
    }
  
  cr = c1.r;
  cg = c1.g;
  cb = c1.b;
  
#ifndef EMBEDDED
  if (y1 != y2)
    {
      rd = (float) (c2.r - c1.r) / (float) (y2 - y1);
      gd = (float) (c2.g - c1.g) / (float) (y2 - y1);
      bd = (float) (c2.b - c1.b) / (float) (y2 - y1);
    }
  else
    {
      rd = 0;
      gd = 0;
      bd = 0;
    }
#endif
  
  for (dy = y1; dy <= y2; dy++)
    {
      putpixel(screen, x + 1, dy + 1, SDL_MapRGB(screen->format, 0, 0, 0));
      
      putpixel(screen, x, dy, SDL_MapRGB(screen->format,
                                         (Uint8) cr,
                                         (Uint8) cg,
                                         (Uint8) cb));

#ifndef EMBEDDED
      cr = cr + rd;
      cg = cg + gd;
      cb = cb + bd;
#endif
    } 
}


/* Draw a single pixel into the surface: */

void putpixel(SDL_Surface * surface, int x, int y, Uint32 pixel)
{
  int bpp;
  Uint8 * p;
  

  /* Assuming the X/Y values are within the bounds of this surface... */
  
  if (x >= 0 && y >= 0 && x < WIDTH && y < HEIGHT)
    {
      /* Determine bytes-per-pixel for the surface in question: */
      
      bpp = surface->format->BytesPerPixel;
      
      
      /* Set a pointer to the exact location in memory of the pixel
         in question: */
      
      p = (((Uint8 *) surface->pixels) +       /* Start at beginning of RAM */
	   (y * surface->pitch) +  /* Go down Y lines */
	   (x * bpp));             /* Go in X pixels */
      
      
      /* Set the (correctly-sized) piece of data in the surface's RAM
         to the pixel value sent in: */
      
      if (bpp == 1)
        *p = pixel;
      else if (bpp == 2)
        *(Uint16 *)p = pixel;
      else if (bpp == 3)
        {
          if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            {
              p[0] = (pixel >> 16) & 0xff;
              p[1] = (pixel >> 8) & 0xff;
              p[2] = pixel & 0xff;
            }
          else
            {
              p[0] = pixel & 0xff;
              p[1] = (pixel >> 8) & 0xff;
              p[2] = (pixel >> 16) & 0xff;
            }
        }
      else if (bpp == 4)
        {
          *(Uint32 *)p = pixel;
        }
    }
}



/* Draw a line segment, rotated around a center point: */

void draw_segment(int r1, int a1,
		  color_type c1,
		  int r2, int a2,
		  color_type c2,
		  int cx, int cy, int a)
{
  draw_line(((fast_cos((a1 + a) >> 3) * r1) >> 10) + cx,
	    cy - ((fast_sin((a1 + a) >> 3) * r1) >> 10),
	    c1,
	    ((fast_cos((a2 + a) >> 3) * r2) >> 10) + cx,
	    cy - ((fast_sin((a2 + a) >> 3) * r2) >> 10),
	    c2);
}


/* Add a bullet: */

void add_bullet(int x, int y, int a, int xm, int ym)
{
  int i, found;
  
  found = -1;
  
  for (i = 0; i < NUM_BULLETS && found == -1; i++)
    {
      if (bullets[i].timer <= 0)
	found = i;
    }
  
  if (found != -1)
    {
#ifndef EMBEDDED
      bullets[found].timer = 50;
#else
      bullets[found].timer = 30;
#endif
      
      bullets[found].x = x;
      bullets[found].y = y;
      
      bullets[found].xm = ((fast_cos(a >> 3) * 5) >> 10) + (xm >> 4);
      bullets[found].ym = - ((fast_sin(a >> 3) * 5) >> 10) + (ym >> 4);

      
      playsound(SND_BULLET);
    }
}


/* Add an asteroid: */

void add_asteroid(int x, int y, int xm, int ym, int size)
{
  int i, found;
  
  
  /* Find a slot: */
  
  found = -1;
  
  for (i = 0; i < NUM_ASTEROIDS && found == -1; i++)
    {
      if (asteroids[i].alive == 0)
	found = i;
    }
  
  
  /* Hack: No asteroids should be stationary! */
  
  while (xm == 0)
    {
      xm = (rand() % 3) - 1;
    }
  
  
  if (found != -1)
    {
      asteroids[found].alive = 1;
      
      asteroids[found].x = x;
      asteroids[found].y = y;
      asteroids[found].xm = xm;
      asteroids[found].ym = ym;
      
      asteroids[found].angle = (rand() % 360);
      asteroids[found].angle_m = (rand() % 6) - 3;
      
      asteroids[found].size = size;
      
      for (i = 0; i < AST_SIDES; i++)
	{
	  asteroids[found].shape[i].radius = (rand() % 3);
	  asteroids[found].shape[i].angle = i * 60 + (rand() % 40);
	}
    }
}


/* Add a bit: */

void add_bit(int x, int y, int xm, int ym)
{
  int i, found;
  
  found = -1;
  
  for (i = 0; i < NUM_BITS && found == -1; i++)
    {
      if (bits[i].timer <= 0)
	found = i;
    }
  
  
  if (found != -1)
    {
      bits[found].timer = 16;
      
      bits[found].x = x;
      bits[found].y = y;
      bits[found].xm = xm;
      bits[found].ym = ym;
    }
}


/* Draw an asteroid: */

void draw_asteroid(int size, int x, int y, int angle, shape_type * shape)
{
  int i, b1, b2;
  int div;
  
#ifndef EMBEDDED
  div = 240;
#else
  div = 120;
#endif
  
  for (i = 0; i < AST_SIDES - 1; i++)
    {
      b1 = (((shape[i].angle + angle) % 180) * 255) / div;
      b2 = (((shape[i + 1].angle + angle) % 180) * 255) / div;
      
      draw_segment((size * (AST_RADIUS - shape[i].radius)),
		   shape[i].angle, mkcolor(b1, b1, b1),
		   (size * (AST_RADIUS - shape[i + 1].radius)),
		   shape[i + 1].angle, mkcolor(b2, b2, b2),
		   x, y,
		   angle);
    }

  b1 = (((shape[AST_SIDES - 1].angle + angle) % 180) * 255) / div;
  b2 = (((shape[0].angle + angle) % 180) * 255) / div;

  draw_segment((size * (AST_RADIUS - shape[AST_SIDES - 1].radius)),
	       shape[AST_SIDES - 1].angle, mkcolor(b1, b1, b1),
	       (size * (AST_RADIUS - shape[0].radius)),
	       shape[0].angle, mkcolor(b2, b2, b2),
	       x, y,
	       angle);
}


/* Queue a sound! */

void playsound(int snd)
{
  int which, i;
  
#ifndef NOSOUND
  if (use_sound)
    {
#ifdef EMBEDDED
      which = -1;
#else
      which = (rand() % 3) + CHAN_THRUST;
      for (i = CHAN_THRUST; i < 4; i++)
	{
	  if (!Mix_Playing(i))
	    which = i;
	}
#endif

      Mix_PlayChannel(which, sounds[snd], 0);
    }
#endif
}


/* Break an asteroid and add an explosion: */

void hurt_asteroid(int j, int xm, int ym, int exp_size)
{
  int k;
  
  add_score(100 / (asteroids[j].size + 1));

  if (asteroids[j].size > 1)
    {
      /* Break the rock into two smaller ones! */
      
      add_asteroid(asteroids[j].x,
		   asteroids[j].y,
		   ((asteroids[j].xm + xm) / 2),
		   (asteroids[j].ym + ym),
		   asteroids[j].size - 1);
      
      add_asteroid(asteroids[j].x,
		   asteroids[j].y,
		   (asteroids[j].xm + xm),
		   ((asteroids[j].ym + ym) / 2),
		   asteroids[j].size - 1);
    }

  
  /* Make the original go away: */
  
  asteroids[j].alive = 0;
  
  
  /* Add explosion: */
  
  playsound(SND_AST1 + (asteroids[j].size) - 1);
  
  for (k = 0; k < exp_size; k++)
    {
      add_bit((asteroids[j].x -
	       (asteroids[j].size * AST_RADIUS) +
	       (rand() % (AST_RADIUS * 2))),
	      (asteroids[j].y -
	       (asteroids[j].size * AST_RADIUS) +
	       (rand() % (AST_RADIUS * 2))),
	      ((rand() % (asteroids[j].size * 3)) -
	       (asteroids[j].size) +
	       ((xm + asteroids[j].xm) / 3)),
	      ((rand() % (asteroids[j].size * 3)) -
	       (asteroids[j].size) +
	       ((ym + asteroids[j].ym) / 3)));
    }
}


/* Increment score: */

void add_score(int amount)
{
  /* See if they deserve a new life: */

  if (score / ONEUP_SCORE < (score + amount) / ONEUP_SCORE)
  {
    lives++;
    strcpy(zoom_str, "EXTRA LIFE");
    text_zoom = ZOOM_START;
    playsound(SND_EXTRALIFE);
  }



  /* Add to score: */
  
  score = score + amount;
}


/* Draw a character: */

void draw_char(char c, int x, int y, int r, color_type cl)
{
  int i, v;
  
  /* Which vector is this character? */
  
  v = -1;
  if (c >= '0' && c <= '9')
    v = (c - '0');
  else if (c >= 'A' && c <= 'Z')
    v = (c - 'A') + 10;
  
  
  if (v != -1)
    {
      for (i = 0; i < 5; i++)
	{
	  if (char_vectors[v][i][0] != -1)
	    {
	      draw_line(x + (char_vectors[v][i][0] * r),
			y + (char_vectors[v][i][1] * r),
			cl,
			x + (char_vectors[v][i][2] * r),
			y + (char_vectors[v][i][3] * r),
			cl);
	    }
	}
    }
}


void draw_text(char * str, int x, int y, int s, color_type c)
{
  int i;

  for (i = 0; i < strlen(str); i++)
    draw_char(str[i], i * (s + 3) + x, y, s, c);
}


void draw_thick_line(int x1, int y1, color_type c1,
		     int x2, int y2, color_type c2)
{
  draw_line(x1, y1, c1, x2, y2, c2);
  draw_line(x1 + 1, y1 + 1, c1, x2 + 1, y2 + 1, c2);
}


void reset_level(void)
{
  int i;
  
  
  for (i = 0; i < NUM_BULLETS; i++)
    bullets[i].timer = 0;
  
  for (i = 0; i < NUM_ASTEROIDS; i++)
    asteroids[i].alive = 0;
  
  for (i = 0; i < NUM_BITS; i++)
    bits[i].timer = 0;
  
  for (i = 0; i < (level + 1) && i < 10; i++)
    {
#ifndef EMBEDDED
      add_asteroid(/* x */ (rand() % 40) + ((WIDTH - 40) * (rand() % 2)),
		   /* y */ (rand() % HEIGHT),
		   /* xm */ (rand() % 9) - 4,
		   /* ym */ ((rand() % 9) - 4) * 4,
		   /* size */ (rand() % 3) + 2);
#else
      add_asteroid(/* x */ (rand() % WIDTH),
		   /* y */ (rand() % 40) + ((HEIGHT - 40) * (rand() % 2)),
		   /* xm */ ((rand() % 9) - 4) * 4,
		   /* ym */ (rand() % 9) - 4,
		   /* size */ (rand() % 3) + 2);
#endif
    }


  sprintf(zoom_str, "LEVEL %d", level);

  text_zoom = ZOOM_START;
}


/* Show program version: */

void show_version(void)
{
  printf("Vectoroids - Version " VER_VERSION " (" VER_DATE ")\n");
}


/* Show usage display: */

void show_usage(FILE * f, char * prg)
{
  fprintf(f, "Usage: %s {--help | --usage | --version | --copying }\n"
             "       %s [--fullscreen] [--nosound]\n\n", prg, prg);
}


/* Set video mode: */
/* Contributed to "Defendguin" by Mattias Engdegard <f91-men@nada.kth.se> */

SDL_Surface * set_vid_mode(unsigned flags)
{
  /* Prefer 16bpp, but also prefer native modes to emulated 16bpp. */
  
  int depth;
  
  depth = SDL_VideoModeOK(WIDTH, HEIGHT, 16, flags);
  return depth ? SDL_SetVideoMode(WIDTH, HEIGHT, depth, flags) : NULL;
}


/* Draw text, centered horizontally: */

void draw_centered_text(char * str, int y, int s, color_type c)
{
  draw_text(str, (WIDTH - strlen(str) * (s + 3)) / 2, y, s, c);
}

