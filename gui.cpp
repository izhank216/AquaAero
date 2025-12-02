#include "emulator.h"
#include <SDL.h>
#include <SDL_image.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

std::vector<Game> games;

int gui_main() {
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_GAMECONTROLLER)!=0) return -1;

    SDL_Window* window = SDL_CreateWindow("AquaAero Emulator", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(window,renderer);
    ImGui_ImplSDLRenderer_Init(renderer);

    AquaAeroEmulator emu;
    emu.initialize();

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

                    // Load ISO into Unicorn memory now
                    std::ifstream isoFile(g.path, std::ios::binary | std::ios::ate);
                    if(isoFile.is_open()) {
                        std::streamsize size = isoFile.tellg();
                        isoFile.seekg(0, std::ios::beg);

                        std::vector<char> buffer(size);
                        if(isoFile.read(buffer.data(), size)) {
                            // Map memory large enough to hold ISO
                            uc_mem_map(emu.uc, 0x10000000, size, UC_PROT_ALL);
                            uc_mem_write(emu.uc, 0x10000000, buffer.data(), size);
                            std::cout << "Loaded " << g.name << " into Unicorn memory\n";
                        }
                        isoFile.close();
                    }

                    games.push_back(g);
                }
            }
        }

        for(size_t i=0;i<games.size();i++) {
            std::string label = games[i].name + " | " + games[i].owner + " | " + games[i].country;
            if(ImGui::Selectable(label.c_str())) {
                // Load ISO for selected game
                emu.loadISO(games[i].path);

                // Map and write full ISO to Unicorn memory
                std::ifstream isoFile(games[i].path, std::ios::binary | std::ios::ate);
                if(isoFile.is_open()) {
                    std::streamsize size = isoFile.tellg();
                    isoFile.seekg(0, std::ios::beg);

                    std::vector<char> buffer(size);
                    if(isoFile.read(buffer.data(), size)) {
                        uc_mem_map(emu.uc, 0x10000000, size, UC_PROT_ALL);
                        uc_mem_write(emu.uc, 0x10000000, buffer.data(), size);
                    }
                    isoFile.close();
                }

                // Set PC to start of ISO (simplified: real entry should come from DOL header)
                uc_reg_write(emu.uc, UC_PPC_REG_PC, 0x10000000);

                emu.run(); // Run the game
            }
        }

        ImGui::End();
        ImGui::Render();

        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
