#include "jpeg.h"
#include "huffman.h"
#include <iostream>

static const int LUMA_TABLE[64] = {
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68,109,103, 77,
    24, 35, 55, 64, 81,104,113, 92,
    49, 64, 78, 87,103,121,120,101,
    72, 92, 95, 98,112,100,103, 99
};
 
static const int CHROMA_TABLE[64] = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};

static const int ZIGZAG[64] = {
     0,  1,  5,  6, 14, 15, 27, 28,
     2,  4,  7, 13, 16, 26, 29, 42,
     3,  8, 12, 17, 25, 30, 41, 43,
     9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};

static double cosTable[64];
static bool isCosTableReady = false;

static void buildCosTable() {
	if (isCosTableReady) return;
	for (int i = 0; i < 64; i++) {
		int n = i % 8;
		int k = i / 8;
		cosTable[n * 8 + k] = std::cos((2.0 * n + 1.0) * k * PI / 16.0);
	}
	isCosTableReady = true;
}

std::vector<uint8_t> jpeg::compress(img::Image image) {
	int width = image.Width();
	int height = image.Height();

	img::RGB2YCbCr(image);
	
	std::vector<unsigned char> y  = image.GetChannel(0);
	std::vector<unsigned char> cb = image.GetChannel(1);
	std::vector<unsigned char> cr = image.GetChannel(2);

	cb = downsampling(cb, width, height);
	cr = downsampling(cr, width, height);

	const int chromaWidth  = width / 2;
	const int chromaHeight = height / 2;

	std::vector<double> yDCT  = dct(y, width, height);
	std::vector<double> cbDCT = dct(cb, chromaWidth, chromaHeight);
	std::vector<double> crDCT = dct(cr, chromaWidth, chromaHeight);

	std::vector<int> yQuant  = quantization(yDCT, width, height, false);
	std::vector<int> cbQuant = quantization(cbDCT, chromaWidth, chromaHeight, true);
	std::vector<int> crQuant = quantization(crDCT, chromaWidth, chromaHeight, true);	

	yQuant = zigZagScan(yQuant, width, height);
	cbQuant = zigZagScan(cbQuant, chromaWidth, chromaHeight);
	crQuant = zigZagScan(crQuant, chromaWidth, chromaHeight);

	std::vector<uint8_t> bitstream = huffman::encode(yQuant, cbQuant, crQuant, width, height);

	return bitstream;
}

img::Image jpeg::decompress(std::vector<uint8_t> bitstream, int width, int height) {
	std::vector<int> yQuant, cbQuant, crQuant;	

	huffman::decode(bitstream, yQuant, cbQuant, crQuant, width, height);

	const int chromaWidth = width / 2;
	const int chromaHeight = height / 2;

	yQuant = reverseZigZagScan(yQuant, width, height);	
	cbQuant = reverseZigZagScan(cbQuant, chromaWidth, chromaHeight);	
	crQuant = reverseZigZagScan(crQuant, chromaWidth, chromaHeight);

	std::vector<double> yDCT = dequantization(yQuant, width, height, false);	
	std::vector<double> cbDCT = dequantization(cbQuant, chromaWidth, chromaHeight, true);	
	std::vector<double> crDCT = dequantization(crQuant, chromaWidth, chromaHeight, true);	

	std::vector<unsigned char> y = inverseDCT(yDCT, width, height);
	std::vector<unsigned char> cb = inverseDCT(cbDCT, chromaWidth, chromaHeight);
	std::vector<unsigned char> cr = inverseDCT(crDCT, chromaWidth, chromaHeight);

    const int dctWidth  = (width / 8) * 8;
    const int dctHeight = (height / 8) * 8;

	cb = upsampling(cb, dctWidth, dctHeight);
	cr = upsampling(cr, dctWidth, dctHeight);

    img::Image image(dctWidth, dctHeight, 3);
    image.SetChannel(0, y);
    image.SetChannel(1, cb);
    image.SetChannel(2, cr);

    img::YCbCr2RGB(image);

    img::Image full(width, height, 3);
    for (int v = 0; v < height; v++) {
        for (int u = 0; u < width; u++) {
            int srcU = std::min(u, dctWidth  - 1);
            int srcV = std::min(v, dctHeight - 1);
            for (int c = 0; c < 3; c++)
                full.SetValue(u, v, c, image.GetValue(srcU, srcV, c));
        }
    }

	return image;
}

std::vector<unsigned char> jpeg::downsampling(std::vector<unsigned char> chroma, int width, int height) {
	const int downWidth = width/2;
	const int downHeight = height/2;
	std::vector<unsigned char> output(downWidth * downHeight);

	for (int v = 0; v < downHeight; v++) {
		for (int u = 0; u < downWidth; u++) {
			int p0 = chroma[(2 * v) * width + (2 * u)];
			int p1 = chroma[(2 * v) * width + (2 * u + 1)];
			int p2 = chroma[(2 * v + 1) * width + (2 * u)];
			int p3 = chroma[(2 * v + 1) * width + (2 * u + 1)];
			output[v * downWidth + u] = static_cast<unsigned char>((p0 + p1 + p2 + p3)/4);
		}
	}

	return output;
}

std::vector<unsigned char> jpeg::upsampling(std::vector<unsigned char> chroma, int width, int height) {
    const int downWidth  = (width  / 2 / 8) * 8;  // dimensão real após inverseDCT
    const int downHeight = (height / 2 / 8) * 8;

    std::vector<unsigned char> output(width * height);

    for (int v = 0; v < height; v++) {
        for (int u = 0; u < width; u++) {
            const int srcU = std::min(u / 2, downWidth  - 1);
            const int srcV = std::min(v / 2, downHeight - 1);
            output[v * width + u] = chroma[srcV * downWidth + srcU];
        }
    }

    return output;
}

std::vector<double> jpeg::dct(std::vector<unsigned char> channel, int width, int height) {
	buildCosTable();

	const int dctWidth = (width / 8) * 8;
	const int dctHeight = (height / 8) * 8;
	
	std::vector<double> output(dctWidth * dctHeight);

	for (int blockY = 0; blockY < dctHeight; blockY += 8) {
		for (int blockX = 0; blockX < dctWidth; blockX += 8) {	
		
			for (int v = 0; v < 8; v++) {
				for (int u = 0; u < 8; u++) {
					double sum = 0.0;
					
					for (int y = 0; y < 8; y++) {
						const double cosV = cosTable[y * 8 + v];
						for (int x = 0; x < 8; x++) {
							sum += 
								(channel[(blockY + y) * width + (blockX + x)] - 128.0) 
								* cosTable[x * 8 + u]
								* cosV;
						}
					}

					double cu = (u == 0) ? 1.0 / SQRT2 : 1.0;
            		double cv = (v == 0) ? 1.0 / SQRT2 : 1.0;	

					output[(blockY + v) * dctWidth + (blockX + u)] = 0.25 * cu * cv * sum;
				}
			}
		}
	}

	return output;	
}

std::vector<unsigned char> jpeg::inverseDCT(std::vector<double> dctMatrix, int width, int height) {
    buildCosTable();

    const int dctWidth  = (width / 8) * 8;
    const int dctHeight = (height / 8) * 8;

    std::vector<unsigned char> output(dctWidth * dctHeight);

    for (int blockY = 0; blockY < dctHeight; blockY += 8) {
        for (int blockX = 0; blockX < dctWidth; blockX += 8) {

            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    double sum = 0.0;

                    for (int v = 0; v < 8; v++) {
                        const double cosV = cosTable[y * 8 + v];
                        for (int u = 0; u < 8; u++) {
                            double cu = (u == 0) ? 1.0 / SQRT2 : 1.0;
                            double cv = (v == 0) ? 1.0 / SQRT2 : 1.0;

                            sum += cu * cv
                                * dctMatrix[(blockY + v) * dctWidth + (blockX + u)]
                                * cosTable[x * 8 + u]
                                * cosV;
                        }
                    }

                    double pixel = 0.25 * sum + 128.0;

                    pixel = img::Clamp(pixel);
                    output[(blockY + y) * dctWidth + (blockX + x)] = static_cast<unsigned char>(pixel);
                }
            }
        }
    }

    return output;
}

std::vector<int> jpeg::quantization(std::vector<double> dctMatrix, int width, int height, bool isChroma) {
	const int* table = isChroma ? CHROMA_TABLE : LUMA_TABLE;

	const int dctWidth = (width / 8) * 8;
	const int dctHeight = (height / 8) * 8; 

	std::vector<int> output(dctWidth * dctHeight);	

	for (int blockY = 0; blockY < dctHeight; blockY += 8) {
		for (int blockX = 0; blockX < dctWidth; blockX += 8) {		
			for (int y = 0; y < 8; y++) {
				for (int x = 0; x < 8; x++) {
					const int globalX = blockX + x;
					const int globalY = blockY + y;

					output[globalY * dctWidth + globalX] = static_cast<int>(
						std::round(dctMatrix[globalY * dctWidth + globalX] / table[y * 8 + x])
					);
				}				
			}
		}
	}
	
	return output;
}

std::vector<double> jpeg::dequantization(std::vector<int> quantizationMatrix, int width, int height, bool isChroma) {
	const int* table = isChroma ? CHROMA_TABLE : LUMA_TABLE;

	const int dctWidth = (width / 8) * 8;
	const int dctHeight = (height / 8) * 8; 

	std::vector<double> output(dctWidth * dctHeight);	

	for (int blockY = 0; blockY < dctHeight; blockY += 8) {
		for (int blockX = 0; blockX < dctWidth; blockX += 8) {		
			for (int y = 0; y < 8; y++) {
				for (int x = 0; x < 8; x++) {
					const int globalX = blockX + x;
					const int globalY = blockY + y;

					output[globalY * dctWidth + globalX] =
						quantizationMatrix[globalY * dctWidth + globalX] * table[y * 8 + x];
				}				
			}
		}
	}
	
	return output;
}

std::vector<int> jpeg::zigZagScan(std::vector<int> quantizationMatrix, int width, int height) {
    const int quantWidth  = (width / 8) * 8;
    const int quantHeight = (height / 8) * 8;
    std::vector<int> output(quantWidth * quantHeight);

    for (int blockY = 0; blockY < quantHeight; blockY += 8) {
        for (int blockX = 0; blockX < quantWidth; blockX += 8) {
            int block[64];
            for (int y = 0; y < 8; y++)
                for (int x = 0; x < 8; x++)
                    block[y * 8 + x] = quantizationMatrix[(blockY + y) * quantWidth + (blockX + x)];

            const int blockOffset = (blockY / 8 * (quantWidth / 8) + blockX / 8) * 64;
            for (int i = 0; i < 64; i++)
        		output[blockOffset + ZIGZAG[i]] = block[i];
    	}
	}
    return output;
}

std::vector<int> jpeg::reverseZigZagScan(std::vector<int> vector, int width, int height) {
    const int quantWidth  = (width / 8) * 8;
    const int quantHeight = (height / 8) * 8;
    std::vector<int> output(quantWidth * quantHeight);

    for (int blockY = 0; blockY < quantHeight; blockY += 8) {
        for (int blockX = 0; blockX < quantWidth; blockX += 8) {
			int block[64];
            const int blockOffset = (blockY / 8 * (quantWidth / 8) + blockX / 8) * 64;

            for (int i = 0; i < 64; i++)
                block[i] = vector[blockOffset + ZIGZAG[i]];

            for (int y = 0; y < 8; y++)
                for (int x = 0; x < 8; x++)
                    output[(blockY + y) * quantWidth + (blockX + x)] = block[y * 8 + x];
        }
    }

    return output;
}
