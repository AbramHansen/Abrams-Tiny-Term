#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <string>
#include <fstream>

class AsciiFont{
    private:
        //offset ascii code by -32 to get array index
        std::array<SDL_Texture*, 94> mCharacters;
        int mWidth, mHeight;

    public:
        AsciiFont(std::string bdfFilepath);
        ~AsciiFont();
        SDL_Texture* getCharTexture(char character);
};
