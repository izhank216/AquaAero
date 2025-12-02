#include "emulator.h"
#include <SDL.h>
#include <SDL_vulkan.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

struct Game {
    std::string path;
    std::string name;
    std::string owner;
    std::string country;
    SDL_Texture* bannerTexture = nullptr;
};

std::vector<Game> games;

// GameCube framebuffer size
const int fbWidth = 640;
const int fbHeight = 528;
uint32_t framebuffer[fbWidth * fbHeight] = {0};

// DSP audio buffer
const int audioSampleRate = 44100;
uint8_t audioBuffer[audioSampleRate * 4] = {0};

// RGBA4 -> RGBA8 conversion for banner
void RGBA4toRGBA8(const uint16_t* src, uint8_t* dst, int pixels) {
    for(int i = 0; i < pixels; i++) {
        uint16_t val = src[i];
        dst[i*4+0] = ((val>>12)&0xF)*17;
        dst[i*4+1] = ((val>>8)&0xF)*17;
        dst[i*4+2] = ((val>>4)&0xF)*17;
        dst[i*4+3] = (val&0xF)*17;
    }
}

// Load ISO banner
SDL_Texture* loadBanner(SDL_Renderer* renderer, const std::string& isoPath) {
    std::ifstream file(isoPath,std::ios::binary);
    if(!file.is_open()) return nullptr;
    file.seekg(0x20,std::ios::beg);
    const int width=96,height=32;
    uint16_t bannerRaw[width*height];
    file.read(reinterpret_cast<char*>(bannerRaw), width*height*2);
    file.close();
    uint8_t bannerRGBA[width*height*4];
    RGBA4toRGBA8(bannerRaw,bannerRGBA,width*height);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
        bannerRGBA, width, height, 32, width*4, SDL_PIXELFORMAT_RGBA32
    );
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer,surface);
    SDL_FreeSurface(surface);
    return texture;
}

// Audio callback (simple silence for now, DSP can be integrated later)
void audioCallback(void* userdata, Uint8* stream, int len) {
    memset(stream,0,len);
    // Later: mix DSP audio output here
}

int gui_main() {
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_GAMECONTROLLER|SDL_INIT_AUDIO)!=0) return -1;

    // Setup audio
    SDL_AudioSpec spec{};
    spec.freq = audioSampleRate;
    spec.format = AUDIO_F32SYS;
    spec.channels = 2;
    spec.samples = 4096;
    spec.callback = audioCallback;
    SDL_OpenAudio(&spec, nullptr);
    SDL_PauseAudio(0); // Start audio playback

    // Vulkan window
    SDL_Window* window = SDL_CreateWindow("AquaAero Emulator", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 768, SDL_WINDOW_VULKAN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(window,renderer);
    ImGui_ImplSDLRenderer_Init(renderer);

    AquaAeroEmulator emu;
    emu.initialize();

    SDL_Texture* fbTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, fbWidth, fbHeight);

    bool running = true;
    SDL_Event event;

    while(running) {
        while(SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if(event.type==SDL_QUIT) running=false;
        }

        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Game List");

        if(ImGui::Button("Add ISOs in Folder")) {
            for(auto& p: fs::directory_iterator(".")) {
                if(p.path().extension()==".iso") {
                    Game g;
                    g.path = p.path().string();
                    g.name = p.path().filename().string();
                    g.owner = "Unknown";
                    g.country = "Unknown";
                    g.bannerTexture = loadBanner(renderer, g.path);
                    games.push_back(g);
                }
            }
        }

        for(size_t i=0;i<games.size();i++) {
            if(games[i].bannerTexture) {
                ImGui::Image((void*)games[i].bannerTexture, ImVec2(96,32));
                ImGui::SameLine();
            }
            std::string label = games[i].name + " | " + games[i].owner + " | " + games[i].country;
            if(ImGui::Selectable(label.c_str())) {
                // Load ISO metadata and DOL sections
                emu.loadISO(games[i].path);
                emu.loadDOLFromISO(); // Writes text/data sections into Unicorn memory

                // Run the game
                emu.run();
            }
        }

        ImGui::End();

        // Update Vulkan framebuffer (currently using SDL texture for simplicity)
        SDL_UpdateTexture(fbTexture,nullptr,framebuffer,fbWidth*4);

        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, fbTexture, nullptr, nullptr);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(fbTexture);
    for(auto& g:games)
        if(g.bannerTexture) SDL_DestroyTexture(g.bannerTexture);

    SDL_CloseAudio();
    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
