#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string>
#include <vector>

#include "../include/terminal.h"

bool init();
void mainLoop();
void close();

int windowWidth = 128;
int windowHeight = 64;
std::string windowTitle = "Abram's Tiny Term";

SDL_Window* window{nullptr};
SDL_Renderer* renderer{nullptr};
Terminal* term{nullptr};

int main(int argc, char* args[]){
    SDL_Log("Starting Abram's Tiny Term\n");
    
    if(!init()){
        SDL_Log("init failure!\n");
        return 1;
    }

    mainLoop();

    close();

    return 0;
}

bool init(){
    if(!SDL_Init(SDL_INIT_VIDEO)){
        SDL_Log("SDL could not initialize! SDL error: %s\n", SDL_GetError());
        return false;
    }

    if(!SDL_CreateWindowAndRenderer(windowTitle.c_str(), windowWidth, windowHeight, 0, &window, &renderer)){
        SDL_Log("Window or renderer could not be created! SDL error: %s\n", SDL_GetError());
        return false;
    }

    term = new Terminal(renderer, windowWidth, windowHeight);
    if(!term->init()){
        SDL_Log("Failed to initialize terminal!\n");
        return false;
    }

    return true;
}

void close(){
    SDL_DestroyRenderer(renderer);
    renderer = nullptr;
    SDL_DestroyWindow(window);
    window = nullptr;

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
        
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);
        
        term->render(0,0);

        SDL_RenderPresent(renderer);
    }
}
