#include "../include/ascii_font.h"

AsciiFont::AsciiFont(std::string bdfFilepath, int paddingX, int paddingY){
    this->renderer = nullptr;
    this->bdfFilepath = bdfFilepath;
    this->paddingX = paddingX;
    this->paddingY = paddingY;
    this->width = 0;
    this->height = 0;
}

void AsciiFont::setRenderer(SDL_Renderer* renderer){
    this->renderer = renderer;
}

static std::vector<int> getNumsFromString(std::string input){
    std::vector<int> output;

    for(int i = 0; i < input.size(); i++){
        while(!(input[i] > 47 && input[i] < 58 && i < input.size())){
            i++;
        }
        std::string currentNum;
        while(input[i] > 47 && input[i] < 58 && i < input.size()) {
            currentNum += input[i];
            i++;
        }
        if (currentNum != "") {
            output.push_back(std::stoi(currentNum));
        }
    }

    return output;
}

SDL_Texture* AsciiFont::createCharacterTexture(std::vector<std::string> characterMap){
    std::vector<unsigned char> pixelData;

    for(int row = 0; row < characterMap.size(); row++){
        int number = (int) std::stoi(characterMap[row], nullptr, 16);

        int sizeOfLine = characterMap[row].size() * 4 - 1;
        for(int startBit = sizeOfLine; startBit >= sizeOfLine - width + 1; startBit--){
            if((number >> startBit) & 1){
                pixelData.push_back(0xFF);
                pixelData.push_back(0xFF);
                pixelData.push_back(0xFF);
            } else {
                pixelData.push_back(0x00);
                pixelData.push_back(0x00);
                pixelData.push_back(0x00);
            }
        }
    }
    
    SDL_Surface* characterSurface;
    if(!(characterSurface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGB24, pixelData.data(), width * 3))){
        SDL_Log("Unable to create character surface from pixel data: %s\n", SDL_GetError());
    }
    
    SDL_Texture* characterTexture;
    if(!(characterTexture = SDL_CreateTextureFromSurface(renderer, characterSurface))){
        SDL_Log("Unable to create character texture from surface: %s\n", SDL_GetError());
    }

    SDL_DestroySurface(characterSurface);
    characterSurface = nullptr;

    return characterTexture;
}

bool AsciiFont::load(){
    bool success = true;

    std::ifstream bdfFile(bdfFilepath);

    if(bdfFile.is_open()){
        std::string line;
        
        //find width and height
        bool boundingBoxFound = false;
        while(!boundingBoxFound && getline(bdfFile, line)){
            if(std::string::npos != line.find("FONTBOUNDINGBOX")){
                boundingBoxFound = true;
                std::vector nums = getNumsFromString(line);
                if(nums.size() > 1){
                    width = nums[0];
                    height = nums[1];
                } else {
                    SDL_Log("Error in font file: %s\n", bdfFilepath.c_str());
                    success = false;
                }
            }
        }
        if(!boundingBoxFound){
            SDL_Log("Error in font file: %s\n", bdfFilepath.c_str());
            success = false;
        }

        //extract characters
        while(getline(bdfFile, line)){
            if(std::string::npos != line.find("ENCODING")){
                int asciiIndex = -1;
                std::vector lineNumbers = getNumsFromString(line);
                if(lineNumbers.size() > 0){
                    asciiIndex = lineNumbers[0];

                    if(asciiIndex > 31 && asciiIndex < 127){
                        while(getline(bdfFile, line)){
                            if(std::string::npos != line.find("BITMAP")){
                                std::vector<std::string> characterData;
                                while(getline(bdfFile, line) && std::string::npos == line.find("ENDCHAR")){
                                    characterData.push_back(line);
                                }

                                characters[asciiIndex - 32] = createCharacterTexture(characterData);
                                break;
                            }
                        }
                    }
                }
            }
        }

        bdfFile.close();
    } else {
        SDL_Log("Unable to load font file: %s\n", bdfFilepath.c_str());
        success = false;
    }

    return success;
}

AsciiFont::~AsciiFont(){
    for(SDL_Texture* texture : characters){
        SDL_DestroyTexture(texture);
        texture = nullptr;
        width = 0;
        height = 0;
    }
}

int AsciiFont::getWidth(){
    return width;
}

int AsciiFont::getHeight(){
    return height;
}

bool AsciiFont::render(float x, float y, char character){
    bool success = true;
    if(character > 32 && character < 127) {
        SDL_FRect destination = {x,y, static_cast<float>(width), static_cast<float>(height)};
        SDL_RenderTexture(renderer, characters[character - 32], nullptr, &destination);
    } else {
        success = false;
    }
    return success;
}
