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
#include <termios.h>
#include <sstream>

#include "ascii_font.h"

struct Line{
    int row;
    std::string text;
};

class Terminal{
    private:
        int pixelWidth, pixelHeight;
        int columns, rows;
        bool initialized;

        std::vector<std::string> scrollBackLines;
        int maxScrollbackLines;

        std::vector<Line> lines;

        int tabWidth;

        enum PtyOutputState {
            NORMAL_TEXT,
            ESCAPE_START,
            CSI_SEQUENCE,
            OSC_SEQUENCE,
            DCS_SEQUENCE
        };
        enum PtyOutputState ptyOutputState;
        void handleAsciiCode(char command);
        void handleSingleCharacterSequence(char command);
        /*
        These "addTo" functions accumulate characters in their string
        until they determine the sequence is done.
        They then call the handle function, reset ptyOutputState to NORMAL_TEXT,
        and clear their string.
        */
        std::string currentCSISequence;
        void addToCSISequence(char character);
        void handleCSISequence(std::vector<unsigned int> args, char command);
        // TODO fully impliment these. They currently just ignore the sequence.
        std::string currentOSCSequence;
        void addToOSCSequence(char character);
        void handleOSCSequence();
        std::string currentDCSSequence;
        void addToDCSSequence(char character);
        void handleDCSSequence();

        int cursorColumn, cursorRow;

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
        bool drawCharacter(int column, int row, char character); //TODO add color
        void drawLines();
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
