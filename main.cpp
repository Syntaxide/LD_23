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

int px, py;
int dx,dy;	//pixel offsets for tweening
float cx,cy;	//camera start in tiles
enum Direction { RIGHT, DOWN, LEFT, UP };
enum Tile { OPEN, DIRT, ROCK, ELDER, PORTAL, PAD, GATE, ERROR};
static bool isMoving = false;
static bool isRunning = false;

//use as a sort of command queue
enum Key { KRIGHT, KDOWN, KLEFT, KUP};
static bool keyDown[] = {false, false, false, false};

bool messageWindow;
Direction pd;
TTF_Font *font = NULL;
SDL_Surface *screen;
char *message = NULL;
vector<SDL_Surface*> images;
vector<Mix_Chunk*> sounds;

vector<World*> worldstack;
static bool abilities[] = {false, false, false};
int pl;	//player level

struct World{
	World(){
		w=0;
		h=0;
		sx = sy = 1;
		grid = NULL;
		timeleft = 0;
		timemax = 0;
	}
	World(const char *filename){		//assumed to png images
		sx = sy = 0;
		timeleft = 0;
		timemax = 0;
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
				return GATE;
			else
			{
				return ERROR;
			}
		}
};


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
				case SDLK_DOWN:
				      keyDown[ KDOWN ] = false;
				case SDLK_LEFT:
				      keyDown[ KLEFT ] = false;
				case SDLK_RIGHT:
				      keyDown[ KRIGHT ] = false;
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
	for(unsigned int x=0;x<images.size();x++)
		SDL_FreeSurface(images[x]);
	for(unsigned int x=0;x<worldstack.size();x++)
		delete worldstack[ x ];
	for(unsigned int x=0;x<sounds.size();x++)
		Mix_FreeChunk( sounds[ x ] );
}

void tryMove()
{
	int xc=px, yc=py;	//xcandidate, ycandidate
	if(pd == RIGHT)
		xc+=1;
	else if(pd == LEFT)
		xc-=1;
	else if(pd == DOWN)
		yc+=1;
	else
		yc-=1;

	//collision stuff here
	if( yc >=0 && yc < worldstack[pl]->h && xc >=0 && xc < worldstack[pl]->w)		//bounds check
	{
		switch( worldstack[pl]->grid[yc][xc] )
		{
			case OPEN:
			case ERROR:
			case PAD:
			case PORTAL:
				if(abilities[0]){	//can move
					isMoving = true;
				}
				break;
			case DIRT:
				if(abilities[2])
					isMoving = true;
				break;
			case ROCK:
				break;
			case ELDER:
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
	else	//can try to move again, assuming no collision
	{
		for(int x=0;x<=KUP;x++)
		{
			if(keyDown[x]){
				pd = (Direction)x;
				tryMove();
				if(isMoving)
					break;
			}
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

void DrawTile(int x, int y)	//these represent tile coords on screen
{
	Uint32 color;
	int imgindex = -1;
	int wx = cx + x;	//represent coords in grid
	int wy = cy + y;
	if( wx >= 0 && wx < worldstack[pl]->w && wy >= 0 && wy < worldstack[pl]->h)	//inbound
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
			case ROCK:
				imgindex = 12;
				break;
			case GATE:
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
	static int walkframe = 0;
	Clear();

	//draw world
	
	//draw each square visible on screen
	for(int y=-2;y<14;y++)
	{
		for(int x=-2;x<14;x++)
		{
			DrawTile(x,y);
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


	//drawMessageWindow
	if(messageWindow)
	{
//		SDL_Rect r = {100,100, 400, 50};
//		SDL_FillRect( screen, &r, SDL_MapRGB(screen->format, 255,255,255 ) );
		DrawText( message, 100,100, 0, 0, 0 );
		DrawText( "esc to dismiss", 100,150, 0, 0, 0 );
	}
}

void SetWorld(unsigned int layer)
{
	if(layer >= 0 && layer < worldstack.size())
	{
		px = worldstack[ layer ]->sx;
		py = worldstack[ layer ]->sy;
		pl = layer;
	}
}

int main(int argc, char **argv)
{
	SDL_Init( SDL_INIT_EVERYTHING );
	TTF_Init();
	font = TTF_OpenFont( "data/foo.ttf", 28 );
	if(font == NULL)
		cout<<"unable to load font"<<endl;
	Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096 );
	worldstack.push_back( new World("data/level01.png") );
	worldstack.push_back( new World("data/level02.png") );
	worldstack.push_back( new World("data/level03.png") );
	SetWorld( 0 );
	screen = SDL_SetVideoMode(600,600, 32, SDL_SWSURFACE);
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

	LoadSound( "data/sound.wav" );	//sound 0


	bool shouldrun = true;

	Uint32 lastupdate = SDL_GetTicks();
	const Uint32 upticks = 16;
	while( shouldrun )
	{
		Uint32 time = SDL_GetTicks();
		while( time - lastupdate > upticks)
		{
			ProcessInput(shouldrun);
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
