#include "../include/terminal.h"

Terminal::Terminal(SDL_Renderer* renderer, int width, int height, std::string fontPath)
    : screenWidth(width),
    screenHeight(height),
    initialized(false),
    renderTarget(nullptr),
    renderer(renderer)
{
    font.setFilepath(fontPath);
}

Terminal::~Terminal(){
    SDL_DestroyTexture(renderTarget);
}

bool Terminal::init(){
    if(!(renderTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, screenWidth, screenHeight))){
        SDL_Log("Unable to create render target texture: %s\n", SDL_GetError());
        return false;
    }

    if(!SDL_SetTextureScaleMode(renderTarget, SDL_SCALEMODE_NEAREST)){
        SDL_Log("Could not set texture scale mode to SDL_SCALEMODE_NEAREST: %s\n", SDL_GetError());
        return false;
    }

    font.setRenderer(renderer);
    font.load();

    initialized = true;
    return true;
}

bool Terminal::render(int x, int y){
    if(!SDL_SetRenderTarget(renderer, renderTarget)){
        SDL_Log("Error setting render target to texture: %s\n", SDL_GetError());
        return false;
    }

    //TODO make own function
    //draw the text here
    font.render(0,0,'A');
    //end draw text
    
    if(!SDL_SetRenderTarget(renderer, nullptr)){
        SDL_Log("Error setting render target back to the window: %s\n", SDL_GetError());
        return false;
    }
    
    SDL_FRect destinationRect = {x,y, static_cast<float>(screenWidth), static_cast<float>(screenHeight)};
    if(!SDL_RenderTexture(renderer, renderTarget, nullptr, &destinationRect)){
        SDL_Log("Error rendering terminal texture to window: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void Terminal::update(){
    return;
}
