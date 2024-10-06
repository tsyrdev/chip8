#include <iostream>
#include <chrono>
#include <thread> 
#include <string>

#include <SDL.h>
#include <utility>
#include "chip.hpp"

const char* WINDOW_NAME = "CHIP8";

SDL_Texture* texture; 
SDL_Window* window; 
SDL_Renderer* renderer; 

Chip* chip; 

int keys[16] = {
    SDL_SCANCODE_X,
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_W,
    SDL_SCANCODE_E,
    SDL_SCANCODE_A,
    SDL_SCANCODE_S,
    SDL_SCANCODE_D,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_C,
    SDL_SCANCODE_4,
    SDL_SCANCODE_R,
    SDL_SCANCODE_F,
    SDL_SCANCODE_V
};

void init(char *filepath) {
   if (SDL_Init(SDL_INIT_VIDEO) < 0)
        std::cerr << "Failed to initialize the SDL system: " << SDL_GetError();

    window = SDL_CreateWindow(WINDOW_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 512, SDL_WINDOW_SHOWN);
    if (window == nullptr)
        std::cerr << "Failed to initialize the window: " << SDL_GetError(); 
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
        std::cerr << "Failed to initialize the renderer: " << SDL_GetError(); 
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);
    if (texture == nullptr)
        std::cerr << "Failed to initialize the texture: " << SDL_GetError(); 

    chip = new Chip(); 
    chip->LoadRom(filepath);
}

void deinit() {
    delete chip; 
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();   
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Please enter the filepath for the ROM to be read." << std::endl;
        return 1; 
    } else if (argc > 2) {
        std::cout << "Please enter only one filepath." << std::endl; 
        return 2;
    }

    init(argv[1]); 

    bool keepOpen = true; 
    bool render = true; 
    while(keepOpen) {
        SDL_Event ev; 
        int num; 
        if (SDL_PollEvent(&ev) > 0) {
            if (ev.type == SDL_QUIT)
               keepOpen = false;  
            if (ev.type == SDL_KEYDOWN)
                for (int i = 0; i < 16; ++i) 
                    if (keys[i] == ev.key.keysym.scancode) {
                        std::cout << "here" << std::endl; 
                        chip->keys[i] = 1;
                    }
            if (ev.type == SDL_KEYUP)
                for (int i = 0; i < 16; ++i) 
                    if (keys[i] == ev.key.keysym.scancode)
                        chip->keys[i] = 0;
        }

        if (chip->FetchDecodeExec() == OPRET::DRAW)
            render = true; 

        if (render) { // 16x larger 
            void* pixels;
            int pitch; 
            SDL_LockTexture(texture, nullptr, &pixels, &pitch); 
            uint32_t* pixelData = static_cast<uint32_t*>(pixels); 
            uint8_t (&vram)[32][64] = chip->getVram(); 

            for (int y = 0; y < 32; ++y)
                for (int x = 0; x < 64; ++x) {
                    if (vram[y][x])
                        pixelData[(y * pitch / 4) + x] = 0xFFFFFFFF; 
                }
            SDL_UnlockTexture(texture);

            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60hz 
    }

    deinit(); 
    return 0; 
};
