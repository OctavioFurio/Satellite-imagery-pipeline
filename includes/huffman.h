#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <cstdint>
#include <vector>
 
namespace huffman {
    std::vector<uint8_t> encode(
        const std::vector<int>& yQuant,
        const std::vector<int>& cbQuant,
        const std::vector<int>& crQuant,
        int width, int height);
 
    void decode(
        const std::vector<uint8_t>& bitstream,
        std::vector<int>& yQuant,
        std::vector<int>& cbQuant,
        std::vector<int>& crQuant,
        int width, int height);
}

#endif
