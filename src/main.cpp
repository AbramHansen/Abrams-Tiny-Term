#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string>

#include "../include/terminal.h"

bool init();
void mainLoop();
void close();
bool handleEvent(SDL_Event event);
char getAsciiCode(SDL_Keycode keycode);
std::string getCodeSequence(SDL_Keycode keycode);
void sendAsciiCharacter(SDL_Keycode keycode);
void handleKeypadInput(SDL_Keycode keycode);

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

    if(!SDL_CreateWindowAndRenderer(windowTitle.c_str(), 0, 0, 0, &window, &renderer)){
        SDL_Log("Window or renderer could not be created! SDL error: %s\n", SDL_GetError());
        return false;
    }

    term = new Terminal(renderer);
    if(!term->init("bash")){
        SDL_Log("Failed to initialize terminal!\n");
        return false;
    }

    term->setPadding(0,2);

    SDL_SetWindowSize(window, term->getPixelWidth(), term->getPixelHeight());

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
                if(event.key.key <= SDLK_TILDE){
                    sendAsciiCharacter(event.key.key);
                }else if(event.key.key >= SDLK_KP_DIVIDE && event.key.key <= SDLK_KP_EQUALS){
                    handleKeypadInput(event.key.key);
                }else{
                    //handle other keybord keys (f1-f12, home, end, etc)
                    term->sendSequence(getCodeSequence(event.key.key));
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

std::string getCodeSequence(SDL_Keycode keycode){
    std::string sequence = "";
    
    char modifierCode = '0';
    SDL_Keymod modState = SDL_GetModState();
    bool ctrlPressed = modState & SDL_KMOD_CTRL;
    bool shiftPressed = modState & SDL_KMOD_SHIFT;
    bool altPressed = modState & SDL_KMOD_ALT;
    if(ctrlPressed && shiftPressed && altPressed) modifierCode = '8';
    else if(ctrlPressed && altPressed) modifierCode = '7';
    else if(ctrlPressed && shiftPressed) modifierCode = '6';
    else if(altPressed && shiftPressed) modifierCode = '4';
    else if(ctrlPressed) modifierCode = '5';
    else if(altPressed) modifierCode = '3';
    else if(shiftPressed) modifierCode = '2';
    
    //due to VT100 legacy we handle f1-f4, arrow keys, home, and end differently
    if((keycode >= SDLK_F1 && keycode <= SDLK_F4) || (keycode >= SDLK_HOME && keycode <= SDLK_UP)){
        if(modifierCode == '0'){
            if(keycode == SDLK_F1) sequence = "\eOP";
            else if(keycode == SDLK_F2) sequence = "\eOQ";
            else if(keycode == SDLK_F3) sequence = "\eOR";
            else if(keycode == SDLK_F4) sequence = "\eOS";
            else if(keycode == SDLK_HOME) sequence = "\e[H";
            else if(keycode == SDLK_END) sequence = "\e[F";
            else if(keycode == SDLK_UP) sequence = "\e[A";
            else if(keycode == SDLK_DOWN) sequence = "\e[B";
            else if(keycode == SDLK_RIGHT) sequence = "\e[C";
            else if(keycode == SDLK_LEFT) sequence = "\e[D";
        }else{
            sequence = "\e[1;";
            sequence += modifierCode;
            if(keycode == SDLK_F1) sequence += "P";
            else if(keycode == SDLK_F2) sequence += "Q";
            else if(keycode == SDLK_F3) sequence += "R";
            else if(keycode == SDLK_F4) sequence += "S";
            else if(keycode == SDLK_HOME) sequence += "H";
            else if(keycode == SDLK_END) sequence += "F";
            else if(keycode == SDLK_UP) sequence += "A";
            else if(keycode == SDLK_DOWN) sequence += "B";
            else if(keycode == SDLK_RIGHT) sequence += "C";
            else if(keycode == SDLK_LEFT) sequence += "D";           
        }

        return sequence;
    }

    //notice keys like print screen or pause break are not listed
    //they are often not sent or are inconsistent so we will ignore
    if(keycode == SDLK_F5) sequence = "\e[15";
    else if(keycode == SDLK_F6) sequence = "\e[17";
    else if(keycode == SDLK_F7) sequence = "\e[18";
    else if(keycode == SDLK_F8) sequence = "\e[19";
    else if(keycode == SDLK_F9) sequence = "\e[20";
    else if(keycode == SDLK_F10) sequence = "\e[21";
    else if(keycode == SDLK_F11) sequence = "\e[23";
    else if(keycode == SDLK_F12) sequence = "\e[24";
    else if(keycode == SDLK_INSERT) sequence = "\e[2";
    else if(keycode == SDLK_DELETE) sequence = "\e[3";
    else if(keycode == SDLK_PAGEUP) sequence = "\e[5";
    else if(keycode == SDLK_PAGEDOWN) sequence = "\e[6";

    if(sequence != ""){
        //add to sequence for modifiers
        if(modifierCode != '0'){
            sequence += ';';
            sequence += modifierCode;
        }

        sequence += '~';
    }

    return sequence;
}

void sendAsciiCharacter(SDL_Keycode keycode){
    //handle ALT
    if(SDL_GetModState() & SDL_KMOD_ALT)
        term->sendChar('\e');

    term->sendChar(getAsciiCode(keycode));
}

void handleKeypadInput(SDL_Keycode keycode){
    //handle NUM LOCK
    if(SDL_GetModState() & SDL_KMOD_NUM){
        if(keycode == SDLK_KP_1) sendAsciiCharacter(SDLK_1);
        else if(keycode == SDLK_KP_2) sendAsciiCharacter(SDLK_2);
        else if(keycode == SDLK_KP_3) sendAsciiCharacter(SDLK_3);
        else if(keycode == SDLK_KP_4) sendAsciiCharacter(SDLK_4);
        else if(keycode == SDLK_KP_5) sendAsciiCharacter(SDLK_5);
        else if(keycode == SDLK_KP_6) sendAsciiCharacter(SDLK_6);
        else if(keycode == SDLK_KP_7) sendAsciiCharacter(SDLK_7);
        else if(keycode == SDLK_KP_8) sendAsciiCharacter(SDLK_8);
        else if(keycode == SDLK_KP_9) sendAsciiCharacter(SDLK_9);
        else if(keycode == SDLK_KP_0) sendAsciiCharacter(SDLK_0);
        else if(keycode == SDLK_KP_PERIOD) sendAsciiCharacter(SDLK_PERIOD);
        else if(keycode == SDLK_KP_EQUALS) sendAsciiCharacter(SDLK_EQUALS);
        return;
    }

    if(keycode == SDLK_KP_DIVIDE) sendAsciiCharacter(SDLK_SLASH);
    else if(keycode == SDLK_KP_MULTIPLY) sendAsciiCharacter(SDLK_ASTERISK);
    else if(keycode == SDLK_KP_MINUS) sendAsciiCharacter(SDLK_MINUS);
    else if(keycode == SDLK_KP_PLUS) sendAsciiCharacter(SDLK_PLUS);
    else if(keycode == SDLK_KP_ENTER) sendAsciiCharacter(SDLK_RETURN);
    else if(keycode == SDLK_KP_7) term->sendSequence(getCodeSequence(SDLK_HOME));
    else if(keycode == SDLK_KP_1) term->sendSequence(getCodeSequence(SDLK_END));
    else if(keycode == SDLK_KP_9) term->sendSequence(getCodeSequence(SDLK_PAGEUP));
    else if(keycode == SDLK_KP_3) term->sendSequence(getCodeSequence(SDLK_PAGEDOWN));
    else if(keycode == SDLK_KP_0) term->sendSequence(getCodeSequence(SDLK_INSERT));
    else if(keycode == SDLK_KP_PERIOD) term->sendSequence(getCodeSequence(SDLK_DELETE));
}
