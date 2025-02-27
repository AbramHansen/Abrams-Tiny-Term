#include "../include/terminal.h"

Terminal::Terminal(SDL_Renderer* renderer)
    : pixelWidth(0),
    pixelHeight(0),
    initialized(false),
    renderTarget(nullptr),
    renderer(renderer),
    paddingX(0),
    paddingY(0),
    cursorColumn(0),
    cursorRow(0),
    ptyOutputState(NORMAL_TEXT),
    currentCSISequence(""),
    currentOSCSequence(""),
    currentDCSSequence(""),
    tabWidth(8)
{}

Terminal::~Terminal(){
    SDL_DestroyTexture(renderTarget);
    if(initialized){
        kill(childPID, SIGKILL);
        waitpid(childPID, nullptr, 0);
        close(masterFD);
    }
}

bool Terminal::initPTY(){
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

    //fork, child process replaces itself with the configured shell
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

    if(!initPTY()){
        SDL_Log("Could not initialize PTY!\n");
        return false;
    }

    Line initialLine = {0, ""};
    lines.push_back(initialLine);

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

    drawLines(); 

    if(!SDL_SetRenderTarget(renderer, nullptr)){
        SDL_Log("Error setting render target back to the window: %s\n", SDL_GetError());
        return false;
    }
    
    SDL_FRect destinationRect = {static_cast<float>(x),static_cast<float>(y), static_cast<float>(pixelWidth), static_cast<float>(pixelHeight)};
    if(!SDL_RenderTexture(renderer, renderTarget, nullptr, &destinationRect)){
        SDL_Log("Error rendering terminal texture to window: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void Terminal::update(){
    const int BUFF_SIZE = 256;
    char buffer[BUFF_SIZE];
    ssize_t bytesRead = read(masterFD, buffer, sizeof(buffer));
    if (bytesRead > 0){
        write(STDOUT_FILENO, buffer, bytesRead);
        for(int i = 0; i < bytesRead; i++){
            /* sequence handling specifically must come before checking for '\e' 
             * this is due to OSI and DCS codes potentially being terminated with 0x1B 0x5C.
             */
            if(ptyOutputState == CSI_SEQUENCE){
                addToCSISequence(buffer[i]);
            }else if(ptyOutputState == OSC_SEQUENCE){
                addToOSCSequence(buffer[i]);
            }else if(ptyOutputState == DCS_SEQUENCE){
                addToDCSSequence(buffer[i]);           
            }else if(buffer[i] == '\e'){
                ptyOutputState = ESCAPE_START;
            }else if(ptyOutputState == ESCAPE_START){
                // determine what kind of escape sequence it is
                if(buffer[i] == '['){
                    ptyOutputState = CSI_SEQUENCE;
                }else if(buffer[i] == ']'){
                    ptyOutputState = OSC_SEQUENCE;
                }else if(buffer[i] == 'P'){
                    ptyOutputState = DCS_SEQUENCE;
                }else{ //single character sequence
                    handleSingleCharacterSequence(buffer[i]);
                    ptyOutputState = NORMAL_TEXT;
                }

            }else if(ptyOutputState == NORMAL_TEXT){
                if(buffer[i] < 32 || buffer[i] > 126){
                    handleAsciiCode(buffer[i]);
                }else{
                    lines.back().text.push_back(buffer[i]);
                }
            }

            /*if(buffer[i] == '\n' || buffer[i] == '\r'){ //newline
                lineIndex++;
                if(lineIndex >= maxLines)
                    lineIndex = 0;
            }else if(buffer[i] == '\b'){ //backspace
                lines[lineIndex].pop_back();
            }else{ // append output to line
                lines[lineIndex] += buffer[i];
            }*/
        }
    }
}

void Terminal::handleAsciiCode(char character){
    if(character == '\r'){
        int row = lines.back().row;
        row += lines.back().text.length() / columns;
        if(lines.back().text.length() % columns)
            row++;
        Line newline = {row, ""};
        lines.push_back(newline);
    }
}

void Terminal::handleSingleCharacterSequence(char command){
    ptyOutputState = NORMAL_TEXT;
}

static std::vector<unsigned int> parseParameters(const std::string& input) {
    std::vector<unsigned int> parameters;
    std::istringstream inputStream(input);
    std::string token;

    while (std::getline(inputStream, token, ';')) {
        if (token.empty()) {
            parameters.push_back(0); // Default value for empty parameters
        } else {
            try {
                parameters.push_back(std::stoi(token));
            } catch (const std::invalid_argument&) {
                parameters.push_back(0); // Invalid -> Default to 0
            } catch (const std::out_of_range&) {
                parameters.push_back(0); // Out of range -> Default to 0
            }
        }
    }

    if(!input.empty() && input.back() == ';')
        parameters.push_back(0);

    return parameters;
}

void Terminal::addToCSISequence(char character){
    if(character >= '@' && character <= '~'){
        if(!currentCSISequence.empty()){
            if(currentCSISequence[0] >= '0' && currentCSISequence[0] <= '9'){
                std::vector<unsigned int> parameters = parseParameters(currentCSISequence);
                handleCSISequence(parameters, character);
            }else if(currentCSISequence[0] == '?'){ // TODO ignored for now
                // handle private modes
            }else if(currentCSISequence[0] == '='){
                // handle set mode sequences
            }
        }
        ptyOutputState = NORMAL_TEXT;
        currentCSISequence = "";
    }else{
        currentCSISequence += character;
    }
}

void Terminal::handleCSISequence(std::vector<unsigned int> args, char command){
    return;
}

void Terminal::addToOSCSequence(char character){
    if((character == '\\' && !currentOSCSequence.empty() && currentOSCSequence.back() == '\e') || character == '\a'){
        // TODO impliment
        ptyOutputState = NORMAL_TEXT;
        currentOSCSequence = "";
    }else{
        currentOSCSequence += character;
    }
}

void Terminal::addToDCSSequence(char character){
    if(character == '\\' && !currentOSCSequence.empty() && currentOSCSequence.back() == '\e'){
        // TODO impliment
        ptyOutputState = NORMAL_TEXT;
        currentOSCSequence = "";
    }else{
        currentDCSSequence += character;
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
    
    // set shell
    if (parameters.find("shell") != parameters.end())
        shell = parameters["shell"];
    else
        shell = "sh";

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

    //set maxScrollbackLines
    if (parameters.find("max_lines") != parameters.end())
        maxScrollbackLines = safeStoi(parameters["scrollback_lines"], 10, 256);
    else
        maxScrollbackLines = 256;

    //set columns and rows
    if (parameters.find("columns") != parameters.end())
        columns = safeStoi(parameters["columns"], 10, 32);
    else
        columns = 32;

    if (parameters.find("rows") != parameters.end())
        rows = safeStoi(parameters["rows"], 10, 10);
    else
        rows = 10;
    
    if (parameters.find("tab_width") != parameters.end())
        tabWidth = safeStoi(parameters["tab_width"], 10, 8);
    else
        tabWidth = 8;

    // TODO add check for user config file in ~/.config/tiny-term
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

bool Terminal::drawCharacter(int column, int row, char character){
    if(column >= columns || row >= rows)
        return false;

    font.render(column * (font.getWidth() + paddingX), row * (font.getHeight() + paddingY), character);
    return true;
}

void Terminal::drawLines(){
    for(Line line : lines){
        if(!line.text.empty()){
            for(int i = 0; i < line.text.length(); i++){
                int characterRow = line.row + (i / columns);
                int characterColumn = i % columns;
                drawCharacter(characterColumn, characterRow, line.text[i]);
            }
        }
    }
}
