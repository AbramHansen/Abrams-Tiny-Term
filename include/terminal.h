#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "ascii_font.h"

class Terminal{
    private:
        int pixelWidth, pixelHeight;
        int columns, rows;
        bool initialized;

        AsciiFont font;
        unsigned int paddingX, paddingY;

        SDL_Texture* renderTarget;
        SDL_Renderer* renderer;
        
        int masterFD, slaveFD;
        pid_t childPID;
        bool initPTY(std::string shell);
        bool initFont();
    public:
        Terminal(
                SDL_Renderer* renderer,
                int width = 128,
                int height = 128,
                std::string fontPath = "../media/fonts/tom-thumb.bdf"
                );
        ~Terminal();
        bool init(std::string shell = "sh");
        void update();
        bool render(int x = 0, int y = 0);
        void setPadding(unsigned int x, unsigned int y);
        bool updateDimensions(int newWidth, int newHeight);
};
