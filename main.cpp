//Made by Benjamin Schussler

//w,a,s,d to move, q to distort,r to reset, escape to quit
//i, o, and p to reset, increase, and decrease FOV (FOV will only go above 180 in fisheye mode)
//f to toggle fisheye lens
//b/h,n/j,m/k to increase/decrease room size, room height, and wall length respectively, and ',' to set to default
//'l' to toggle inverted mouse
//t to change the texture (can only be changed after pressing r and before pressing q (or before pressing q for the first time))
//e to toggle random hallways 

//also, collision detection can be turned off by setting collisions=false in main(), but rendering inside of solid objects may cause flashing imagery.

// with help from https://stackoverflow.com/questions/34424816/sdl-window-does-not-show
// and https://medium.com/@edkins.sarah/set-up-sdl2-on-your-mac-without-xcode-6b0c33b723f7
//https://stackoverflow.com/questions/20579658/how-to-draw-pixels-in-sdl-2-0
//https://lazyfoo.net/tutorials/SDL/
//https://stackoverflow.com/questions/50361975/sdl-framerate-cap-implementation
//https://www.cplusplus.com/articles/GwvU7k9E/

//I also wanted to include custom textures with http://qdbmp.sourceforge.net/, but it kept crashing :/

#include <stdlib.h>
//#include "qdbmp.h"
//#include <stdio.h>
#include <iostream>
#include <SDL2/SDL.h>
#include <cmath>
using namespace std;

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define PIXSIZ 5
#define PI 3.14159
#define RDIST 100
#define PICTURE_RES_X 400
#define PICTURE_RES_Y 200
#define MINMARCH .01
#define MAX_FRAMERATE 30

double mymod(double x, double mod){
    return x-mod*floor(x/mod);
}

class Vec3{
    public:

        double x,y,z;
        bool isNull;

        Vec3(double x1,double y1,double z1){
            x=x1;
            y=y1;
            z=z1;
            isNull=false;
        }

        Vec3(){
            isNull=true;
        }

        double mag(){
            return sqrt(pow(x,2)+pow(y,2)+pow(z,2));
        }

        double sum(){
            return x+y+z;
        }

        Vec3 abs(){
            return Vec3(fabs(x),fabs(y),fabs(z));
        }

        Vec3 copy(){
            return Vec3(x,y,z);
        }

        Vec3 norm(){ //normalize to magnitude 1
            return mul(1/mag());
        }

        Vec3 add(Vec3 vec){
            return Vec3(x+vec.x,y+vec.y,z+vec.z);
        }

        Vec3 neg(){
            return Vec3(-x,-y,-z);
        }

        Vec3 mod(double mod){
            return Vec3(mymod(x,mod),mymod(y,mod),mymod(z,mod));
        }

        Vec3 mul(double k){
            return Vec3(x*k,y*k,z*k);
        }

        Vec3 lerp(Vec3 newVec,double blend){
            return copy().mul(1-blend).add(newVec.mul(blend));
        }

        double dot(Vec3 vec){
            return x*vec.x+y*vec.y+z*vec.z;
        }

        Vec3 cross(Vec3 vec){
            return Vec3(y*vec.z-z*vec.y,z*vec.x-x*vec.z,x*vec.y-y*vec.x);
        }

        double dist(Vec3 vec){
            return sqrt(pow(x-vec.x,2)+pow(y-vec.y,2)+pow(z-vec.z,2));
        }

        Vec3 X(){
            return Vec3(x,0,0);
        }
        Vec3 Y(){
            return Vec3(0,y,0);
        }
        Vec3 Z(){
            return Vec3(0,0,z);
        }
};

class VecAndNum{
    public:
        Vec3 vec;
        double num;

        VecAndNum(Vec3 vec1,double num1){
            vec=vec1;
            num=num1;
        }
        VecAndNum(){
            vec=Vec3();
            num=-1;
        }
};

class Dir{
    public:
        double th;  //xy angle
        double a;   //height angle

        Dir(double th1, double a1){
            th=th1;
            a=a1;
        }

        static Dir dir(Vec3 vec1){
            Vec3 vec=vec1.norm();
            return Dir(-atan2(vec.y,vec.x),asin(vec.z));
        }

        Dir copy(){
            return Dir(th,a);
        }

        Dir add(Dir dir){
            return Dir(th+dir.th,a+dir.a);
        }

        Vec3 fwd(){
            return Vec3(cos(a)*cos(th),cos(a)*sin(th),sin(a));
        }
        Vec3 side(){
            return Vec3(sin(-th),cos(th),0);
        }
        Vec3 up(){
            return Vec3(sin(-a)*cos(th),sin(-a)*sin(th),cos(a));
        }

        Vec3 compose(Dir dir){
            return copy().fwd().mul(cos(dir.th)).add(copy().side().mul(sin(dir.th))).mul(cos(dir.a)).add(copy().up().mul(sin(dir.a)));
        }
};

/*void BMP_GetPixelRGB(BMP* bmp, UINT x, UINT y, Vec3 color){
    UCHAR* r;
    UCHAR* g;
    UCHAR* b;
    BMP_GetPixelRGB(bmp, x, y, r,g,b);
    color=Vec3(*r,*g,*b);
}*/

/*double squaredist(Vec3 pos1, Vec3 pos2){    //taxicab metric distance
    return fmax(fabs(pos1.x-pos2.x),fabs(pos1.y-pos2.y));
}*/

int wrap(int num, int max){
    return (num%max+max)%max;
}

/*int myrand(int seed, int x, int y){
    srand(seed);
    srand(rand()*(x-rand())+rand()+x+2341);
    srand(rand()*(y-rand())+rand()+y+8432);
    return (rand()%5)==0;
}*/

int vertexType(int x, int y, int randSeed){ //where are openings at intersection (x,y)
    srand(randSeed);
    srand(rand()*(x-rand())+rand()+x+2341);
    srand(rand()*(y-rand())+rand()+y+8432);
    return rand()%6;
}

bool wallOfVertex(int axis, int x, int y, int randSeed){    //axis is an int (0:x, 1:y, 2:-x, 3:-y), would be better form to use an enum
    int v=vertexType(x, y, randSeed);
    switch(axis){
        case 0:
            return v==0 or v==1 or v==2;
            break;
        case 1:
            return v==0 or v==3 or v==4;
            break;
        case 2:
            return v==1 or v==3 or v==5;
            break;
        case 3:
            return v==2 or v==4 or v==5;
            break;
        default:    //inaccessible (here for completeness)
            return false;
            break;
    }
}

bool wallAt(int axis, int x, int y, int randSeed){  //axis 0=x, 1=y
    return wallOfVertex(axis, x, y, randSeed) or 
           wallOfVertex((axis+2)%4, x-(axis==0?1:0), y-(axis==1?1:0), randSeed);
}

double dist(Vec3 pos, Vec3 vel,double roomSize, double wallSize, double roomHeight, bool randomHalls){    //distance which uses ray velocity to get a better estimate

    if(randomHalls){
        bool x=wallAt(0,floor(pos.x/roomSize+.5),floor(pos.y/roomSize),333);//myrand(0,floor(pos.x/25+.5),floor(pos.y/25));
        bool y=wallAt(1,floor(pos.x/roomSize),floor(pos.y/roomSize+.5),333);//myrand(0,floor(pos.x/25),floor(pos.y/25+.5));
        //int wall=wallType('x',floor(pos.x/25+.5),floor(pos.y/25),333);

        double mx=mymod(pos.x,roomSize);
        double my=mymod(pos.y,roomSize);

        double dx=(fabs(mx-roomSize/2)-wallSize/2)/fabs(vel.x);
        double dy=(fabs(my-roomSize/2)-wallSize/2)/fabs(vel.y);

        double d1=fmin((fmin(mx,roomSize-mx)+(roomSize-wallSize)/2)/fabs(vel.x),
                       (fmin(my,roomSize-my)+(roomSize-wallSize)/2)/fabs(vel.y));

        return  fmin(fmin(fmax(x?dx:fmin(dx,dy),
                               y?dy:fmin(dx,dy)),
                          d1),
                     (roomHeight/2-fabs(pos.z))/fabs(vel.z));
    }

    return  fmin(fmax((fabs(mymod(pos.x,roomSize)-roomSize/2)-wallSize/2)/fabs(vel.x),
                    (fabs(mymod(pos.y,roomSize)-roomSize/2)-wallSize/2)/fabs(vel.y)),
                    (roomHeight/2-fabs(pos.z))/abs(vel.z));
}



double dist(Vec3 pos,double roomSize, double wallSize, double roomHeight, bool randomHalls){

    if(randomHalls){
        bool x=wallAt(0,floor(pos.x/roomSize+.5),floor(pos.y/roomSize),333);//myrand(0,floor(pos.x/25+.5),floor(pos.y/25));
        bool y=wallAt(1,floor(pos.x/roomSize),floor(pos.y/roomSize+.5),333);//myrand(0,floor(pos.x/25),floor(pos.y/25+.5));
        
        double dx=fabs(mymod(pos.x,roomSize)-roomSize/2);
        double dy=fabs(mymod(pos.y,roomSize)-roomSize/2);

        return  fmin(fmax(x?dx:fmin(dx,dy),
                          y?dy:fmin(dx,dy))-wallSize/2,
                        roomHeight-fabs(pos.z));
    }
    return  fmin(fmax(fabs(mymod(pos.x,roomSize)-roomSize/2),
                      fabs(mymod(pos.y,roomSize)-roomSize/2))-wallSize/2,
                    roomHeight-fabs(pos.z));
    //return fmin(squaredist(pos.mod(25),Vec3(25,25,0))-8,5-fabs(pos.z));

}

/*Vec3 collision(Vec3 pos,double roomSize, double wallSize, bool randomHalls){ //movement to avoid collision //xOrY=true: x, else y
    bool xOrY=fabs(mymod(pos.x,roomSize)-roomSize/2)>fabs(mymod(pos.y,roomSize)-roomSize/2);

    return Vec3(xOrY?fmin(0,fabs(mymod(pos.x,roomSize)-roomSize/2)-wallSize/2-MINMARCH)*((mymod(pos.x,roomSize)>roomSize/2)?-1:1):0,
                xOrY?0:fmin(0,fabs(mymod(pos.y,roomSize)-roomSize/2)-wallSize/2-MINMARCH)*((mymod(pos.y,roomSize)>roomSize/2)?-1:1),
                0);
}*/

Vec3 colorFromDir(Vec3 texture[PICTURE_RES_X][PICTURE_RES_Y], Dir facing){
    return texture[wrap(1+(int)floor(-PICTURE_RES_X*facing.th/(2*PI)),PICTURE_RES_X)][wrap((int)floor(PICTURE_RES_Y*(facing.a+PI/2)/PI),PICTURE_RES_Y)];

}

Vec3 colorFromPos(VecAndNum posAndDist, Vec3 texture[PICTURE_RES_X][PICTURE_RES_Y], Vec3 centerPos, int useTexture, bool textureFixed){
    if(posAndDist.vec.isNull){
        return Vec3(0,0,0);
    }
    
    //return Vec3(fmod(posAndDist.vec.x,2)*128,fmod(posAndDist.vec.y,2)*128,fmod(posAndDist.vec.z,2)*128);

    if(!textureFixed){
        switch(useTexture){
            case 0:
                return colorFromDir(texture,Dir::dir(posAndDist.vec.add(centerPos.neg())));
                break;
            case 1:
                return Vec3(255,255,255).mul(1-fmin(posAndDist.num<30?posAndDist.num/70:(3./7-3./20+posAndDist.num/200),1));
                break;
            case 2:
                return Vec3(fmod(posAndDist.vec.x,2)*128,fmod(posAndDist.vec.y,2)*128,fmod(posAndDist.vec.z,2)*128);
                break;
        }
    }

    return colorFromDir(texture,Dir::dir(posAndDist.vec.add(centerPos.neg())));

    //return myrand(0,floor((pos.x-12.5)/25),floor((pos.y)/25))==0?Vec3(mymod(posAndDist.vec.z,255),0,0):
    //        (myrand(0,floor((pos.x-12.5)/25),floor((pos.y)/25))==1?Vec3(0,mymod(posAndDist.vec.y,255),0):(
    //            myrand(0,floor((pos.x-12.5)/25),floor((pos.y)/25))==2?Vec3(0,0,mymod(posAndDist.vec.z,255)):Vec3(255,255,255)));
    
    //return Vec3(255,fmod(posAndDist.vec.z,2)*128,fmod(posAndDist.vec.z,2)*128);
    //return Vec3(255,255,255).mul(1-((double)posAndDist.num)/RDIST);
}

Vec3 vecFromPixel(int x, int y, Dir facing, double FOV, bool fisheye){
    return fisheye?facing.compose(Dir(PI*FOV*(-x+WINDOW_WIDTH/2)/(WINDOW_WIDTH*180),
                              PI*FOV*(-y+WINDOW_HEIGHT/2)/(WINDOW_HEIGHT*180))):
                    facing.fwd().add(facing.side().mul(PI*FOV*(-x+WINDOW_WIDTH/2)/(WINDOW_WIDTH*180))).add(
                            facing.up().mul(PI*FOV*(-y+WINDOW_HEIGHT/2)/(WINDOW_HEIGHT*180))).norm();
}

VecAndNum march(Vec3 pos,Vec3 vel,int maxSteps,double minDist, double roomSize, double wallSize, double roomHeight, bool randomHalls, double maxDist=32767){ //returns position of surface and number of steps

    Vec3 raypos=pos.copy();
    double num=0;

    for(int i=0;i<=maxSteps;i++){
        vel=vel.norm();

        double d=dist(raypos,vel, roomSize, wallSize, roomHeight, randomHalls);
        num+=d;

        if(d>maxDist){
            return VecAndNum(raypos,32767);
        }
        if(d>minDist){

            vel=vel.mul(d);

            raypos=raypos.add(vel);
        }
        else{
            return VecAndNum(raypos,num);
        }
    }
    return VecAndNum(raypos,32767);
}

Vec3 getPixel(int x,int y,Vec3 pos,Dir facing,Vec3 texture[PICTURE_RES_X][PICTURE_RES_Y],Vec3 centerPos, double roomSize, double wallSize, double roomHeight, double FOV, bool fisheye, int useTexture, bool textureFixed, bool randomHalls){
    Vec3 vel=vecFromPixel(x,y,facing, FOV, fisheye);

    return colorFromPos(march(pos,vel,RDIST,MINMARCH, roomSize, wallSize, roomHeight, randomHalls),texture,centerPos, useTexture, textureFixed);
}

void distort(Vec3 currentPic[PICTURE_RES_X][PICTURE_RES_Y], Vec3 centerPos, Vec3 pos,Dir facing, Dir endFacing, double roomSize, double wallSize, double roomHeight, int useTexture, bool textureFixed, bool randomHalls){
    Vec3 currentPic1[PICTURE_RES_X][PICTURE_RES_Y];
    for(int i=0;i<PICTURE_RES_X;i++){
        for(int j=0;j<PICTURE_RES_Y;j++){
            currentPic1[i][j]=colorFromPos(march(pos,facing.compose(Dir(i*2*PI/PICTURE_RES_X-endFacing.th,j*PI/PICTURE_RES_Y-PI/2)),RDIST,MINMARCH, roomSize, wallSize, roomHeight, randomHalls),currentPic,centerPos, useTexture, textureFixed);
            //getPixel(int x,int y,Vec3 pos,Dir facing,Vec3 texture[PICTURE_RES][PICTURE_RES])
        }
    }

    for(int i=0;i<PICTURE_RES_X;i++){
        for(int j=0;j<PICTURE_RES_Y;j++){
            currentPic[i][j]=currentPic1[i][j].copy();
        }
    }
}

int main(void) {
    //std::cout<<mymod(-24,25);
    //BMP* bmpTest= BMP_Create(100,100,16);

    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window *window;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    
    //variables (some unused)
    int a; //a,b,delta for timing
    int b=SDL_GetTicks();
    double delta = 0;

    double temp;
    int tempX;
    int tempY;
    Vec3 tempPos;

    SDL_Rect pixel;
    pixel.x=0;
    pixel.y=0;
    pixel.h=PIXSIZ;
    pixel.w=PIXSIZ;
    double speed=.5;
    double roomSize=25;
    double wallSize=16;
    double roomHeight=10;
    double FOV=90;
    bool fisheye=false;
    bool invertedMouse=false;
    int useTexture=0;   //it would be better form to make an enum class for this
    bool textureFixed=false;
    bool collisions=true; //always true; collision detection can be turned off by setting collisions=false, but rendering inside of solid objects may cause flashing imagery.
    bool randomHalls=true;

    Vec3 currentPic[PICTURE_RES_X][PICTURE_RES_Y];
    for(int i=0;i<PICTURE_RES_X;i++){
        for(int j=0;j<PICTURE_RES_Y;j++){
            currentPic[i][j]=Vec3(rand()%255,rand()%255,rand()%255);
        }
    }
    Vec3 centerPos(0,0,0);
    Vec3 pos(0,0,0);
    Vec3 vel(0,0,0);
    Dir facing(0,0); 
    Dir centerFacing(0,0);

    bool open=true;
    int x,y;
    int mouseX,mouseY;

    bool move_fwd=false;
    bool move_bk=false;
    bool move_lft=false;
    bool move_rght=false;
    Vec3 pix(0,0,0);

    SDL_ShowCursor(SDL_DISABLE);

    while(open) {
        a = SDL_GetTicks();
        delta = a - b;

        if (delta > 1000/MAX_FRAMERATE)
        {
            //std::cout << "fps: " << 1000 / delta << std::endl;

            b = a;    

            vel=Vec3(0,0,0);

            if(move_fwd){
                vel=vel.add(Dir(facing.th,0).fwd().mul(speed));
            }       
            if(move_bk){
                vel=vel.add(Dir(facing.th+PI,0).fwd().mul(speed));
            }        
            if(move_lft){
                vel=vel.add(Dir(facing.th,0).side().mul(speed));
            }        
            if(move_rght){
                vel=vel.add(Dir(facing.th+PI,0).side().mul(speed));
            }

            pos=pos.add(vel.X());
            if(collisions and dist(pos, roomSize, wallSize, roomHeight, randomHalls)<=MINMARCH){
                pos=pos.add(vel.X().neg());
            }

            pos=pos.add(vel.Y());
            if(collisions and dist(pos, roomSize, wallSize, roomHeight, randomHalls)<=MINMARCH){
                pos=pos.add(vel.Y().neg());
            }

            /*if(collisions and dist(pos, roomSize, wallSize, roomHeight, randomHalls)<=MINMARCH){
                pos=pos.add(collision(pos, roomSize, wallSize, randomHalls));
            }*/

            //SDL_WaitEvent(&event);
            while(SDL_PollEvent(&event)!=0){
                switch(event.type){
                    case SDL_QUIT:
                        open=false;
                      break;
                    case SDL_KEYDOWN:
                        switch(event.key.keysym.sym){
                            case SDLK_ESCAPE:
                                open=false;
                                break;
                            case SDLK_LSHIFT:
                                speed*=2;
                                break;
                            case SDLK_w:
                                move_fwd=true;
                                break;
                            case SDLK_s:
                                move_bk=true;
                                break;
                            case SDLK_a:
                                move_lft=true;
                                break;                        
                            case SDLK_d:
                                move_rght=true;
                                break;
                            case SDLK_q:

                                temp=0;//rand()%2==0?0:PI/4;//2*PI*(rand()%100)/100.;   //set to something other than zero to change the direction you face after distorting

                                distort(currentPic,centerPos,pos,facing,Dir(temp,0), roomSize, wallSize, roomHeight, useTexture, textureFixed, randomHalls);

                                pos=Vec3(0,0,0);

                                facing=Dir(temp,0);
                                textureFixed=true;
                                break;
                            case SDLK_r:
                                    for(int i=0;i<PICTURE_RES_X;i++){
                                        for(int j=0;j<PICTURE_RES_Y;j++){
                                            currentPic[i][j]=Vec3(rand()%255,rand()%255,rand()%255);
                                        }
                                    }

                                    pos=Vec3(0,0,0);
                                    centerFacing=Dir(0,0);
                                    textureFixed=false;

                                    break;
                            case SDLK_i:
                                FOV=90;
                                break;
                            case SDLK_o:
                                FOV+=10;
                                break;
                            case SDLK_p:
                                FOV-=10;
                                break;
                            case SDLK_f:
                                fisheye=!fisheye;
                                break;
                            case SDLK_b:
                                roomSize++;
                                break;
                            case SDLK_h:
                                roomSize--;
                                break;
                            case SDLK_n:
                                roomHeight++;
                                break;
                            case SDLK_j:
                                roomHeight--;
                                break;
                            case SDLK_m:
                                wallSize++;
                                break;
                            case SDLK_k:
                                wallSize--;
                                break;
                            case SDLK_COMMA:
                                roomSize=25;
                                roomHeight=10;
                                wallSize=16;
                            case SDLK_l:
                                invertedMouse=!invertedMouse;
                                break;
                            case SDLK_t:
                                if(!textureFixed){
                                    useTexture=++useTexture%3;
                                }
                                break;
                            case SDLK_e:
                                randomHalls=!randomHalls;
                                break;
                        }
                        break;
                    case SDL_KEYUP:
                        switch(event.key.keysym.sym){
                            case SDLK_LSHIFT:
                                speed/=2;
                                break;
                            case SDLK_w:
                                move_fwd=false;
                                break;
                            case SDLK_s:
                                move_bk=false;
                                break;
                            case SDLK_a:
                                move_lft=false;
                                break;                        
                            case SDLK_d:
                                move_rght=false;
                                break;
                        }
                        break;
                }
            }

            //get mouse
            SDL_GetMouseState(&mouseX,&mouseY);

            //set camera direction
            facing.th+=(invertedMouse?1:-1)*2*PI*((double)mouseX/WINDOW_WIDTH-.5);
            facing.a+=(invertedMouse?1:-1)*PI*((double)mouseY/WINDOW_HEIGHT-.5);

            //bound up/down camera direction
            facing.a=fmax(fmin(facing.a,PI/2),-PI/2);

            SDL_WarpMouseInWindow(window,WINDOW_WIDTH/2,WINDOW_HEIGHT/2);

            for (x = 0; x < WINDOW_WIDTH; x+=PIXSIZ){for (y = 0; y < WINDOW_HEIGHT; y+=PIXSIZ){
                //get pixel
                pix=getPixel(x,y,pos,facing,currentPic,centerPos, roomSize, wallSize, roomHeight, FOV, fisheye, useTexture, textureFixed, randomHalls);

                //set pixel
                SDL_SetRenderDrawColor(renderer, pix.x, pix.y, pix.z, 255);
                pixel.x=x;
                pixel.y=y;
                SDL_RenderFillRect(renderer, &pixel);

            }}
            SDL_RenderPresent(renderer);
        }
    }
    
    //clean up

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}