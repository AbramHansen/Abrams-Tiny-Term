#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <string>
#include <fstream>
#include <vector>

class AsciiFont{
    private:
        std::string bdfFilepath;
        SDL_Renderer* renderer;
        //offset ascii code by -32 to get array index
        std::array<SDL_Texture*, 95> characters;
        int width, height;
        int paddingX, paddingY;

        SDL_Texture* createCharacterTexture(std::vector<std::string>);
    public:
        AsciiFont(int paddingX = 1, int paddingY = 0);
        ~AsciiFont();
        void setFilepath(std::string filepath);
        void setRenderer(SDL_Renderer* renderer);
        bool load(); //returns false if load is unsuccessful
        int getWidth();
        int getHeight();
        bool render(float x, float y, char character);
};
