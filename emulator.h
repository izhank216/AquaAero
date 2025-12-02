#ifndef EMULATOR_H
#define EMULATOR_H

#include <string>
#include <vector>
#include <iostream>
#include <unicorn/unicorn.h> // PowerPC CPU emulation

struct Game {
    std::string name;
    std::string owner;
    std::string country;
    std::string path;
    unsigned char banner[32*32*4]; // raw banner RGBA (32x32)
};

struct GPU {
    void initializeOpenGL();
    void initializeVulkan();
    void renderFrame();
};

class AquaAeroEmulator {
public:
    AquaAeroEmulator();
    ~AquaAeroEmulator();

    void initialize();
    void loadISO(const std::string& path);
    void loadBanner();
    void runFrame();
    void run();

    Game currentGame;
    GPU gpu;

private:
    uc_engine *uc;
    void initCPU();
    void executeCPU();
};

#endif
