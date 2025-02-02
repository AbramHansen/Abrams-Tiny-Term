#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string>

#include "../include/ascii_font.h"

constexpr int screenWidth{128};
constexpr int screenHeight{64};
std::string windowTitle{"Abram's Tiny Term"};

bool init();
bool loadMedia();
void close();
void mainLoop();

SDL_Window* gWindow{nullptr};
SDL_Texture* gSplashImage{nullptr};
SDL_Renderer* gRenderer{nullptr};

AsciiFont gTomThumb("../media/fonts/tom-thumb.bdf");

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
        if(!SDL_CreateWindowAndRenderer(windowTitle.c_str(), screenWidth, screenHeight, 0, &gWindow, &gRenderer)){
            SDL_Log("Window could not be created! SDL error: %s\n", SDL_GetError());
            success = false;
        } else {
            gTomThumb.setRenderer(gRenderer);
        }
            
    }

    return success;
}

bool loadMedia(){
    bool success{ true };

    SDL_Surface* splashImageSurface;
    std::string splashImagePath = "../media/textures/tiny_term.bmp";
    if(splashImageSurface = SDL_LoadBMP(splashImagePath.c_str()); splashImageSurface == nullptr){
        SDL_Log("Unable to load image %s! SDL Error: %s\n", splashImagePath.c_str(), SDL_GetError());
        success = false;
    } else {
        if(gSplashImage = SDL_CreateTextureFromSurface(gRenderer, splashImageSurface); gSplashImage == nullptr){
            SDL_Log( "Unable to create texture from loaded pixels! SDL error: %s\n", SDL_GetError() );
            success = false;
        }

        SDL_DestroySurface(splashImageSurface);
    }

    if(gTomThumb.load()){
        SDL_Log("Font Width: %i\n", gTomThumb.getWidth());
        SDL_Log("Font Height: %i\n", gTomThumb.getHeight());
    } else {
        SDL_Log("Unable to load font!");
        success = false;
    }

    return success;
}

void close(){
    SDL_DestroyTexture(gSplashImage);
    gSplashImage = nullptr;

    SDL_DestroyWindow(gWindow);
    gWindow = nullptr;

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
        
        SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(gRenderer);
        //SDL_RenderTexture(gRenderer, gSplashImage, nullptr, nullptr);
        int x = 0;
        int y = 0;
        for(char c = ' '; c < 127; c++, x += gTomThumb.getWidth() + 1){
            if(x > 124){
                x = 0;
                y += gTomThumb.getHeight();
            }
            gTomThumb.render(x,y,c);
        }

        SDL_RenderPresent(gRenderer);
    }
}
