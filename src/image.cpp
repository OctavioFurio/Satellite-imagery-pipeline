#include "image.h"

img::Image::Image()
	: width(0), height(0), channels(0) {}

img::Image::Image(int width, int height, int channels) 
	: 
	width(width), height(height), channels(channels), 
	values(width * height * channels, 0) {}

bool img::Image::Load(const std::string& filename) {
	int w, h, c;
	unsigned char* data = stbi_load(filename.c_str(), &w, &h, &c, STBI_rgb);
	c = 3;	

	if (!data) return false;	

	width = w;
	height = h;
	channels = c;

	values.assign(data, data + width * height * channels);
	stbi_image_free(data);

	return true;
}

bool img::Image::Save(const std::string& filename) const {
	return stbi_write_png(
		filename.c_str(), width, height, channels, 
		values.data(), width * channels);
}

unsigned char img::Image::GetValue(int u, int v, int channel) const {
	if (u < 0 || u >= width ||
		v < 0 || v >= height ||
		channel < 0 || channel >= channels)
		return 0;

	int index = (v * width + u) * channels + channel;
	return values[index];
}

void img::Image::SetValue(int u, int v, int channel, unsigned char value) {
	if (u < 0 || u >= width ||
		v < 0 || v >= height ||
		channel < 0 || channel >= channels)
		return;

	int index = (v * width + u) * channels + channel;
	values[index] = Clamp(value);
}

std::vector<unsigned char> img::Image::GetChannel(int channel) const {
	std::vector<unsigned char> matrix(width * height);

	if (channel < 0 || channel >= channels)
		return matrix;
	
	for (int i = 0; i < width * height; i++)
		matrix[i] = values[i * channels + channel];
	return matrix;
}

void img::Image::SetChannel(int channel, std::vector<unsigned char> matrix) {
	if ((int)matrix.size() < width * height)
		return;

	for (int i = 0; i < width * height; i++)
		values[i * channels + channel] = matrix[i];
}

int img::Image::Width() const { return width; }
int img::Image::Height() const { return height; }
int img::Image::Channels() const { return channels; }

int img::Clamp(int value) {
	return std::max(0, std::min(255, value));
}

void img::RGB2YCbCr(img::Image &image) {
	for (int v = 0; v < image.Height(); v++) {
		for (int u = 0; u < image.Width(); u++) {
			double r = image.GetValue(u, v, 0);
			double g = image.GetValue(u, v, 1);
			double b = image.GetValue(u, v, 2);

			double y = 0.299 * r + 0.587 * g + 0.114 * b;
			double cb = 128.0 - 0.168736 * r - 0.331264 * g + 0.5 * b;
			double cr = 128.0 + 0.5 * r - 0.418688 * g - 0.081312 * b;

			y = Clamp(std::round(y));
			cb = Clamp(std::round(cb));
			cr = Clamp(std::round(cr));
			
			image.SetValue(u, v, 0, y);
			image.SetValue(u, v, 1, cb);
			image.SetValue(u, v, 2, cr);
		}
	}
}

void img::YCbCr2RGB(img::Image &image) {
	for (int v = 0; v < image.Height(); v++) {
		for (int u = 0; u < image.Width(); u++) {
			double y = image.GetValue(u, v, 0);
			double cb = image.GetValue(u, v, 1);
			double cr = image.GetValue(u, v, 2);

			double r = y + 1.402 * (cr - 128.0);
			double g = y - 0.344136 * (cb - 128.0) - 0.714136 * (cr - 128.0);
			double b = y + 1.772 * (cb - 128.0);

			r = Clamp(std::round(r));
			g = Clamp(std::round(g));
			b = Clamp(std::round(b));

			image.SetValue(u, v, 0, r);
			image.SetValue(u, v, 1, g);
			image.SetValue(u, v, 2, b);
		}
	}
}
