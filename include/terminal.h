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
        int screenWidth;
        int screenHeight;
        bool initialized;

        AsciiFont font;

        SDL_Texture* renderTarget;
        SDL_Renderer* renderer;
        
        int masterFD, slaveFD;
        pid_t childPID;
        bool initPTY(std::string shell);
    public:
        Terminal(
                SDL_Renderer* renderer,
                int width = 128,
                int height = 64,
                std::string fontPath = "../media/fonts/tom-thumb.bdf"
                );
        ~Terminal();
        bool init(std::string shell = "sh");
        void update();
        bool render(int x = 0, int y = 0);
};
