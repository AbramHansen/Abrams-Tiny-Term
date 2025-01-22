#include "../include/ascii_font.h"

AsciiFont::AsciiFont(std::string bdfFilepath){
    std::ifstream bdfFile(bdfFilepath);

    if(bdfFile.is_open()){
        std::string line;
        while(getline(bdfFile, line)){
            SDL_Log("%s\n", line.c_str());
        }
        bdfFile.close();
    } else {
        SDL_Log("Unable to load font: %s\n", bdfFilepath.c_str());
    }
}
