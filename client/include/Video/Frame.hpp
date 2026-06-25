#pragma once

#include <vector>
#include <cstdint>

struct Frame
{
    uint64_t frameNumber = 0;
    uint64_t timestamp = 0;

    int width = 0;
    int height = 0;

    std::vector<uint8_t> pixels;
};