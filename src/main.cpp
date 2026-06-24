#include <iostream>
#include <string>
#include <cstdint>
#include <fstream>

#include "image.h"
#include "jpeg.h"

void saveBitstream(const std::string& path, const std::vector<uint8_t>& bitstream, int width, int height) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(&width),  sizeof(int));
    file.write(reinterpret_cast<const char*>(&height), sizeof(int));
    file.write(reinterpret_cast<const char*>(bitstream.data()), bitstream.size());
}

std::vector<uint8_t> loadBitstream(const std::string& path, int& width, int& height) {
    std::ifstream file(path, std::ios::binary);
    file.read(reinterpret_cast<char*>(&width),  sizeof(int));
    file.read(reinterpret_cast<char*>(&height), sizeof(int));
    return std::vector<uint8_t>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

int main(int argc, char* argv[]) {

    if (argc != 4) {
        std::cerr << "Invalid number of arguments!" << std::endl;
        std::cerr << "main <compress = 0 / decompress = 1> <input> <output>" << std::endl;
        return 1;
    }

    int type = atoi(argv[1]);
    std::string inputPath = argv[2];
    std::string outputPath = argv[3];

    switch (type) {
        case 0: {
            img::Image image;
            if (!image.Load(inputPath)) {
                std::cerr << "Fail to load image!" << std::endl;
                return 1;
            }
            std::cout << "Image loaded" << std::endl;
            std::vector<uint8_t> bitstream = jpeg::compress(image);
            std::cout << "Image compressed" << std::endl;
            saveBitstream(outputPath, bitstream, image.Width(), image.Height());
            break;
        }
        case 1: {
            int width, height;
            std::vector<uint8_t> bitstream = loadBitstream(inputPath, width, height);
            img::Image image = jpeg::decompress(bitstream, width, height);
            std::cout << "Image decompressed" << std::endl;
            if (!image.Save(outputPath)) {
                std::cerr << "Fail to save image!" << std::endl;
                return 1;
            }
            std::cout << "Image saved" << std::endl;
            break;
        }
        default:
            std::cerr << "Invalid type" << std::endl;
            return 1;
    }

    return 0;
}
