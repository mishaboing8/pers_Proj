#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_timer.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

//--------------GUI CHARACTERISTICS--------------------------
int TEX_X_COUNT;
int TEX_Y_COUNT;

int const WINDOW_WIDTH = 640;
int WINDOW_HEIGHT;

int const maxLVL = 30;
int WIDTH_RELATIVETY;
int HEIGHT_RELATIVETY;

int const resourcesCount = 15;

char const *resources[resourcesCount] = {
                            "resources/background.png", 
                            "resources/box.png", "resources/wall.png","resources/target.png",
                            "resources/builder.png", "resources/win.png", "resources/icon.png", 
                            "resources/coolBoxOn.png","resources/coolBoxOff.png",
                            "resources/coolBuilder.png", "resources/coolGoogles.png", "resources/coolWall.png",
                            "resources/coolWallUp.png", "resources/coolTarget.png",
                            "resources/DIY.png"
                            };

int const imgCount = 10;

char const *numbersPaths[imgCount] = {
                    "resources/numbers/one.png", "resources/numbers/two.png", 
                    "resources/numbers/three.png", "resources/numbers/four.png", 
                    "resources/numbers/five.png", "resources/numbers/six.png", 
                    "resources/numbers/seven.png", "resources/numbers/eight.png", 
                    "resources/numbers/nine.png", "resources/numbers/zero.png"};


SDL_Texture *resTex[resourcesCount];
SDL_Texture *numText[imgCount];
SDL_Texture *allNumText[maxLVL];

SDL_Surface *icon = NULL;

const char *window_title = "SOKOBAN";
Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
                         


//-----------------SOKOBAN LOGIC VALUES----------------------

#include <stdbool.h>
#include <stdlib.h>

#define MAX_WIDTH 30//max witdth or height of reading maps
#define MAX_HEIGHT 30

static char field[MAX_HEIGHT][MAX_WIDTH] = {{'0'}};//field charactereistics (at first empty)
int height = 0, width = 0;//starting width or height, and level
int currLVL = 0;

static FILE* input;
static char FILE_NAME[] = "maps.txt";//file with created maps

int targetXPos[MAX_HEIGHT*MAX_WIDTH] = {0}, targetYPos[MAX_HEIGHT*MAX_WIDTH] = {0};//all targets and their positions
int targetCount = 0;//target or box count (boxCount == targetCount)
int wallsCount = 0;

int xPos = 0, yPos = 0;//main character position

/*
empty space = '0' (zero)
wall = '-' '|' '/' '\'
main hero = 'I'
box = 'B'
box target = 'X'
(just in file) box on target = '*', printed will be like 'B'
*/


int initText(SDL_Renderer *rend);
int destroyText();


int loadField(int lvl);//finds and loads level from file
int loadLevel();//same as loadField, but with error catching loop
void printField(SDL_Rect* xBuilder, SDL_Rect xWalls[wallsCount], SDL_Rect xBoxes[targetCount], SDL_Rect xTargets[targetCount]);
void findHero();//finds main character on map(if there exists more than one character, main will be the most uppier(up) and leftier(left))

void stepLeft();
void stepRight();
void stepUp();
void stepDown();

int readyTargetCount();//returns all targets count, that are with boxes (box on target)
bool isOnTarget(int x, int y);//returns true if object on this coord is on target

int play();
int makeLevel();
int choosingSokobalLVL();

int main(int argc, char* argv[])
{
    int choose = 0;
    while (true) {
        xPos = 0, yPos = 0;
        targetCount = 0;
        if(loadLevel() == -1) return 0;//load level heigt and width of field
        if(currLVL == 0) makeLevel();
        else play();
    }

    destroyText();
    
    return 0;
}
int play(){
    TEX_X_COUNT = width;
    TEX_Y_COUNT = height;
    WINDOW_HEIGHT = WINDOW_WIDTH * TEX_Y_COUNT/TEX_X_COUNT;//window height = width * field sides difference(16:9)
    WIDTH_RELATIVETY = WINDOW_WIDTH/TEX_X_COUNT;
    HEIGHT_RELATIVETY = WINDOW_HEIGHT/TEX_Y_COUNT;


    //Creates pointers to various game elements
    //Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;

    SDL_Window  *window = NULL;
    SDL_Renderer*rend = NULL;

    SDL_Texture *targetTexture = NULL;
    SDL_Texture *coolTargetTexture = NULL;
    // SDL_Surface *resSurf[resourcesCount];

    int request_quit = 0;

    //Initializes SDL
    SDL_Init(SDL_INIT_EVERYTHING);

    //Prints error message if SDL fails to initalize
    if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());

        //Quits SDL
        SDL_Quit();
        return 1;
    }
    /*
    Creates centered window with the window title pointer, width, height, and
    no flags
    */
    window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED,
                                              SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    //Prints error message if SDL failed to create window1
    if(!window)
    {
        printf("Failed to initalize window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    //Initializes renderer
    rend = SDL_CreateRenderer(window, -1, render_flags);
    if(!rend){
        printf("Failed to initialize renderer: %s\n", SDL_GetError());

        //Destroys window
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    if(initText(rend) == -1){
        SDL_DestroyRenderer(rend);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

    Mix_Music *backAudio = Mix_LoadMUS("resources/audio/dawning.mp3");
    if(!backAudio) {
        printf("Failed to initialize audio: %s\n", SDL_GetError());

        //Destroys window
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }


    //Texturizes loaded images

    SDL_SetWindowIcon(window, icon);

    targetTexture = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WINDOW_WIDTH, WINDOW_HEIGHT);
    coolTargetTexture =SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Structs to hold coordinate positions
    SDL_Rect xBoxes[targetCount];
    SDL_Rect xTargets[targetCount];//all arrays are filled with {0,0,0,0} rect (default value)
    SDL_Rect xWalls[wallsCount];

    SDL_Rect xBackground = {0, 0, 0, 0};
    SDL_Rect xBuilder = {0, 0, 0, 0};
    SDL_Rect xWin = {0, 0, 0, 0};
    SDL_Rect xGoog = {0, 0, 0, 0};


    //Gets scale and dimensions of textures
    SDL_QueryTexture(resTex[0], NULL, NULL, &xBackground.w, &xBackground.h);
    SDL_QueryTexture(resTex[4], NULL, NULL, &xBuilder.w, &xBuilder.h);
    SDL_QueryTexture(resTex[5], NULL, NULL, &xWin.w, &xWin.h);

    if(WINDOW_WIDTH/WINDOW_HEIGHT >= 3){//win texture is 48 : 16 (1:3)
        xWin.w = WINDOW_HEIGHT;
        xWin.h = WINDOW_HEIGHT/3;
    } else {
        xWin.w = WINDOW_WIDTH;
        xWin.h = WINDOW_WIDTH/3;
    }
        xWin.x = WINDOW_WIDTH/2 - xWin.w/2;
        xWin.y = WINDOW_HEIGHT/2 - xWin.h/2;

    for(int i = 0; i < wallsCount; i++)
         SDL_QueryTexture(resTex[2], NULL, NULL, &xWalls[i].w, &xWalls[i].h);
    
    for (int i = 0; i < targetCount; i++) {
        SDL_QueryTexture(resTex[3], NULL, NULL, &xTargets[i].w, &xTargets[i].h);
        SDL_QueryTexture(resTex[1], NULL, NULL, &xBoxes[i].w, &xBoxes[i].h);
    }
    
    //Reorder positions and dimensions of textures
    xBackground.w = WINDOW_WIDTH;
    xBackground.h = WINDOW_HEIGHT;

    for (int i = 0; i < targetCount; i++) {
        xBoxes[i].w = WIDTH_RELATIVETY;
        xBoxes[i].h = HEIGHT_RELATIVETY;
        xTargets[i].w = WIDTH_RELATIVETY;
        xTargets[i].h = HEIGHT_RELATIVETY;
    }

    xBuilder.w = WIDTH_RELATIVETY;
    xBuilder.h = HEIGHT_RELATIVETY;
    xGoog.w =  xBuilder.w;
    xGoog.h = xBuilder.h;
    xGoog.x = xBuilder.x;
    xGoog.y = -xGoog.h;//out of the screen


    for(int i = 0; i < wallsCount; i++){
        xWalls[i].h = HEIGHT_RELATIVETY;
        xWalls[i].w = WIDTH_RELATIVETY;
    }
    //locate all abjects, and combine targets with normal background
    printField(&xBuilder, xWalls, xBoxes, xTargets);
    SDL_RenderClear(rend);
    SDL_SetRenderTarget(rend, targetTexture);
    
    SDL_RenderCopy(rend, resTex[0], NULL, &xBackground);

    for(int i = 0; i < targetCount; i++)
        SDL_RenderCopy(rend, resTex[3], NULL, &xTargets[i]);

    SDL_SetRenderTarget(rend, NULL);
    SDL_RenderClear(rend);

    //combine cool staff
    SDL_SetRenderTarget(rend, coolTargetTexture);
    
    SDL_RenderCopy(rend, resTex[0], NULL, &xBackground);                            
    
    for(int i = 0; i < targetCount; i++)
        SDL_RenderCopy(rend, resTex[13], NULL, &xTargets[i]);

    SDL_SetRenderTarget(rend, NULL);
    SDL_RenderClear(rend);


    int wonFrameRate = 0;
    bool coolMode = false, startCoolMode = false;
    int coolBoxOn[targetCount];
    int coolWallOn[wallsCount];
    int frameCount = 0;
    srand(time(NULL));
    for(int i = 0; i < targetCount; i++)
        coolBoxOn[i] = rand() % 2 == 0 ? 0 : 1;

    for(int i = 0; i < wallsCount; i++)
        coolWallOn[i] = rand() % 2 == 0 ? 4 : 5;  
    
    //Initializes game loop
    while(!request_quit)
    {
        printField(&xBuilder, xWalls, xBoxes, xTargets);
        xGoog.x = xBuilder.x;

        if(startCoolMode && !coolMode){
            xGoog.y += xGoog.h/30;
        }

        if(xGoog.y >= xBuilder.y && !coolMode){
            Mix_PlayMusic(backAudio, -1);
            coolMode = true;
        }
        //Processes events
        SDL_Event event;

        
        while(SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                request_quit = 1;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.scancode)
                {
                case SDL_SCANCODE_ESCAPE:
                    request_quit = 1;
                    break;
                case SDL_SCANCODE_R:
                    loadField(currLVL);
                    break;
                case SDL_SCANCODE_W:
                case SDL_SCANCODE_UP:
                    stepUp();
                    break;
                case SDL_SCANCODE_A:
                case SDL_SCANCODE_LEFT:
                    stepLeft();
                    break;
                case SDL_SCANCODE_S:
                case SDL_SCANCODE_DOWN:
                    stepDown();
                    break;
                case SDL_SCANCODE_D:
                case SDL_SCANCODE_RIGHT:
                    stepRight();
                    break;
                case SDL_SCANCODE_B:
                    startCoolMode = true;

                    break;
                }
                break;
            }
        }

        //Clears screen for renderer
        SDL_RenderClear(rend);

        //Copies textures to buffer and presents them to screen
        SDL_RenderCopy(rend, !coolMode ? targetTexture : coolTargetTexture, NULL, NULL);

        if(frameCount == 25) {
            for(int i = 0; i < targetCount; i++)
                coolBoxOn[i] = rand() % 2 == 0 ? 7 : 8;

            for(int i = 0; i < wallsCount; i++)
                coolWallOn[i] = rand() % 2 == 0 ? 11 : 12;    

            frameCount = 0;
        }
        for(int i = 0; i < targetCount; i++)
            SDL_RenderCopy(rend, !coolMode ? resTex[1] : resTex[coolBoxOn[i]], NULL, &xBoxes[i]);

        for(int i = 0; i < wallsCount; i++)
            SDL_RenderCopy(rend, !coolMode ? resTex[2] : resTex[coolWallOn[i]], NULL, &xWalls[i]);

        SDL_RenderCopy(rend, !coolMode ? resTex[4] : resTex[9], NULL, &xBuilder);

        
        if(startCoolMode && !coolMode){
            SDL_RenderCopy(rend, resTex[10], NULL, &xGoog);
        }

        if(readyTargetCount() == targetCount || wonFrameRate > 0){
            SDL_RenderCopy(rend, resTex[5], NULL, &xWin);
            wonFrameRate++;
        }

        SDL_RenderPresent(rend);
        //Waits 1/60th of a second for a 60 fps lock
        SDL_Delay(1000 / 60);
        frameCount++;

        if(wonFrameRate == 100) break;
    }
    Mix_CloseAudio();
    Mix_FreeMusic(backAudio);


    destroyText();
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return request_quit == 1 ? -1 : 1;
}

//--------------------SOKOBAN MAIN LOGIC---------------------------

void printField(SDL_Rect* xBuilder, SDL_Rect xWalls[wallsCount], SDL_Rect xBoxes[targetCount], SDL_Rect xTargets[targetCount]){
    int bCount = 0, tCount = 0, wCount = 0;
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            switch (field[i][j])
            {
            case '*':
                xTargets[tCount].x = j * WIDTH_RELATIVETY;
                xTargets[tCount].y = i * HEIGHT_RELATIVETY;
                tCount++;
            case 'B':
                xBoxes[bCount].x = j * WIDTH_RELATIVETY;
                xBoxes[bCount].y = i * HEIGHT_RELATIVETY;
                bCount++;
                break;
            case 'I':
                xBuilder->x = j * WIDTH_RELATIVETY;
                xBuilder->y = i * HEIGHT_RELATIVETY;
                break;
            case 'X':
                xTargets[tCount].x = j * WIDTH_RELATIVETY;
                xTargets[tCount].y = i * HEIGHT_RELATIVETY;
                tCount++;
                break;
            case '\\':
            case '/':
            case '|':
            case '-':
                xWalls[wCount].x = j * WIDTH_RELATIVETY;
                xWalls[wCount].y = i * HEIGHT_RELATIVETY;
                wCount++;
                break;
            default:
                break;
            }
        }
    }
}

int loadLevel(){
    int lvl = choosingSokobalLVL();
    if(lvl == -1) return -1;
    if(lvl == 0) return 1;//DIY
    if(loadField(lvl) == -1) return -1;
    return 1;
}

int loadField(int level){
    char currChar;
    targetCount = 0;
    wallsCount = 0;

    input = fopen(FILE_NAME, "r");
    char line[256];
    int prevLVL = 10;//if there is infinity loop(if lvl not exist), it will roll with same lvl again and again
                        //so if previous lvl == current level -> return error code
    while(1){
        fscanf(input, "%d", &currLVL);
        fscanf(input, "%d", &width);
        fscanf(input, "%d", &height);
        if(currLVL == prevLVL) return -1;
        if (currLVL == level) {
            fscanf(input, "%c", &currChar);//skip new lines char 
            break;//arrived to neccesary level
        }
        else 
            for (int i = 0; i < height; i++)
                fscanf(input, "%s", line);//skip current level lines
            
           prevLVL = currLVL; 
    }


    for(int i = 0; i < height; i++){
        for(int j = 0; j < width+1; j++){//+1 -> inclusive new line char
            fscanf(input, "%c", &currChar);

            if(currChar == '\n') continue;//skip new line
            else {
                if(currChar == 'X' || currChar == '*'){
                    targetXPos[targetCount] = j;
                    targetYPos[targetCount] = i;
                    targetCount++;
                }
                if(currChar == '-' || currChar == '|' || currChar == '\\' || currChar == '/')
                    wallsCount++;
                
                
                field[i][j] = currChar == '*' ? 'B' : currChar;
            }
        }
    }

        findHero();
        fclose(input);
        return 1;
}

void findHero(){
    //hero char == I
    for(int i = 0; i < height; i++)
        for(int j = 0; j < width; j++)
            if(field[i][j] == 'I'){
                xPos = j;
                yPos = i;
                return;
            }
}

bool isWall(int x, int y){
    return field[y][x] == '-' || field[y][x] == '|' || field[y][x] == '\\' || field[y][x] == '/';
}

void allocateTargets(){//on the places where were boxes or main character could be targets. After every step it is neccesary to place targets back(all 0 -> X)
    for(int i = 0; i < targetCount; i++)
        if (field[targetYPos[i]][targetXPos[i]] == '0')
            field[targetYPos[i]][targetXPos[i]] = 'X';
}

void stepLeft(){
    if(xPos <= 0 || isWall(xPos - 1, yPos)) // wall or out of bounds
        return;

    if(field[yPos][xPos - 1] == 'B' && (isWall(xPos - 2, yPos) || field[yPos][xPos-2] == 'B' || xPos-2 < 0))//after box are wall/anoter box/out of bounds
        return;

    if(field[yPos][xPos - 1] == 'B')
        field[yPos][xPos - 2] = 'B';//move box
        
    field[yPos][xPos] = '0';
    xPos--;//move hero
    field[yPos][xPos] = 'I';

    allocateTargets();//set all targets, because on their positions can stay zero char
    return;
}

void stepRight(){
    if(xPos >= width - 1 || isWall(xPos + 1, yPos)) // wall or out of bounds
        return;

    if(field[yPos][xPos + 1] == 'B' && (isWall(xPos + 2, yPos) || field[yPos][xPos+2] == 'B' || xPos+2 > width - 1))//after box are wall/anoter box/out of bounds
        return;

    if(field[yPos][xPos + 1] == 'B')
        field[yPos][xPos + 2] = 'B';//move box
        
    field[yPos][xPos] = '0';
    xPos++;//move hero
    field[yPos][xPos] = 'I';

    allocateTargets();//set all targets, because on their positions can stay zero char
    return;
}

void stepUp(){
    if(yPos <= 0 || isWall(xPos, yPos-1)) // wall or out of bounds
        return;

    if(field[yPos-1][xPos] == 'B' && (isWall(xPos, yPos-2) || field[yPos-2][xPos] == 'B' || yPos-2 < 0))//after box are wall/anoter box/out of bounds
        return;

    if(field[yPos-1][xPos] == 'B')
        field[yPos-2][xPos] = 'B';//move box
        
    field[yPos][xPos] = '0';
    yPos--;//move hero
    field[yPos][xPos] = 'I';

    allocateTargets();//set all targets, because on their positions can stay zero char
    return;
}

void stepDown(){
    if(yPos >= height - 1 || isWall(xPos, yPos+1)) // wall or out of bounds
        return;

    if(field[yPos+1][xPos] == 'B' && (isWall(xPos, yPos+2) || field[yPos+2][xPos] == 'B' || yPos+2 > height - 1))//after box are wall/anoter box/out of bounds
        return;

    if(field[yPos+1][xPos] == 'B')
        field[yPos+2][xPos] = 'B';//move box
        
    field[yPos][xPos] = '0';
    yPos++;//move hero
    field[yPos][xPos] = 'I';

    allocateTargets();//set all targets, because on their positions can stay zero char
    return;
}

bool isOnTarget(int x, int y){
    for (int i = 0; i < targetCount; i++) 
        if(targetXPos[i] == x && targetYPos[i] == y) return true;
    
    return false;
}

int readyTargetCount(){
    int count = 0;
    for(int i = 0; i < height; i++)
        for(int j = 0; j < width; j++)
            if (field[i][j] == 'B' && isOnTarget(j, i))
                count++;

    return count;
}


//---------------------CHOOOSE SOKOBAN LELEVEL --------------------

int const CHOOSING_WINDOW_WIDTH = 720;
int const CHOOSING_WINDOW_HEIGHT = 720;
int texWidth = CHOOSING_WINDOW_WIDTH/10, texHeight = CHOOSING_WINDOW_HEIGHT/10;

int findLVLCount();
int combineNumTexture(SDL_Renderer *rend, int digit, SDL_Texture *targetTexture, int texWidth, int texHeight);


int choosingSokobalLVL(){
    int lvlCount = findLVLCount()+1;//because +DIY

    SDL_Window  *window = NULL;
    SDL_Renderer *rend = NULL;

    SDL_Rect xNum[imgCount+1];//+1 because +DIY

    SDL_Init(SDL_INIT_EVERYTHING);

    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,CHOOSING_WINDOW_WIDTH, CHOOSING_WINDOW_HEIGHT, 0);

    if(!window) {
        printf("Failed to initalize window: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    rend = SDL_CreateRenderer(window, -1, render_flags);
    if(!rend) {
        printf("Failed to initialize renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    if(initText(rend) == -1){
        SDL_DestroyRenderer(rend);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    SDL_SetWindowIcon(window, icon);

    int texWidth = CHOOSING_WINDOW_WIDTH/10, texHeight = CHOOSING_WINDOW_HEIGHT/10;
    
    SDL_Rect xBackground = {0,0,CHOOSING_WINDOW_WIDTH,CHOOSING_WINDOW_HEIGHT};
    SDL_Rect xTargeted = {0,0,0,0};


    xTargeted.w = texWidth*4/5;
    xTargeted.h = texHeight*4/5;

    int request_quit = 0;
    int currLVL = 0;
    int choosedLVL = -1;
    
    while(!request_quit)
    { 
        //Processes events
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                request_quit = 1;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.scancode)
                {
                case SDL_SCANCODE_ESCAPE:
                    request_quit = 1;
                    choosedLVL = -1;
                    break;
                case SDL_SCANCODE_A:
                case SDL_SCANCODE_LEFT:
                    if(--currLVL < 0) currLVL = lvlCount-1;                    
                    break;
                case SDL_SCANCODE_D:
                case SDL_SCANCODE_RIGHT:
                    if(++currLVL >= lvlCount) currLVL = 0; 
                    break;
                case SDL_SCANCODE_S:
                case SDL_SCANCODE_DOWN:
                    currLVL+=5;
                    if(currLVL >= lvlCount) currLVL = lvlCount-1;
                    break;
                case SDL_SCANCODE_W:
                case SDL_SCANCODE_UP:
                    currLVL-=5;
                    if (currLVL < 0) currLVL = 0;
                    break;
                case SDL_SCANCODE_RETURN://aka enter
                    request_quit = 1;
                    choosedLVL = currLVL;
                }
            default:
            break;
            }
        }
    SDL_RenderClear(rend);
    SDL_RenderCopy(rend, resTex[0], NULL, &xBackground);

    for(int i = 0; i < lvlCount; i++){
        xNum[i].w = texWidth;
        xNum[i].h = texHeight;
        xNum[i].x = i == 0 ? texWidth/2 : i%5 == 0 ? xNum[i-5].x : xNum[i-1].x+texWidth*2;
        xNum[i].y = i == 0 ? texHeight/2 : i%5 == 0 ? xNum[i-5].y + texHeight*3/2: xNum[i-1].y;
    }
    
    for(int i = 0; i < lvlCount; i++){
        if(i == currLVL) {
            SDL_RenderCopy(rend, resTex[3], NULL, &xNum[i]);
            xTargeted.x = xNum[i].x + xNum[i].w/10;
            xTargeted.y = xNum[i].y + xNum[i].h/10;
            SDL_RenderCopy(rend, i == 0 ? resTex[14] : allNumText[i-1], NULL, &xTargeted);

        } else {
            SDL_RenderCopy(rend, i == 0 ? resTex[14] : allNumText[i-1], NULL, &xNum[i]);
        }
    }
    SDL_RenderPresent(rend);
    }

    destroyText();
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return choosedLVL;
}

int findLVLCount(){
    int height, currLVL = 0, prevLVL = 10;

    input = fopen(FILE_NAME, "r");
    char line[256];
    //if there is infinity loop(if lvl not exist), it will roll with same lvl again and again
    //so if previous lvl == current level -> return error code
    while(1){
        fscanf(input, "%d", &currLVL);
        fscanf(input, "%d", &height);
        fscanf(input, "%d", &height);
        if(currLVL == prevLVL) return currLVL;

        for (int i = 0; i < height; i++)
            fscanf(input, "%s", line);//skip current level lines
            
        prevLVL = currLVL; 
    }
    fclose(input);
}

int combineNumTexture(SDL_Renderer *rend, int digit, SDL_Texture *targetTexture, int texWidth, int texHeight){
    char str[10];
    sprintf(str, "%d", digit);
    int strLen = 0;
    for (int i = 0; i < 10; i++){
        if(str[i] == 0) break;
        else strLen++;
    }


    SDL_RenderClear(rend);

    SDL_SetRenderTarget(rend, targetTexture);

    SDL_Rect xTar = {0,0,texWidth, texHeight};
    SDL_Rect xPart = {0, texHeight/2 - texHeight/(2*strLen), texWidth/strLen, texHeight/(strLen)};
    SDL_RenderCopy(rend, resTex[1], NULL, &xTar);
    int texPos = 0;//0 == 48 , 9 == 57
    for (int i = 0; i < strLen; i++) {
        texPos = str[i]-48 == 0 ? 10 : str[i]-48;//1234567890 0 -> 10
        SDL_RenderCopy(rend, numText[texPos-1], NULL, &xPart);
        xPart.x += xPart.w;
    }
    SDL_SetRenderTarget(rend, NULL);
    SDL_RenderClear(rend);

    return 0;
}

//------------------DO YOUR OWN LEVEL----------------------------

bool hasRect(int x, int y, int length, SDL_Rect objects[length]);

int makeLevel(){
    SDL_Window  *window = NULL;
    SDL_Renderer *rend = NULL;

    SDL_Rect xObj = {0,0,0,0};
    SDL_Rect xObjects[MAX_HEIGHT*MAX_WIDTH] = {xObj};
    int objText[MAX_HEIGHT*MAX_WIDTH] = {0};
    char field[MAX_HEIGHT][MAX_WIDTH];

    for(int x = 0; x < MAX_HEIGHT; x++) for(int y = 0; y < MAX_WIDTH; y++) field[y][x] = '0';

    SDL_Init(SDL_INIT_EVERYTHING);

    if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED, CHOOSING_WINDOW_WIDTH, CHOOSING_WINDOW_HEIGHT, 0);

    if(!window)
    {
        printf("Failed to initalize window: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    rend = SDL_CreateRenderer(window, -1, render_flags);
    if(!rend)
    {
        printf("Failed to initialize renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    if(initText(rend) == -1){
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_Rect xBackground = {0,0, CHOOSING_WINDOW_WIDTH, CHOOSING_WINDOW_HEIGHT};
    int request_quit = 0;
    SDL_Rect movRect = {texWidth,texHeight,texWidth,texHeight};
    int frameCount = 0, currTex = 0, fieldLength = 0, clickCount = 0;

    bool grow = false;
    bool choosingBorders = true;

    bool setBuilder = false; 
    int placedBoxCount = 0, placedTargetCount = 0;

    int mouseX = 0, mouseY = 0;

    while(!request_quit){

        if(frameCount == 65){
            grow = !grow;
            frameCount = 0;
        }
        if(frameCount%2 == 0 && !choosingBorders){
            movRect.w += grow ? 1 : -1;
            movRect.h += grow ? 1 : -1;
            movRect.x += frameCount%4 == 0 ? 0 : grow ? -1 : 1;
            movRect.y += frameCount%4 == 0 ? 0 : grow ? -1 : 1;
        }

        if(!choosingBorders)frameCount++;

        //Processes events
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                request_quit = 1;
                break;
            case SDL_MOUSEBUTTONDOWN:
                if(choosingBorders) break;
                
                int x = movRect.x - movRect.x%texWidth, y = movRect.y - movRect.y%texHeight;

                if(hasRect(x, y, clickCount, xObjects)) break;

                xObjects[clickCount] = movRect;
                xObjects[clickCount].x = x;
                xObjects[clickCount].y = y;
                xObjects[clickCount].w = texWidth;
                xObjects[clickCount].h = texHeight;
                objText[clickCount] = currTex;
                char c = '0';
                switch (currTex){
                case 1:
                    c = 'B';
                    placedBoxCount++;
                    break;
                case 2:
                    c = '|';
                    break;
                case 3:
                    c = 'X';
                    placedTargetCount++;
                    break;
                case 4:
                    if(setBuilder) break;
                    
                    c = 'I';
                    setBuilder = true;
                    break;
                default:
                    c = '0';
                    break;
                }
                field[xObjects[clickCount].y/texHeight][xObjects[clickCount].x / texWidth] = c;
                clickCount++;
                break;
            case SDL_MOUSEMOTION:
                if(choosingBorders) break;
                mouseX = event.motion.x;
                mouseY = event.motion.y;

                movRect.x += mouseX <= movRect.x ? -texWidth : mouseX >= movRect.x && mouseX <= movRect.x + texWidth ? 0 : texWidth;
                movRect.y += mouseY <= movRect.y ? -texHeight : mouseY >= movRect.y && mouseY <= movRect.y + texHeight ? 0 : texHeight;
                movRect.x += movRect.x < 0 ? texWidth : movRect.x > CHOOSING_WINDOW_WIDTH ? -texWidth : 0;
                movRect.y += movRect.y < 0 ? texHeight : movRect.y > CHOOSING_WINDOW_HEIGHT ? -texHeight : 0;

                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.scancode)
                {
                case SDL_SCANCODE_ESCAPE:
                    request_quit = 1;
                    break;
                case SDL_SCANCODE_A:
                case SDL_SCANCODE_LEFT:
                    if(!choosingBorders){
                        if(currTex > 1) currTex--;
                    } else {
                        if(currTex >= 1) {
                            currTex--;
                            for(int i = 0; i < currTex; i++){
                                xObjects[i].w = CHOOSING_WINDOW_WIDTH/currTex;
                                xObjects[i].h = CHOOSING_WINDOW_HEIGHT/currTex;
                                xObjects[i].x = i == 0 ? 0 : xObjects[i-1].x + xObjects[i].w;
                                xObjects[i].y = i == 0 ? 0 : xObjects[i-1].y + xObjects[i].h;
                            }
                        }
                    }
                    break;
                case SDL_SCANCODE_D:
                case SDL_SCANCODE_RIGHT:
                    if(!choosingBorders){
                        if(currTex < 4)currTex++;
                    } else {
                        if(currTex < maxLVL){
                            currTex++;
                            for(int i = 0; i < currTex; i++){
                                xObjects[i].w = CHOOSING_WINDOW_WIDTH/currTex;
                                xObjects[i].h = CHOOSING_WINDOW_HEIGHT/currTex;
                                xObjects[i].x = i == 0 ? 0 : xObjects[i-1].x + xObjects[i].w;
                                xObjects[i].y = i == 0 ? 0 : xObjects[i-1].y + xObjects[i].h;
                            }
                        }
                    }
                    break;
                case SDL_SCANCODE_RETURN://aka enter
                    if(!choosingBorders) {
                        if(!setBuilder || placedTargetCount != placedBoxCount) break;

                        request_quit = 2;
                    }
                    else {
                        if(currTex == 0) break;

                        fieldLength = currTex;
                        currTex = 1;
                        choosingBorders = false;
                        texWidth = xObjects[0].w;
                        texHeight = xObjects[0].h;
                        movRect.h = texHeight;
                        movRect.w = texWidth;
                        movRect.x = 0;
                        movRect.y = 0;   
                    }
                    break;
                case SDL_SCANCODE_P:
                    printf("\n");
                    for(int y = 0; y < fieldLength; y++){
                        for(int x = 0; x < fieldLength; x++){
                            printf("%c ", field[y][x]);
                        }
                        printf("\n"); 
                    }
                    break;
                case SDL_SCANCODE_Z:
                    if(clickCount <= 0) break;
                    
                    clickCount--;
                    int x = xObjects[clickCount].x/texWidth, y = xObjects[clickCount].y/texHeight;
                    field[y][x] = '0';
                    break;
                case SDL_SCANCODE_R:
                    clickCount = 0;
                    for(int y = 0; y < fieldLength; y++) for(int x = 0; x < fieldLength; x++) field[y][x] = '0';
                    break;
                default:
                    break;  
                }

            default:
                break;
            }
        }

        SDL_RenderClear(rend);
        if(choosingBorders){
            if(currTex < 5){
                SDL_RenderCopy(rend, resTex[1], NULL, &xBackground);
                for (int i = 0; i < currTex; i++) {
                    SDL_RenderCopy(rend, resTex[2], NULL, &xObjects[i]);
                }
                SDL_RenderCopy(rend, numText[currTex == 0 ? 9 : currTex-1], NULL, &xBackground);
            }
            else{
                SDL_RenderCopy(rend, allNumText[currTex-1], NULL, &xBackground);
                for (int i = 0; i < currTex; i++) {
                    SDL_RenderCopy(rend, resTex[2], NULL, &xObjects[i]);
                }
            }
        } else {

            SDL_RenderCopy(rend, resTex[0], NULL, &xBackground);
            for(int i = 0; i < clickCount; i++){
                SDL_RenderCopy(rend, resTex[objText[i]], NULL, &xObjects[i]);
            }

            SDL_RenderCopy(rend, resTex[currTex], NULL, &movRect);
        }
        SDL_RenderPresent(rend);

        SDL_Delay(1000 / 120);
    }

    destroyText();

    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(window);
    SDL_Quit();
    if(request_quit == 1) return 1;

    int lvlCount = findLVLCount();

    input = fopen(FILE_NAME, "a");

    fprintf(input, "\n%d %d %d\n", lvlCount+1, fieldLength, fieldLength);
    for(int y = 0; y < fieldLength; y++){
        for(int x = 0; x < fieldLength; x++){
            fprintf(input, "%c", field[y][x]);
        }
        fprintf(input, "\n");
    }

    fclose(input);
    return 2;
}

bool hasRect(int x, int y, int length, SDL_Rect objects[length]){
    for(int i = 0; i < length; i++)
        if(x == objects[i].x && y == objects[i].y) return true;
    return false;
}

//------------------INITIOLIZE AND DESTROY TEXTURES--------------

int initText(SDL_Renderer *rend){
    SDL_Surface *numImg[imgCount];
    SDL_Surface *resSurf[resourcesCount];

    bool error = false;

    for(int i = 0; i < imgCount; i++){
        numImg[i] = IMG_Load(numbersPaths[i]);
        if (!numImg[i]) {
            error = true;
            printf("Problem in: %s\n", numbersPaths[i]);
        }
    }

    for(int i = 0; i < resourcesCount; i++){
        resSurf[i] = IMG_Load(resources[i]);
        if (!resSurf[i]) {
            error = true;
            printf("Problem in: %s\n", resources[i]);
        }
    }
    icon = IMG_Load(resources[6]);
    if(!icon) error = true;

    if(error){
        printf("Failed to load images\n");
        return -1;
    }


    for(int i = 0; i < maxLVL; i++){
        allNumText[i] = SDL_CreateTexture(rend, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, texWidth, texHeight);
    }

    for(int i = 0; i < imgCount; i++){
        numText[i] = SDL_CreateTextureFromSurface(rend, numImg[i]);
        SDL_FreeSurface(numImg[i]);
        if (!numText[i]) error = true;
    }

    for(int i = 0; i < resourcesCount; i++){
        resTex[i] = SDL_CreateTextureFromSurface(rend, resSurf[i]);
        SDL_FreeSurface(resSurf[i]);
        if(!resTex[i]) error = true;
    }

    for(int i = 0; i < maxLVL; i++){
        if (!allNumText[i]) error = true;
    }

    if (error) {
        printf("Failed to texturize images\n");
        return -1;
    }

    for (int i = 1; i <= maxLVL; i++) 
        combineNumTexture(rend, i, allNumText[i-1], texWidth, texHeight);

    return 0;
}

int destroyText(){
    for(int i = 0; i < imgCount; i++)
        SDL_DestroyTexture(numText[i]);

    for(int i = 0; i < maxLVL; i++)
        SDL_DestroyTexture(allNumText[i]);

    for(int i = 0; i < resourcesCount; i++)
        SDL_DestroyTexture(resTex[i]);

    SDL_FreeSurface(icon);
    return 0;
}