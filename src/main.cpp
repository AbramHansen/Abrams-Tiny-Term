#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string>
#include <vector>

#include "../include/terminal.h"

bool init();
void mainLoop();
void close();
bool handleEvent(SDL_Event event);
char getAsciiCode(SDL_Keycode keycode);

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
    if(!term->init("bash")){
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

    SDL_Event event;
    SDL_zero(event);

    while(!quit){
        //get event data
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_EVENT_QUIT){
                quit = true;
            } else {
                if(!handleEvent(event))
                    SDL_Log("Error handling event: %i\n", event.type);
            }
        }
        
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);
        
        term->update();
        term->render(0,0);

        SDL_RenderPresent(renderer);
    }
}

char currentlyPressed = -1;
int timeSincePressedMS = 0;

bool handleEvent(SDL_Event event){
    switch (event.type){
        case SDL_EVENT_WINDOW_RESIZED:
            {
                int newWidth, newHeight;
                if(!SDL_GetWindowSize(window, &newWidth, &newHeight))
                    return false;
                term->updateDimensions(newWidth, newHeight);
                break;
            }
        case SDL_EVENT_KEY_DOWN:
            {
                currentlyPressed = -1;

                if(event.key.key <= 0xb1){
                    if(SDL_GetModState() & SDL_KMOD_ALT)
                        term->sendChar('\e');
                    term->sendChar(getAsciiCode(event.key.key));
                }else{
                    SDL_Log("GET ESCAPE SEQUENCE\n"); //TODO somehow ingnore modifiers
                }
            }
            break;
    }
    return true;
}

char getAsciiCode(SDL_Keycode keycode){
    char asciiCode = keycode;
    SDL_Keymod modifiers = SDL_GetModState();

    //handle CAPS LOCK
    if(modifiers & SDL_KMOD_CAPS && asciiCode >= 'a' && asciiCode <= 'z') asciiCode -= 32;
    
    //handle SHIFT
    if(modifiers & SDL_KMOD_SHIFT){
        if(asciiCode >= 'a' && asciiCode <= 'z') asciiCode -= 32;
        else if(asciiCode == '`') asciiCode = '~';
        else if(asciiCode == '1') asciiCode = '!';
        else if(asciiCode == '2') asciiCode = '@';
        else if(asciiCode == '3') asciiCode = '#';
        else if(asciiCode == '4') asciiCode = '$';
        else if(asciiCode == '5') asciiCode = '%';
        else if(asciiCode == '6') asciiCode = '^';
        else if(asciiCode == '7') asciiCode = '&';
        else if(asciiCode == '8') asciiCode = '*';
        else if(asciiCode == '9') asciiCode = '(';
        else if(asciiCode == '0') asciiCode = ')';
        else if(asciiCode == '-') asciiCode = '_';
        else if(asciiCode == '=') asciiCode = '+';
        else if(asciiCode == '[') asciiCode = '{';
        else if(asciiCode == ']') asciiCode = '}';
        else if(asciiCode == '\\') asciiCode = '|';
        else if(asciiCode == ';') asciiCode = ':';
        else if(asciiCode == '\'') asciiCode = '\"';
        else if(asciiCode == ',') asciiCode = '<';
        else if(asciiCode == '.') asciiCode = '>';
        else if(asciiCode == '/') asciiCode = '?';
    }

    //handle CTRL
    if(modifiers & SDL_KMOD_CTRL) asciiCode ^= 0b01100000;
        
    return asciiCode;
}

