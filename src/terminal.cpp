#include "../include/terminal.h"

Terminal::Terminal(SDL_Renderer* renderer, int width, int height, std::string fontPath)
    : screenWidth(width),
    screenHeight(height),
    initialized(false),
    renderTarget(nullptr),
    renderer(renderer)
{
    font.setFilepath(fontPath);
}

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

bool Terminal::init(std::string shell){
    if(!(renderTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, screenWidth, screenHeight))){
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

    font.setRenderer(renderer);
    font.load();

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
    
    SDL_FRect destinationRect = {x,y, static_cast<float>(screenWidth), static_cast<float>(screenHeight)};
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
        SDL_Log("-->");
        write(STDOUT_FILENO, buffer, bytesRead);
        SDL_Log("<--\n");
    }
}
