#include "emulator.h"
#include <fstream>
#include <cstring>

AquaAeroEmulator::AquaAeroEmulator() : uc(nullptr) {}

AquaAeroEmulator::~AquaAeroEmulator() {
    if (uc) uc_close(uc);
}

void AquaAeroEmulator::initialize() {
    initCPU();
    gpu.initializeOpenGL();
    gpu.initializeVulkan();
}

void AquaAeroEmulator::loadISO(const std::string& path) {
    currentGame.path = path;
    currentGame.name = path.substr(path.find_last_of("/\\") + 1);
    loadBanner();
}

void AquaAeroEmulator::loadBanner() {
    std::ifstream iso(currentGame.path, std::ios::binary);
    if (!iso.is_open()) return;

    // GameCube banner offset: 0x20 (simplified assumption)
    iso.seekg(0x20, std::ios::beg);
    iso.read(reinterpret_cast<char*>(currentGame.banner), 32*32*4);
    iso.close();
}

void AquaAeroEmulator::initCPU() {
    uc_err err = uc_open(UC_ARCH_PPC, UC_MODE_32, &uc);
    if (err != UC_ERR_OK) {
        uc = nullptr;
        return;
    }
    uc_mem_map(uc, 0x1000, 2*1024*1024, UC_PROT_ALL);
    uc_reg_write(uc, UC_PPC_REG_R1, 0x200000);
}

void AquaAeroEmulator::executeCPU() {
    if (!uc) return;
    uint8_t code[4] = {0x7C,0x00,0x00,0x00}; // NOP
    uc_mem_write(uc, 0x1000, code, sizeof(code));
    uc_emu_start(uc, 0x1000, 0x1000+sizeof(code), 0, 0);
}

void AquaAeroEmulator::runFrame() {
    executeCPU();
    gpu.renderFrame();
}

void AquaAeroEmulator::run() {
    for (int i=0;i<5;i++) runFrame();
}

// GPU stub
void GPU::initializeOpenGL() {
    // OpenGL context setup stub
}
void GPU::initializeVulkan() {
    // Vulkan context setup stub
}
void GPU::renderFrame() {
    // Clear frame / render stub
}
