#include "../include/terminal.h"

Terminal::Terminal(SDL_Renderer* renderer)
    : pixelWidth(0),
    pixelHeight(0),
    initialized(false),
    renderTarget(nullptr),
    renderer(renderer),
    paddingX(1),
    paddingY(0)
{}

Terminal::~Terminal(){
    SDL_DestroyTexture(renderTarget);
    if(initialized){
        kill(childPID, SIGKILL);
        waitpid(childPID, nullptr, 0);
        close(masterFD);
    }
}

bool Terminal::initPTY(std::string shell){
    //Get file descriptors
    masterFD = posix_openpt(O_RDWR | O_NOCTTY);
    if(masterFD == -1)
        return false;
    if(grantpt(masterFD) == -1)
        return false;
    if(unlockpt(masterFD) == -1)
        return false;

    char slaveName[50];
    if(ptsname_r(masterFD, slaveName, 50) != 0)
        return false;

    slaveFD = open(slaveName, O_RDWR);

    //fork, child process replaces itself with bash
    pid_t pid;
    pid = fork();

    if(pid == 0){
        close(masterFD);
        
        //create new session group
        setsid();

        //make the terminal the controlling terminal for the session check for -1 error
        ioctl(slaveFD, TIOCSCTTY, nullptr); 

        //duplicate the slave file descriptor for stdin, stdout, and stderr
        dup2(slaveFD, 0);
        dup2(slaveFD, 1);
        dup2(slaveFD, 2);

        close(slaveFD);

        //execute shell and replace the current process
        execlp(shell.c_str(), shell.c_str(), nullptr);
    } else {
        close(slaveFD);
        childPID = pid;
        //prevent blocking reads
        fcntl(masterFD, F_SETFL, O_NONBLOCK);
    }

    return true;
}

bool Terminal::initFont(){
    font.setFilepath(fontPath);
    font.setRenderer(renderer);

    if(!font.load())
        return false;

    return true;
}

void Terminal::setPixelDimensions(){
    pixelWidth = columns * (font.getWidth() + paddingX);
    pixelHeight = rows * (font.getHeight() + paddingY);
}

bool Terminal::updateDimensions(int newWidth, int newHeight){
    pixelWidth = newWidth;
    pixelHeight = newHeight;

    columns = (pixelWidth + paddingX) / (font.getWidth() + paddingX);
    rows = (pixelHeight + paddingY) / (font.getHeight() + paddingY);

    SDL_Log("New column size: %i\n", columns);
    SDL_Log("New row size: %i\n", rows);

    //TODO could break if not initialized but needs to be called from init method
    //change the size of the pty
    struct winsize ws;
    ws.ws_col = columns;
    ws.ws_row = rows;
    if(ioctl(masterFD, TIOCSWINSZ, &ws) == -1)
        return false;

    //let the application running in the terminal know there was a change
    if(kill(childPID, SIGWINCH) == -1)
        return false;

    //update render target texture
    if(!(renderTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, pixelWidth, pixelHeight))){
        SDL_Log("Unable to update render target texture: %s\n", SDL_GetError());
        return false;
    }
   
    return true;
}

void Terminal::setPadding(unsigned int x, unsigned int y){
    paddingX = x;
    paddingY = y;
    
    if(initialized)
        updateDimensions(pixelWidth, pixelHeight);
}

bool Terminal::init(std::string shell){
    if(!loadConfig()){
        SDL_Log("Could not load config!\n");
        return false;
    }

    if(!initFont()){
        SDL_Log("Could not initialize font!\n");
        return false;
    }

    setPixelDimensions();

    if(!(renderTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, pixelWidth, pixelHeight))){
        SDL_Log("Unable to create render target texture: %s\n", SDL_GetError());
        return false;
    }

    if(!SDL_SetTextureScaleMode(renderTarget, SDL_SCALEMODE_NEAREST)){
        SDL_Log("Could not set texture scale mode to SDL_SCALEMODE_NEAREST: %s\n", SDL_GetError());
        return false;
    }

    if(!initPTY(shell)){
        SDL_Log("Could not initialize PTY!\n");
        return false;
    }

    initialized = true;
    return true;
}

bool Terminal::render(int x, int y){
    if(!initialized){
        SDL_Log("Call to Terminal::render before terminal is initialized!\n");
        return false;
    }

    if(!SDL_SetRenderTarget(renderer, renderTarget)){
        SDL_Log("Error setting render target to texture: %s\n", SDL_GetError());
        return false;
    }

    //TODO make own function
    //draw the text here
    font.render(0,0,'A');
    //end draw text
    
    if(!SDL_SetRenderTarget(renderer, nullptr)){
        SDL_Log("Error setting render target back to the window: %s\n", SDL_GetError());
        return false;
    }
    
    SDL_FRect destinationRect = {x,y, static_cast<float>(pixelWidth), static_cast<float>(pixelHeight)};
    if(!SDL_RenderTexture(renderer, renderTarget, nullptr, &destinationRect)){
        SDL_Log("Error rendering terminal texture to window: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void Terminal::update(){
    char buffer[100];
    ssize_t bytesRead = read(masterFD, buffer, sizeof(buffer));
    if (bytesRead > 0){
        write(STDOUT_FILENO, buffer, bytesRead);
    }
}

void Terminal::sendChar(char character){
    write(masterFD, &character, 1);
}

void Terminal::sendSequence(const std::string& sequence){
    write(masterFD, sequence.c_str(), sequence.length());
}

bool Terminal::loadParametersFromFile(std::string filepath, std::unordered_map<std::string, std::string> &parameters){
    std::ifstream file(filepath);
    if(!file.is_open()){
        SDL_Log("Unable to open config file: %s\n", filepath.c_str());
        return false;
    }

    std::string line;
    while(std::getline(file, line)){
        if(line.size() < 4) continue;
        if(line[0] == '#') continue; //comment
                                     
        std::string parameter = "";
        std::string value = "";

        int i;
        //get parameter
        for(i = 0; i < line.size(); i++){
            if(line[i] == ':'){
                i++;
                break;
            }

            if(line[i] != ' ') parameter += line[i];
        }

        //get value
        for(; i < line.size(); i++){
            if(line[i] != ' ') value += line[i];
        }

        if(parameter.size() > 0 && value.size() > 0){
            parameters[parameter] = value;
        }
    }

    file.close();

    return true;
}

static int safeStoi(const std::string& string, int base, int defaultValue = 0){
    try {
        int returnValue = stoi(string, nullptr, base);
        return returnValue;
    } catch (const std::invalid_argument&) {
        SDL_Log("Error converting %s to an integer!\n", string.c_str());
        return defaultValue;
    } catch (const std::out_of_range&) {
        SDL_Log("Error %s is too large or small\n", string.c_str());
        return defaultValue;
    }
}


bool Terminal::loadConfig(){
    std::unordered_map<std::string, std::string> parameters;
    std::string defaultConfigFilepath = "../media/defaults.conf";
    if(!loadParametersFromFile(defaultConfigFilepath, parameters))
        return false;

    //set fontPath
    if (parameters.find("font") != parameters.end())
        fontPath = "../media/fonts/" + parameters["font"] + ".bdf";
    else
        fontPath = "../media/fonts/tom-thumb.bdf";

    //load theme
    std::unordered_map<std::string, std::string> colors;
    if (parameters.find("theme") != parameters.end()){
        std::string themePath = "../media/themes/" + parameters["theme"];
        if(!loadParametersFromFile(themePath, colors))
            return false;

        for(int i = 0; i < 16; i++){
            std::string color = "color" + std::to_string(i);
            if(colors.find(color) != colors.end()){
                theme[i] = safeStoi(colors[color], 16);
            }
        }
    }

    //set columns and rows
    if (parameters.find("columns") != parameters.end())
        columns = safeStoi(parameters["columns"], 10, 32);
    else
        columns = 32;

    if (parameters.find("rows") != parameters.end())
        rows = safeStoi(parameters["rows"], 10, 10);
    else
        rows = 10;

    //set max buffer lines
    if (parameters.find("buffer_lines") != parameters.end())
        bufferLines = safeStoi(parameters["buffer_lines"], 1000);
    else
        bufferLines = 1000;

    //TODO add check for user config file in ~/.config/tiny-term
    return true;
}

int Terminal::getPixelWidth(){
    if(initialized)
        return pixelWidth;
    else
        return -1;
}

int Terminal::getPixelHeight(){
    if(initialized)
        return pixelHeight;
    else
        return -1;
}
