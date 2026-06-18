#pragma once
#include <cstdint>

// Physical address map
//   0x0000_0000  RAM (64 MB)
//   0x1000_0000  Accelerator config registers
//   0x2000_0000  Persistent storage commands

static constexpr uint64_t RAM_BASE = 0x00000000ULL;
static constexpr uint64_t RAM_SIZE = 64ULL * 1024 * 1024;

static constexpr uint64_t ACCEL_BASE = 0x10000000ULL;
static constexpr uint64_t ACCEL_SIZE = 256;

static constexpr uint64_t STORAGE_BASE     = 0x20000000ULL;
static constexpr uint64_t STORAGE_SIZE     = 0x1000;
static constexpr uint64_t STORAGE_CMD_LOAD = 0x00; 
static constexpr uint64_t STORAGE_CMD_SAVE = 0x10;  

static constexpr uint32_t IMG_WIDTH  = 1920;
static constexpr uint32_t IMG_HEIGHT = 1080;
static constexpr uint32_t NUM_PIXELS = IMG_WIDTH * IMG_HEIGHT;
static constexpr uint32_t RGB_SIZE   = NUM_PIXELS * 3;  // 6 220 800 bytes
static constexpr uint32_t GRAY_SIZE  = NUM_PIXELS;      

static constexpr uint64_t INPUT_ADDR  = RAM_BASE + 0x00000000ULL;
static constexpr uint64_t OUTPUT_ADDR = RAM_BASE + 0x00600000ULL;

// Written by CPU to ACCEL_BASE to trigger inference
struct AccelConfig {
    uint64_t input_addr;
    uint64_t output_addr;
    uint32_t num_pixels;
};

static constexpr double STORAGE_NS_PER_BYTE = 2.0;   // 500 MB/s
static constexpr double RAM_NS_PER_BYTE     = 0.25;  // 4 GB/s
static constexpr double ACCEL_NS_PER_PIXEL  = 1.0;   // 1 cycle/px @ 1 GHz
