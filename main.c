/****************
	If this does not compile, please run the binary as evidence that the program works, then follow the instructions bellow
Instructions to build
	 * 	To build, add these lines to the linker in project options
 	-lmingw32 -lSDLmain -lSDL
	*	If the SDL source files are not installed, they are available here <http://www.libsdl.org/download-1.2.php>
	* 	For instructions on how to compile the project, assuming dev cpp is the IDE of choice go here 
	<http://lazyfoo.net/SDL_tutorials/lesson01/windows/devcpp/index.php>
 	
*****************/
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDL/SDL.h"
 
#define SCREEN_HEIGHT 400
#define SCREEN_WIDTH 400
#define X_OFFSET 15
#define Y_OFFSET 15

#define TRUE 1
#define FALSE 0
#define LIVE_CELL_CHAR 'x'
#define DEAD_CELL_CHAR ' '
//For the sample string arrays
#define MAX_ROWS 21
#define MAX_COLUMNS 45 
//
#define MAX_IN_ADJ_ROW 3
#define OVERCROWDING_THRESHHOLD 4
#define REPRODUCE_THRESHHOLD 3
#define CONTINUE_THRESHHOLD 2
#define MAX_WIDTH 1000

//
typedef unsigned char BOOL;
//Represents a single GOL world
typedef struct {
	int width;
	int height;
	char** cells;
}Life;
//Contains an array of lives... Also contains an oscillation period which is -1 by default Should have used a linked list
typedef struct {
	int len;
	Life* lives;
	int oscPeriod;
}Lives;

void addLife (Lives* lives, Life *life);
BOOL equalLives (Life* a, Life* b);
void evaluateOscPeriod (Lives* lives);
Life loadFromFile (const char* fileName);
Life createBlankLife (int width, int height);
Life createRandomLife (int width, int height, float percentLiving);
Life createFromString (const char plife[MAX_ROWS][MAX_COLUMNS]) ;
BOOL shouldContinue (Life* life, int y, int x);
BOOL shouldComeAlive (Life* life, int y, int x);
int getNeighbours (Life* life, int y, int x) ;
Life createNewLife (Life* life);
int circleLife (Life* life, Lives* lives, SDL_Surface* surface);
int fadeRGB (int color, int fadeRed, int fadeGrn, int fadeBlu);
BOOL isLegalStr (char* string);
//Some strings that represent different worlds
const char STABLE_WORLD_[MAX_ROWS][MAX_COLUMNS] = {"!------------------------------------------!",
	"!                                          !",
	"!       xx xx                              !",   
	"!        x x                               !",
	"!        x x            xx                 !",
	"!       xx xx           xx                 !",
	"!                                          !",
	"!                                          !",
	"!                                          !",
	"!       xxxx             x                 !",
	"!                        xx                !",
	"!                         x                !",
	"!                                          !",
	"!                                          !",
	"!                                          !",
	"!                                          !",
	"!            x                             !",
	"!            xxx                           !",
	"!                                          !",
	"!                                          !",
	"!------------------------------------------!"};
const char OSCILLATING_WORLD_[MAX_ROWS][MAX_COLUMNS] = {"!------------------------------------------!",
	"!                                          !",
	"!                                          !",   
	"!                                          !",
	"!                                          !",
	"!       xx     xx                          !",
	"!        xx   xx               xxx         !",
	"!     x  x x x x  x                        !",
	"!     xxx xx xx xxx                        !",
	"!      x x x x x x                         !",
	"!       xxx   xxx                          !",
	"!                                          !",
	"!       xxx   xxx                          !",
	"!      x x x x x x               x         !",
	"!     xxx xx xx xxx              x         !",
	"!     x  x x x x  x              x         !",
	"!        xx   xx                           !",
	"!       xx     xx                          !",                                 
	"!                                          !",
	"!                                          !",
	"!------------------------------------------!"};
const  char SPACESHIP_WORLD_[MAX_ROWS][MAX_COLUMNS] = {"!------------------------------------------!",
	"!                                          !",
	"!     x                                    !",   
	"!      x                                   !",
	"!    xxx                                   !",
	"!                                          !",
	"!                                          !",
	"!                                          !",
	"!                                          !",
	"!                                          !",
	"!                                          !",
	"!                                          !",
	"!                                          !",
	"!                                          !",
	"!                                          !",
	"!                                          !",
	"!           x                              !",
	"!          x                               !",
	"!          xxx                             !",
	"!                                          !",
	"!------------------------------------------!"};

//Creates a blank game of life world with specified dimensions	
Life createBlankLife (int width, int height) {
	Life ret;
	int i;
	char** cells = (char**)malloc(sizeof(char*)* height);
	for (i = 0 ; i < height; i++) 
			cells[i] = (char*)calloc(sizeof(char), width);
	ret.width = width;
	ret.height = height;
	ret.cells = cells;
	return ret;
}

//Creates a random world
Life createRandomLife (int width, int height, float percentLiving) {
	int setCells = width * height * percentLiving;
	int i_x;
	int i_y;
	Life ret = createBlankLife (width, height);
	while (setCells > 0) {
		i_x = rand() % width;
		i_y = rand() % height;
		if (ret.cells[i_y][i_x] == FALSE) {
			ret.cells[i_y][i_x] = TRUE;
			setCells--;
		} 
	}
	return ret;
}

//Creates a world from the strings specified
Life createFromString (const char plife[MAX_ROWS][MAX_COLUMNS]) {
	int i;
	int j;
	Life ret = createBlankLife(MAX_COLUMNS - 2,MAX_ROWS - 2);
	for (i = 0; i < ret.height; i++) 
		for (j = 0; j < ret.width; j++)
			if (plife[i + 1][j + 1] == LIVE_CELL_CHAR) 
				ret.cells[i][j] = TRUE;
			else
				ret.cells[i][j] = FALSE;
		
	return ret;	
}

//Loads game of life from a txt file. Assumes that the first and last lines will be "-----"
//And each inner string will be bounded by a "!"
Life loadFromFile (const char* fileName) {
	Life ret;
	FILE* file;
	int i;
	int j;
	int len = 0;
	int str_len = -1; //Assume that every file in the text file will be of equal length
	char buffer[MAX_WIDTH];
	char** strings = (char**)malloc(sizeof(char*));
	//Time to leave if we cannot open the file
	if (file = fopen(fileName,"r"))
	{
		while (fgets(buffer,MAX_WIDTH,file) != NULL) {
			//To ignore any string that doesnt seem legal. Made specifically for those pesky top numbers
			if (isLegalStr(buffer)) {
				len++;
				strings = (char**)realloc(strings,sizeof(char*) * len);
				if (str_len == -1)
					str_len = strlen(buffer);
				strings[len - 1] = (char*)calloc(sizeof(char),str_len + 1);
				strings[len - 1] = memcpy(strings[len - 1],buffer,sizeof(char) * (str_len + 1));
			}
		}
		//Subtract 3 from the string length. 1 for first "!", one for second "!" and one for the  "\n"
		ret = createBlankLife(str_len - 3,len - 2);
		for(i = 1 ; i < len - 1; i++)
			for (j = 1 ; j < str_len - 2; j++)
				ret.cells[i - 1][j - 1] = (strings[i][j] == LIVE_CELL_CHAR) ? TRUE : FALSE;
		fclose (file);
	}
	return ret;
}

//Checks if a string contains "-", "x", "!". If it contains one of those chars it is assumed that this is a legal gol str
BOOL isLegalStr (char* string) {
	int str_len = strlen(string);
	int i;
	for (i = 0 ; i < str_len; i++) {
		if (string[i] == '-' || string[i] == 'x' || string[i] == '!')
			return TRUE;
	}
	return FALSE;
}

//TRUE if a live cell should live
BOOL shouldContinue (Life* life, int y, int x) {
	int n = getNeighbours(life,y,x);
	//printf("N %d\n",n);
	return n == REPRODUCE_THRESHHOLD || n == CONTINUE_THRESHHOLD;
}

//TRUE if a dead cell should come alive
BOOL shouldComeAlive (Life* life, int y, int x) {
	return getNeighbours(life,y,x) == REPRODUCE_THRESHHOLD;
}

//Returns number of live cells bordering cell with specified co-ordinates
int getNeighbours (Life* life, int y, int x) {
	int x_cnt = 0;
	int y_cnt = 0;
	int i;
	int j;
	int x_ind;
	int y_ind;
	int ret = 0;
	while (y_cnt < MAX_IN_ADJ_ROW) {
		if (y + (y_cnt - 1) < 0)
			y_ind = life->height - 1;
		else if (y + (y_cnt - 1) >= life->height)
			y_ind = 0;
		else
			y_ind = y + (y_cnt - 1);
		while (x_cnt < MAX_IN_ADJ_ROW) {
			if (x + (x_cnt - 1) < 0)
				x_ind = life->width - 1;
			else if (x + (x_cnt - 1) >= life->width)
				x_ind = 0;
			else
				x_ind = x + (x_cnt - 1);
			if (life->cells[y_ind][x_ind] && !(x_ind == x && y_ind == y))
				ret++;
			x_cnt++;
		}
		x_cnt = 0;
		y_cnt++;
	}
	return ret;
}

//Sets the oscillation period of a live if an oscilation period is present. If there is none the osc period will be -1
void evaluateOscPeriod (Lives* lives) {
	int i;
	if (lives->len < 2) return -1;
	for (i = lives->len - 2; i >= 0; i--) {
		if (equalLives (&lives->lives[i], &lives->lives[lives->len - 1])) {
			lives->oscPeriod = (lives->len - 1) - i;
			return;
		}
	}
	lives->oscPeriod = -1;
}

//checks if two game of life iterations are equal
BOOL equalLives (Life* a, Life* b) {
	int i;
	int j;
	if (!((a->width == b->width) && (a->height == b->height)))
		return FALSE;
	for (i = 0 ; i < a->height; i++)
		for (j = 0; j < a->width; j++) 
			if (a->cells[i][j] != b->cells[i][j])
				return FALSE;
	return TRUE;
}

//Creates a new life from a previous life. Essentially one iteration of conways game of life
Life createNewLife (Life* life) {
	Life ret = createBlankLife(life->width, life->height);
	int i;
	int j;
	for (i = 0 ; i < life->height; i++) {
		for (j = 0; j < life->width; j++) {
			ret.cells[i][j] = 
					life->cells[i][j] ? shouldContinue (life,i,j) 
										: shouldComeAlive(life,i,j);
		}
	}
	return ret;
}

//Adds a life struct to a Lives struct (list of lifes);
void addLife (Lives* lives, Life *life) {
	lives->lives = (Life*)realloc(lives->lives, sizeof(Life) * (lives->len + 1));
	lives->lives[lives->len] = *life;
	lives->len++;
}

//Fades a uint32 color by a specified rate in RGB values
int fadeRGB (int color, int fadeBlu, int fadeGrn, int fadeRed) {
	int red = color & 0xFF;
	int green = color & 0xFF00;
	int blue = color &  0xFF0000;
	red = (red >= fadeRed * 0x1) ? red - 0x1 * fadeRed : 0;
	green = (green >= fadeGrn * 0x100) ? green - 0x100 * fadeGrn : 0;
	blue = (blue >= fadeBlu * 0x10000) ? blue - 0x10000 * fadeBlu : 0;
	return red | green | blue | 0xFF000000;
}

//Renders a life to an SDL_surface
void printGOL (Life* life, SDL_Surface* surface) {
	int i;
	int j;
	SDL_Rect rect;
	int color = SDL_MapRGB(surface->format,255,255,255);
	int intr_fade = life->height * life->width / 255 * 8;
	rect.w = SCREEN_WIDTH / life->width;
	rect.h = SCREEN_HEIGHT / life->height;
	
	for (i = 0; i < life->height; i++) {
		for (j = 0 ; j < life->width; j++ ) {
			if ((i * j) % intr_fade == 0)
				color = fadeRGB (color,1,3,3);
			if (life->cells[i][j]) {
				rect.x = j * rect.w;
				rect.y = i * rect.h;
				SDL_FillRect(surface,&rect,color);
			}
		}
	}
	SDL_Flip (surface);
}

//Delays for a specified ammount of milliseconds
void delay (long delayTime) {
	long start = clock ();
	long end;
	do {
		end = clock ();
	}while (end - start < delayTime);
}

//Recursively performs iterations of the game of life until the window is closed
int circleLife (Life* life, Lives* lives, SDL_Surface* surface) {
	Life newlife = createNewLife(life);
	Life* nlife = (Life*)malloc(sizeof(Life));
	SDL_Event event;
	*nlife = newlife;
	if (lives->oscPeriod == -1){
		addLife (lives, nlife);
		evaluateOscPeriod (lives);		
	} else {
		if (lives->oscPeriod > 1) {
			char* oslstr = calloc(sizeof(char),30);;
			sprintf(oslstr,"Oscilating with period of %d",lives->oscPeriod);
			SDL_WM_SetCaption ((const char*)oslstr,NULL);
			free(oslstr);
		} else 
			SDL_WM_SetCaption ("Stable world!", NULL);
	}
	
	SDL_PollEvent (&event);
	if (event.type == SDL_QUIT) {
			SDL_Quit();
			return 0;
	}
	printGOL (nlife,surface);
	delay (100);
	SDL_FillRect(surface,NULL, 0x000000);
	SDL_Flip(surface);
	return circleLife (nlife,lives, surface);
}

int main( int argc, char* args[] ) { 
	SDL_Surface *screen;
	Life life = loadFromFile("world.txt");
	Uint32 flags = SDL_SWSURFACE|SDL_FULLSCREEN;
	
	
	if (&life == NULL)
		return 0;
	Lives lives;
	lives.lives = (Life*)malloc(sizeof(Life));
	lives.len = 1;
	lives.lives[0] = life;
	lives.oscPeriod = -1;
	
	
	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_WM_SetCaption ("Nein!",NULL);
	
	screen = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE );
	
	return circleLife (&life,&lives,screen);
 
}
