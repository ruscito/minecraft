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
    if (window_create(width, height, title)) {
        window_loop();
    }
    window_destroy();

    return OK;
}