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
#include <vulkan/vulkan.h>

namespace fs = std::filesystem;

struct Game {
    std::string path;
    std::string name;
    std::string owner;
    std::string country;
    SDL_Texture* bannerTexture = nullptr;
};

std::vector<Game> games;

// GameCube framebuffer
const int fbWidth = 640;
const int fbHeight = 528;
uint32_t framebuffer[fbWidth * fbHeight] = {0};

// DSP audio buffer
const int audioSampleRate = 44100;
uint8_t audioBuffer[audioSampleRate * 4] = {0};

// RGBA4 -> RGBA8 conversion
void RGBA4toRGBA8(const uint16_t* src, uint8_t* dst, int pixels) {
    for(int i=0;i<pixels;i++){
        uint16_t val = src[i];
        dst[i*4+0] = ((val>>12)&0xF)*17;
        dst[i*4+1] = ((val>>8)&0xF)*17;
        dst[i*4+2] = ((val>>4)&0xF)*17;
        dst[i*4+3] = (val&0xF)*17;
    }
}

// Load banner
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

// Audio callback
void audioCallback(void* userdata, Uint8* stream, int len){
    memset(stream,0,len);
    // Later: mix DSP output
}

int gui_main() {
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_GAMECONTROLLER|SDL_INIT_AUDIO)!=0) return -1;

    // Audio
    SDL_AudioSpec spec{};
    spec.freq = audioSampleRate;
    spec.format = AUDIO_F32SYS;
    spec.channels = 2;
    spec.samples = 4096;
    spec.callback = audioCallback;
    SDL_OpenAudio(&spec,nullptr);
    SDL_PauseAudio(0);

    // Vulkan window
    SDL_Window* window = SDL_CreateWindow("AquaAero Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 768, SDL_WINDOW_VULKAN);

    // Vulkan setup
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSwapchainKHR swapchain;
    VkFormat swapchainFormat;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    // 1️⃣ Create instance
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "AquaAero Emulator";
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.pEngineName = "AquaAero Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo instInfo{};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;

    if(vkCreateInstance(&instInfo,nullptr,&instance)!=VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance\n";
        return -1;
    }

    // 2️⃣ Create surface
    if(!SDL_Vulkan_CreateSurface(window,instance,&surface)) {
        std::cerr << "Failed to create Vulkan surface\n";
        return -1;
    }

    // NOTE: Full Vulkan setup (device selection, swapchain, render pass, pipelines) 
    // is very long and complex. Here we assume framebuffer will be drawn using 
    // a Vulkan texture in the swapchain.

    // Setup SDL Renderer for ImGui (we'll overlay GUI on top)
    SDL_Renderer* renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(window,renderer);
    ImGui_ImplSDLRenderer_Init(renderer);

    AquaAeroEmulator emu;
    emu.initialize();

    SDL_Texture* fbTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, fbWidth, fbHeight);

    bool running = true;
    SDL_Event event;

    while(running){
        while(SDL_PollEvent(&event)){
            ImGui_ImplSDL2_ProcessEvent(&event);
            if(event.type==SDL_QUIT) running=false;
        }

        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Game List");

        if(ImGui::Button("Add ISOs in Folder")){
            for(auto& p: fs::directory_iterator(".")){
                if(p.path().extension()==".iso"){
                    Game g;
                    g.path = p.path().string();
                    g.name = p.path().filename().string();
                    g.owner = "Unknown";
                    g.country = "Unknown";
                    g.bannerTexture = loadBanner(renderer,g.path);
                    games.push_back(g);
                }
            }
        }

        for(size_t i=0;i<games.size();i++){
            if(games[i].bannerTexture){
                ImGui::Image((void*)games[i].bannerTexture,ImVec2(96,32));
                ImGui::SameLine();
            }
            std::string label = games[i].name + " | " + games[i].owner + " | " + games[i].country;
            if(ImGui::Selectable(label.c_str())){
                emu.loadISO(games[i].path);
                emu.loadDOLFromISO(); // Load text/data sections
                emu.run();
            }
        }

        ImGui::End();

        // Update Vulkan framebuffer (for now we copy framebuffer to SDL texture)
        SDL_UpdateTexture(fbTexture,nullptr,framebuffer,fbWidth*4);
        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer,fbTexture,nullptr,nullptr);
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

    vkDestroySurfaceKHR(instance,surface,nullptr);
    vkDestroyInstance(instance,nullptr);

    return 0;
}
