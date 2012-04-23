/*
 * Copyright 2012 Alexander Midlash
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_mixer.h"
#include "SDL/SDL_ttf.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <string.h>

using std::cout;
using std::endl;
using std::vector;


struct World;
void SetWorld(unsigned int layer );
void SetMessage( const char *m );
void tryMove();
void Clear();

int px, py;
int dx,dy;	//pixel offsets for tweening
float cx,cy;	//camera start in tiles
enum Direction { RIGHT=0, DOWN, LEFT, UP };
enum Tile { OPEN, DIRT, ROCK, ELDER, PORTAL, PAD, GATE, CAT, STAR, ERROR};
static SDL_Color color = {0,0,0};	//used for text
static bool isMoving = false;
static bool isRunning = false;

//use as a sort of command queue
enum Key { KRIGHT=0, KDOWN, KLEFT, KUP, KSPACE};
static bool keyDown[] = {false, false, false, false, false};

bool messageWindow;
static bool won = false;
Direction pd;
TTF_Font *font = NULL;
SDL_Surface *screen;
char *message = NULL;
vector<SDL_Surface*> images;
vector<Mix_Chunk*> sounds;

vector<World*> worldstack;
static bool abilities[] = {false, false, false, false, false};
static bool cats[] = { false, false, false, false, false, false };
static int pl=-1;	//player level	-1=menu

struct World{
	World(){
		w=0;
		h=0;
		sx = sy = 1;
		grid = NULL;
		timeleft = 0;
		gx = gy = -1;
		timemax = 0;
	}
	World(const char *filename){		//assumed to png images
		sx = sy = 0;
		timeleft = 0;
		timemax = 0;
		gx = gy = -1;
		SDL_Surface *img = IMG_Load(filename);

		//image specs
		/*cout<<"bits per pixel: "<<(int)img->format->BitsPerPixel<<endl;
		cout<<"bytes per pixel: "<<(int)img->format->BytesPerPixel<<endl;
		cout<<"pitch: "<<(int)img->pitch<<endl;
		cout<<"width: "<<(int)img->w<<endl;*/
		
		initgrid(img->w, img->h);

		//initgrid(2*(d+1)*(d+1));
		SDL_LockSurface( img );
		Uint8 *pixels = (Uint8*)img->pixels;

		RGB rgb;
		for(int y=0;y<h;y++)
		{
			for(int x=0;x<w;x++)
			{
				rgb.r = *pixels;
				pixels++;
				rgb.g = *pixels;
				pixels++;
				rgb.b = *pixels;
				pixels++;


				grid[y][x] = parsepixel( rgb, x, y);
			}
			for(int i=w*3;i<img->pitch;i++)		//ugh... pitch. finally working at least
				pixels++;
		}
		SDL_UnlockSurface( img );
		SDL_FreeSurface( img );
	}
	void StartTimer()
	{
		timeleft = timemax;
	}
	void Tick()
	{
		if(timeleft > 0)
			timeleft--;
	}

	~World(){
		for(int i=0;i<h;i++){
			 delete[] grid[i];
			 grid[i] = NULL;
		}
		delete[] grid;
		grid = NULL;
		w = h = 0;
	}
	int w,h;
	int sx, sy;
	int gx, gy;
	Tile **grid;
	int timemax;
	int timeleft;

	private:
		class RGB{
			public:
			RGB(){
				r = g = b = 0;
			}
			RGB(unsigned char x, unsigned char y, unsigned char z){
				r = x;
				g = y;
				b = z;
			}
			bool operator==(const RGB &other) const
			{
				return other.r == r && other.g == g && other.b == b;
			}
			void dbg() const
			{ 
				cout<<(int)r<<" "<<(int)g<<" "<<(int)b<<endl;
			}
			unsigned char r;
			unsigned char g;
			unsigned char b;
		};
		void initgrid(int a, int b){
			w = a;
			h = b;
			grid = new Tile*[h];
			for(int i=0;i<h;i++)
				grid[i] = new Tile[w];
		}
		Tile parsepixel(const RGB &p, int x, int y)
		{
			//cout<<x<<" "<<y<<" ";
			//p.dbg();
			if( p == RGB(255,255,255) )
				return OPEN;
			else if( p == RGB(255,0,0) )		//start
			{
				sx = x;
				sy = y;
				return OPEN;
			}
			else if( p == RGB(0,0,255) )
				return PORTAL;
			else if( p == RGB(255,255,0) )
				return DIRT;
			else if( p == RGB(0,255,0) )
				return ELDER;
			else if( p.r == 255 && p.b == 255)
			{
				timemax = p.g;
				return PAD;
			}
			else if( p == RGB(0,0,0 ) )
				return ROCK;
			else if( p == RGB(0,255,255) )
			{
				gx = x;
				gy = y;
				return GATE;
			}
			else if( p == RGB(100,100,100) )
			{
				return CAT;
			}
			else if( p == RGB(100,0,0) )
			{
				return STAR;
			}
			else
			{
				return ERROR;
			}
		}
};


void FindCat( int layer )
{
	cats[ layer ] = true;
	bool foundall = true;
	for(int x=0;x<6;x++)
		if(! cats[x])
			foundall = false;
	if(foundall)
	{
		SetMessage("You sense a disturbance...");
		worldstack[3]->grid[26][42] = OPEN;
	}
}



void LoadImage( const char *string )
{
	SDL_Surface *s1 = IMG_Load( string );
	SDL_Surface *s2 = NULL;
	if( s1 == NULL)
		cout<<"unable to load image "<<string<<endl;
	else
		s2 = SDL_DisplayFormat( s1 );

	SDL_FreeSurface( s1 );
	SDL_SetColorKey( s2, SDL_SRCCOLORKEY, SDL_MapRGB(screen->format, 255, 0 ,255) );

	images.push_back( s2 );
}

void LoadSound( const char *string )
{
	Mix_Chunk *s = Mix_LoadWAV( string );
	if( s == NULL )
		cout<<"unable to load sound "<<string<<endl;
	else
		sounds.push_back( s );
}

void ProcessInput(bool &shouldrun) 
{
	SDL_Event e;

	while( SDL_PollEvent( &e ) )
	{
		if(e.type == SDL_QUIT )
			shouldrun = false;
		else if(e.type == SDL_KEYDOWN)
		{
			switch( e.key.keysym.sym )
			{
				case SDLK_ESCAPE:
					if(messageWindow)
						messageWindow = false;
					else
						shouldrun = false;
				      break;
				case SDLK_RETURN:
				      if(pl == -1)
					      SetWorld( 0 );
				      break;

				case SDLK_UP:
				      keyDown[ KUP ] = true;
				      break;
				case SDLK_DOWN:
				      keyDown[ KDOWN ] = true;
				      break;
				case SDLK_RIGHT:
				      keyDown[ KRIGHT ] = true;
				      break;
				case SDLK_LEFT:
				      keyDown[ KLEFT ] = true;
				      break;
				case SDLK_SPACE:
				      keyDown[ KSPACE ] = true;
				      break;
				case SDLK_z:
				  if( abilities[4] )
				  {
					  if(pl > 0)
						  SetWorld( pl-1 );
				  }
				  break;
				case SDLK_LSHIFT:
				case SDLK_RSHIFT:
				      if(abilities[1])
					      isRunning = true;
				      break;
				default:
				      break;
			}

		}
		else if(e.type == SDL_KEYUP)
		{
			switch( e.key.keysym.sym )
			{
				case SDLK_UP:
				      keyDown[ KUP ] = false;
				      break;
				case SDLK_DOWN:
				      keyDown[ KDOWN ] = false;
				      break;
				case SDLK_LEFT:
				      keyDown[ KLEFT ] = false;
				      break;
				case SDLK_RIGHT:
				      keyDown[ KRIGHT ] = false;
				      break;
				case SDLK_SPACE:
				      keyDown[ KSPACE ] = false;
				      break;
				case SDLK_LSHIFT:
				case SDLK_RSHIFT:
				      isRunning = false;
				default:
				      break;
			}

		}
	}

}

void Cleanup()
{
	SDL_WM_IconifyWindow();	//hide window while unloading assets, then close
	SDL_Flip( screen );

	for(unsigned int x=0;x<images.size();x++)
		SDL_FreeSurface(images[x]);
	for(unsigned int x=0;x<worldstack.size();x++)
		delete worldstack[ x ];
	for(unsigned int x=0;x<sounds.size();x++)
		Mix_FreeChunk( sounds[ x ] );
	delete[] message;
}

void tryMove()
{
	int xc=px, yc=py;	//xcandidate, ycandidate
	if(abilities[3] &&keyDown[KSPACE]){
		if(pd == RIGHT)
			xc+=2;
		else if(pd == LEFT)
			xc-=2;
		else if(pd == DOWN)
			yc+=2;
		else
			yc-=2;
	}
	else
	{
			if(pd == RIGHT)
				xc+=1;
			else if(pd == LEFT)
				xc-=1;
			else if(pd == DOWN)
				yc+=1;
			else
				yc-=1;
	}

	//collision stuff here
	if( yc >=0 && yc < worldstack[pl]->h && xc >=0 && xc < worldstack[pl]->w)		//bounds check
	{
		switch( worldstack[pl]->grid[yc][xc] )
		{
			case CAT:
				worldstack[pl]->grid[yc][xc] = OPEN;
				FindCat( pl );
			case STAR:
				//prevent flow
				if( worldstack[pl]->grid[yc][xc] == STAR ){
					if(!won)
						SetMessage("YOU WIN!");
					won = true;
				}
			case OPEN:
			case ERROR:
			case PAD:
			case PORTAL:
				if(!keyDown[KSPACE])
				{
					if(abilities[0]){	//can move
						isMoving = true;
					}
				}
				else
				{
					px = xc;
					py = yc;
					keyDown[ KSPACE ] = false;
				}

				
				break;
			case DIRT:
				if(!keyDown[KSPACE]){
					if(abilities[0] && abilities[2])
						isMoving = true;
				}
				else
				{
					px = xc;
					py = yc;
					keyDown[ KSPACE ] = false;
				}
				break;
			case ROCK:
				break;
			case ELDER:
				if(pl != 4 && pl != 5)
				{
					if(!abilities[pl])
					{
						abilities[pl] = true;
						Mix_PlayChannel( -1, sounds[0], 0 );
						if(pl == 0)
							SetMessage("you've learned to walk!");
						else if(pl == 1)
							SetMessage("press shift to run!");
						else if(pl == 2)
							SetMessage("you can now move through dirt");
						else if(pl == 3)
							SetMessage("You can now teleport using SPACE");
					}
				}
				else if(pl == 4)
				{
					SetMessage("You can walk again. And Good luck. You'll understand later.");
					abilities[ 0 ] = true;
				}
				else if(pl == 5)
				{
					SetMessage("You can now go up levels. Press z to to teleport");
					abilities[ 4 ] = true;
				}
				break;
			case GATE:
				if(worldstack[pl]->timeleft > 0)
				{
					isMoving = true;
				}


				break;

		}

	}
}



void Update()
{
	static int counter = 0;


	switch( worldstack[pl]->grid[py][px])
	{
		case PAD:
			worldstack[pl]->StartTimer();
			break;
		case PORTAL:
			SetWorld( pl + 1);
			break;
		default:
			break;
	}


	if(isMoving){		//tween
		if(pd == RIGHT)
		{
			dx++;
			if(isRunning)
				dx++;
			if(dx >= 50){
				px++;
				dx = 0;
				isMoving = false;
			}
		}
		else if(pd == LEFT)
		{
			dx--;
			if(isRunning)
				dx--;
			if(dx <= -50){
				px--;
				dx = 0;
				isMoving = false;
			}
		}
		else if(pd == UP)
		{
			dy--;
			if(isRunning)
				dy--;
			if(dy <= -50){
				py--;
				dy = 0;
				isMoving = false;
			}
		}
		else if(pd == DOWN)
		{
			dy++;
			if(isRunning)
				dy++;
			if(dy >= 50){
				py++;
				dy = 0;
				isMoving = false;
			}
		}
	}
	if(!isMoving)	//can try to move again, assuming no collision
	{
		for(int x=0;x<=KUP;x++)
		{
			if(keyDown[x]){
				pd = (Direction)x;
				tryMove();
				if(isMoving)	//trymove success
					break;
			}
		}
		if(keyDown[ KSPACE ] && abilities[3] )
		{
			tryMove();
		}
	}

	if(counter % 50 == 0)
		worldstack[pl]->Tick();
	counter++;

	//center camera
	cx = (px)-5;
	cy = (py)-5;

}

void DrawImage(int imgindex, int x, int y)
{
	SDL_Rect r= {x-dx,y-dy,0,0};
	SDL_BlitSurface( images[imgindex] , NULL, screen, &r );
}

void Clear()
{
	SDL_FillRect( screen, NULL, SDL_MapRGB( screen->format, 100, 100, 100 ) );
}

void DrawTile(int x, int y, bool gatepass)	//these represent tile coords on screen
{
	Uint32 color;
	int imgindex = -1;
	int wx = cx + x;	//represent coords in grid
	int wy = cy + y;
	if(gatepass && x != -1){		//only item being drawn is gate, and is actually on level
	}

	else	//draw everything else
	{
		if( wx >= 0 && wx < worldstack[pl]->w && wy >= 0 && wy < worldstack[pl]->h)	//inbounds
		{
			switch(worldstack[pl]->grid[wy][wx]){
				case OPEN:
					imgindex = 9;
					break;
				case ELDER:
					imgindex = 13;
					break;
				case DIRT:
					imgindex = 10;
					break;
				case PORTAL:
					imgindex = 11;
					break;
				case PAD:
					imgindex = 14;
					break;
				case STAR:
					imgindex = 18;
					break;
				case ROCK:
					imgindex = 12;
					if( pl == 3 )
					{
						if(wy == 21)
						{
							if(wx == 37 && cats[0])
								imgindex = 17;
							else if(wx == 39 && cats[1])
								imgindex = 17;
							else if(wx == 41 && cats[2])
								imgindex = 17;
							else if(wx == 43 && cats[3])
								imgindex = 17;
							else if(wx == 45 && cats[4])
								imgindex = 17;
							else if(wx == 47 && cats[5])
								imgindex = 17;
						}
					}

					break;
				case CAT:
					imgindex = 17;
					break;
				case GATE:	//now drawn in gatepass case above
					if(worldstack[pl]->timeleft == 0)
						imgindex = 16;
					else
						imgindex = 15;
					break;
				default:
					color = SDL_MapRGB(screen->format, 0,0,0);
			}
			SDL_Rect r = {(wx-cx)*50,(wy-cy)*50,50,50};
			if(imgindex != -1)
				DrawImage(imgindex, r.x, r.y);
			else
				SDL_FillRect(screen, &r, color);

		}
	}

}

void DrawText( const char *message, int x, int y, int r, int g, int b)
{
	SDL_Color c = {r,g,b};
	SDL_Surface *out = TTF_RenderText_Solid(font, message, c);
	SDL_Rect rect = {x,y,0,0};
	SDL_BlitSurface( out, NULL, screen, &rect );
}

void SetMessage( const char *m)
{
	if(message != NULL)
	{
		delete[] message;
	}

	message = new char[ strlen(m) + 1];
	message[strlen(m)] = '\0';
	strcpy(message, m);
	messageWindow = true;
}




void Draw( )
{
	Clear();
	if(pl == -1)
	{
		DrawText( "Ant in Training", 100, 50, color.r,color.g, color.b);
		DrawText( "Press Enter to Start", 100, 100, color.r,color.g, color.b);
		
		DrawImage(0, 0, 0);
	}
	else
	{
		static int walkframe = 0;

		//draw world
		
		//draw each square visible on screen, other than gate
		for(int y=-1;y<13;y++)
		{
			for(int x=-1;x<13;x++)
			{
				DrawTile(x,y,false);
			}
		}


		/*
		for(int x=0;x< worldstack[ pl ].w; x++)
		{
			for(int y=0;y<worldstack[pl].h;y++)
			{
				Uint32 color;
				if(worldstack[pl].grid[y][x] == false)
					color = SDL_MapRGB(screen->format, 255,155,155);
				else
					color = SDL_MapRGB(screen->format, 0,0,255);

				SDL_Rect r = {(x-cx)*50,(y-cy)*50,50,50};
				SDL_FillRect(screen, &r, color);
			}
		}*/

		//draw player
		if(walkframe / 30 % 2 == 0 )	//switch every two seconds
			DrawImage(2*pd+1, (px-cx)*50+dx, (py-cy)*50+dy);
		else 
			DrawImage(2*pd+2, (px-cx)*50+dx, (py-cy)*50+dy);
		if(abilities[0] && isMoving)
			walkframe++;


		//now draw gate
		DrawTile( worldstack[ pl ]->gx, worldstack[pl]->gy, true);


		//drawMessageWindow
		if(messageWindow)
		{
	//		SDL_Rect r = {100,100, 400, 50};
	//		SDL_FillRect( screen, &r, SDL_MapRGB(screen->format, 255,255,255 ) );
			DrawText( message, 100,50, color.r, color.g, color.b );
			DrawText( "esc to dismiss", 100,100, color.r, color.g, color.b );
		}
	}
}

void SetWorld(unsigned int layer)
{
	for(int x=0;x<5;x++)
		keyDown[ x ] = false;
	isMoving = false;
	dx = 0;
	dy = 0;
	if(layer >= 0 && layer < worldstack.size())
	{
		px = worldstack[ layer ]->sx;
		py = worldstack[ layer ]->sy;
		pl = layer;

		if(pl == 4 && !abilities[4])	//dont break legs on returning
		{
			SetMessage("Oof. Your legs feel broken...");
			abilities[ 0 ] = false;
		}
	}
}

int main(int argc, char **argv)
{
	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_WM_SetCaption("Ant in Training", NULL);
	TTF_Init();
	font = TTF_OpenFont( "data/foo.ttf", 15 );
	if(font == NULL)
		cout<<"unable to load font"<<endl;
	Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096 );
	worldstack.push_back( new World("data/level01.png") );
	worldstack.push_back( new World("data/level02.png") );
	worldstack.push_back( new World("data/level03.png") );
	worldstack.push_back( new World("data/level04.png") );
	worldstack.push_back( new World("data/level05.png") );
	worldstack.push_back( new World("data/level06.png") );

	screen = SDL_SetVideoMode(600,600, 32, SDL_DOUBLEBUF);
	if(screen == NULL)
		cout<<"could not create screen"<<endl;
	LoadImage( "data/catbtn.png" );	//image 0
	LoadImage( "data/ant_10.png" );	//image 1
	LoadImage( "data/ant_20.png" );	
	LoadImage( "data/ant_11.png" );	
	LoadImage( "data/ant_21.png" );	
	LoadImage( "data/ant_12.png" );	
	LoadImage( "data/ant_22.png" );	
	LoadImage( "data/ant_13.png" );	
	LoadImage( "data/ant_23.png" );	
	LoadImage( "data/open.png" );	//image 9
	LoadImage( "data/dirt.png" );	
	LoadImage( "data/portal.png" );	
	LoadImage( "data/rock.png" );	
	LoadImage( "data/elder.png" );	
	LoadImage( "data/pad.png" );	
	LoadImage( "data/gateopen.png" );	
	LoadImage( "data/gateclosed.png" );	
	LoadImage( "data/cattrophy.png" );	//17
	LoadImage( "data/star.png" );	//17

	LoadSound( "data/sound.wav" );	//sound 0


	bool shouldrun = true;

	Uint32 lastupdate = SDL_GetTicks();
	const Uint32 upticks = 16;

	SetMessage("Arrowkeys to turn");
	while( shouldrun )
	{
		Uint32 time = SDL_GetTicks();
		while( time - lastupdate > upticks)
		{
			ProcessInput(shouldrun);
			if(pl != -1)
				Update();
			lastupdate += upticks;
		}
		Draw();
		SDL_Flip( screen );
	}

	Cleanup();

	Mix_CloseAudio();
	SDL_Quit();

	return 0;
}
