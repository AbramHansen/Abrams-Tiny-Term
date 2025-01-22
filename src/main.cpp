#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string>

constexpr int screenWidth{128};
constexpr int screenHeight{64};
std::string windowTitle{"Tiny Term"};

bool init();
bool loadMedia();
void close();
void mainLoop();

SDL_Window* gWindow{nullptr};
SDL_Surface* gScreenSurface{nullptr};
SDL_Surface* gSplashImage{nullptr};


int main(int argc, char* args[]){
    int exitCode{0};

    SDL_Log("Starting %s...\n", windowTitle.c_str());
    
    if(!init()){
        SDL_Log("Init failure!\n");
        exitCode = 1;
    } else {
        if(!loadMedia()){
            SDL_Log("Load Media failure!\n");
            exitCode=2;
        } else {
            mainLoop(); 
        }
    }

    close();

    return exitCode;
}

bool init(){
    bool success{ true };

    if(!SDL_Init(SDL_INIT_VIDEO)){
        SDL_Log("SDL could not initialize! SDL error: %s\n", SDL_GetError());
        success = false;
    } else {
        //create window
        if(gWindow = SDL_CreateWindow(windowTitle.c_str(), screenWidth, screenHeight, 0); gWindow == nullptr){
            SDL_Log("Window could not be created! SDL error: %s\n", SDL_GetError());
            success = false;
        } else {
            gScreenSurface = SDL_GetWindowSurface(gWindow);
        }
    }

    return success;
}

bool loadMedia(){
    bool success{ true };
    
    std::string splashImagePath = "../media/textures/tiny_term.bmp";
    if(gSplashImage = SDL_LoadBMP(splashImagePath.c_str()); gSplashImage == nullptr){
        SDL_Log("Unable to load image %s! SDL Error: %s\n", splashImagePath.c_str(), SDL_GetError());
        success = false;
    }

    return success;
}

void close(){
    SDL_DestroySurface(gSplashImage);
    gSplashImage = nullptr;

    SDL_DestroyWindow(gWindow);
    gWindow = nullptr;
    gScreenSurface = nullptr;

    SDL_Quit();
}

void mainLoop(){
    bool quit{false};

    SDL_Event e;
    SDL_zero(e);

    while(!quit){
        //get event data
        while(SDL_PollEvent(&e)){
            if(e.type == SDL_EVENT_QUIT)
                quit = true;
        }
        
        SDL_FillSurfaceRect(gScreenSurface, nullptr, SDL_MapSurfaceRGB(gScreenSurface, 0xFF, 0xFF, 0xFF));

        SDL_BlitSurface(gSplashImage, nullptr, gScreenSurface, nullptr);

        SDL_UpdateWindowSurface(gWindow);
    }
}
