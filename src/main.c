#include "log.h"
#include "window.h"
#include "defines.h"


static int width = 1280;
static int height = 960;
static const char *title = "Minecraft";



struct Game game;


void init(){
    game.window = &window;
    
}


int main() {
    if (!window_create(width, height, title)) {
        FATAL("Failed to create main window");
        window_destroy();
        return FAIL;
    }
    
    window_loop();
    window_destroy();

    return OK;
}