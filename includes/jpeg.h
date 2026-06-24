#ifndef JPEG_H
#define JPEG_H

#include "image.h"
#include <cmath>
#include <cstdint>
#include <vector>

#define PI 3.141592653589
#define SQRT2 1.41421356237

namespace jpeg {
	std::vector<uint8_t> compress(img::Image image);
	img::Image decompress(std::vector<uint8_t> bitsteam, int width, int height);

	std::vector<unsigned char> downsampling(std::vector<unsigned char> chroma, int width, int height);
	std::vector<unsigned char> upsampling(std::vector<unsigned char> chroma, int width, int height);

	std::vector<double> dct(std::vector<unsigned char> channel, int width, int height);
	std::vector<unsigned char> inverseDCT(std::vector<double> dctMatrix, int width, int height);

	std::vector<int> quantization(std::vector<double> dctMatrix, int width, int height, bool isChroma);
	std::vector<double> dequantization(std::vector<int> quantizationMatrix, int width, int height, bool isChroma);

	std::vector<int> zigZagScan(std::vector<int> quantizationMatrix, int width, int height);
	std::vector<int> reverseZigZagScan(std::vector<int> vector, int width, int height);
}

#endif
