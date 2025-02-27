#include "../include/ascii_font.h"

AsciiFont::AsciiFont(){
    this->renderer = nullptr;
    this->fontBoundingBox = {0,0,0,0};
}

void AsciiFont::setFilepath(std::string filepath){
    bdfFilepath = filepath;
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

SDL_Texture* AsciiFont::createCharacterTexture(std::vector<std::string> characterMap, BoundingBox BBX){
    const int NUM_COLOR_CHANNELS = 4;

    /* because of the way BDF files work, it is simplest to create pixel data first for the character
    and then figure out how we can align it using bounging box information for both the font and the individual character */
    std::vector<unsigned char> characterPixelData;
    std::vector<unsigned char> boundingPixelData(fontBoundingBox.width * fontBoundingBox.height * NUM_COLOR_CHANNELS, 0);

    // create character pixel data
    for(std::string row : characterMap){
        int bits = std::stoi(row, nullptr, 16);
        int shiftBits = 4 * row.size() - 1;
        for(int column = 0; column < BBX.width; column++){
            if((bits >> shiftBits) & 1){
                characterPixelData.push_back(0xFF);
                characterPixelData.push_back(0xFF);
                characterPixelData.push_back(0xFF);
                characterPixelData.push_back(0xFF);
            }else{
                characterPixelData.push_back(0x00);
                characterPixelData.push_back(0x00);
                characterPixelData.push_back(0x00);
                characterPixelData.push_back(0x00);
            }
            shiftBits--;
        }
    }
    
    // calculate coodinates of where the character suface starts in relation to the bounding box
    int startingRow = fontBoundingBox.height - fontBoundingBox.yOffset + BBX.yOffset - BBX.height;
    int startingColumn = BBX.xOffset;
    if(startingColumn < 0)
        startingColumn = 0;
    SDL_Rect coordinates = {startingColumn, startingRow};

    // create the two surfaces
    SDL_Surface* characterSurface;
    if(!(characterSurface = SDL_CreateSurfaceFrom(BBX.width, BBX.height, SDL_PIXELFORMAT_RGBA32, characterPixelData.data(), BBX.width * NUM_COLOR_CHANNELS))){
        SDL_Log("Unable to create character surface from pixel data: %s\n", SDL_GetError());
    }
    SDL_Surface* boundingSurface;
    if(!(boundingSurface = SDL_CreateSurfaceFrom(fontBoundingBox.width, fontBoundingBox.height, SDL_PIXELFORMAT_RGBA32, boundingPixelData.data(), fontBoundingBox.width * NUM_COLOR_CHANNELS))){
        SDL_Log("Unable to create character surface from pixel data: %s\n", SDL_GetError());
    }

    // blit the character onto the bounding surface
    if(!SDL_BlitSurface(characterSurface, nullptr, boundingSurface, &coordinates)){
        SDL_Log("Unable to blit characterSurface onto boundingSurface: %s\n", SDL_GetError());
    }
    
    // create the texture from the combined surfaces
    SDL_Texture* characterTexture;
    if(!(characterTexture = SDL_CreateTextureFromSurface(renderer, boundingSurface))){
        SDL_Log("Unable to create character texture from boundingSurface: %s\n", SDL_GetError());
    }

    // cleanup
    SDL_DestroySurface(characterSurface);
    characterSurface = nullptr;
    SDL_DestroySurface(boundingSurface);
    boundingSurface = nullptr;

    return characterTexture;
}

bool AsciiFont::load(){
    std::ifstream bdfFile(bdfFilepath);

    if(bdfFile.is_open()){
        std::string line;
        
        //find width and height
        bool boundingBoxFound = false;
        while(!boundingBoxFound && getline(bdfFile, line)){
            if(std::string::npos != line.find("FONTBOUNDINGBOX")){
                boundingBoxFound = true;
                std::vector nums = getNumsFromString(line);
                if(nums.size() > 3){
                    fontBoundingBox.width = nums[0];
                    fontBoundingBox.height = nums[1];
                    fontBoundingBox.xOffset = nums[2];
                    fontBoundingBox.yOffset = nums[3];  
                } else {
                    SDL_Log("Error in font file: %s\n", bdfFilepath.c_str());
                    return false;
                }
            }
        }
        if(!boundingBoxFound){
            SDL_Log("Error in font file: %s\n", bdfFilepath.c_str());
            return false;
        }

        //extract characters
        while(getline(bdfFile, line)){
            if(std::string::npos != line.find("ENCODING")){
                int asciiIndex = -1;
                std::vector<int> lineNumbers = getNumsFromString(line);
                if(lineNumbers.size() > 0){
                    asciiIndex = lineNumbers[0];

                    if(asciiIndex >= ' ' && asciiIndex <= '~'){
                        BoundingBox characterBoundingBox = {0,0,0,0};
                        while(getline(bdfFile, line)){
                            if(std::string::npos != line.find("BBX")){

                                std::vector<int> parameters = getNumsFromString(line);
                                if(parameters.size() >= 4){
                                    characterBoundingBox = {parameters[0], parameters[1], parameters[2], parameters[3]};     
                                }
                            }else if(std::string::npos != line.find("BITMAP")){
                                std::vector<std::string> characterData;
                                while(getline(bdfFile, line) && std::string::npos == line.find("ENDCHAR")){
                                    characterData.push_back(line);
                                }

                                characters[asciiIndex - 32] = createCharacterTexture(characterData, characterBoundingBox);
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
        return false;
    }

    return true;
}

AsciiFont::~AsciiFont(){
    for(SDL_Texture* texture : characters){
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

int AsciiFont::getWidth(){
    return fontBoundingBox.width;
}

int AsciiFont::getHeight(){
    return fontBoundingBox.height;
}

bool AsciiFont::render(float x, float y, char character){
    if(character > 32 && character < 127) {
        SDL_FRect destination = {x,y, static_cast<float>(fontBoundingBox.width), static_cast<float>(fontBoundingBox.height)};
       
        if(characters[character - 32] == nullptr){
            SDL_Log("Error, Character %c texture is nullptr!\n", character);
            return false;
        }

        if(!SDL_RenderTexture(renderer, characters[character - 32], nullptr, &destination)){
            SDL_Log("Error rendering character %c: %s\n", character, SDL_GetError());
            return false;
        }
    } else {
        return false;
    }
    return true; 
}
