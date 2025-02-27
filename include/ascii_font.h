#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <string>
#include <fstream>
#include <vector>

struct BoundingBox{
    int width;
    int height;
    int xOffset;
    int yOffset;
};

class AsciiFont{
    private:
        std::string bdfFilepath;
        SDL_Renderer* renderer;
        //offset ascii code by -32 to get array index
        std::array<SDL_Texture*, 95> characters;

        BoundingBox fontBoundingBox;
        
        SDL_Texture* createCharacterTexture(std::vector<std::string> characterMap, BoundingBox BBX);
    public:
        AsciiFont();
        ~AsciiFont();
        void setFilepath(std::string filepath);
        void setRenderer(SDL_Renderer* renderer);
        bool load(); //returns false if load is unsuccessful
        int getWidth();
        int getHeight();
        bool render(float x, float y, char character);
};
