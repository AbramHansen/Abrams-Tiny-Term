#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fstream>
#include <unordered_map>

#include "ascii_font.h"

struct bufferCharacter {
    char character;
    //first 4 bits for foregound second 4 for backgound
    char color;
};

class Terminal{
    private:
        int pixelWidth, pixelHeight;
        int columns, rows;
        bool initialized;

        std::vector<std::vector<bufferCharacter>> buffer;
        int bufferLines;
        int bufferIndex;

        AsciiFont font;
        std::string fontPath;
        unsigned int paddingX, paddingY;

        int theme[16];

        SDL_Texture* renderTarget;
        SDL_Renderer* renderer;
        
        int masterFD, slaveFD;
        pid_t childPID;

        bool initPTY(std::string shell);
        bool initFont();
        void setPixelDimensions();
        bool loadConfig();
        bool loadParametersFromFile(std::string filepath, std::unordered_map<std::string, std::string> &parameters);
    public:
        Terminal(SDL_Renderer* renderer);
        ~Terminal();
        bool init(std::string shell = "sh");
        void update();
        bool render(int x = 0, int y = 0);
        void setPadding(unsigned int x, unsigned int y);
        bool updateDimensions(int newWidth, int newHeight);
        void sendChar(char character);
        void sendSequence(const std::string& sequence);
        int getPixelWidth();
        int getPixelHeight();
};
