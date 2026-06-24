#ifndef IMAGE_H
#define IMAGE_H

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

namespace img {
	class Image {
	private:
		int width;
		int height;
		int channels;
		std::vector<unsigned char> values;
	public:
		Image();
		Image(int width, int height, int channels);
		bool Load(const std::string& filename);
		bool Save(const std::string& filename) const;
		unsigned char GetValue(int u, int v, int channel) const;
		void SetValue(int u, int v, int channel, unsigned char value);
		std::vector<unsigned char> GetChannel(int channel) const;
		void SetChannel(int channel, std::vector<unsigned char> matrix);
		int Width() const;
		int Height() const;
		int Channels() const;
	};

	int Clamp(int value);

	void RGB2YCbCr(img::Image &image);
	void YCbCr2RGB(img::Image &image);
}

#endif
