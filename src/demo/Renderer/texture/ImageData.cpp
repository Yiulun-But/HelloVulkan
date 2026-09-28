#define STB_IMAGE_IMPLEMENTATION
#include "ImageData.h"

#include <stdexcept>

namespace HelloVulkan {


	ImageData::ImageData(
		const char* filename
	) {
		imageSize =
			static_cast<std::size_t>(texWidth) *
			static_cast<std::size_t>(texHeight) *
			4;

		pixels.reset(stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha));

		if (!pixels)
		{
			throw std::runtime_error("Failed to load image!");
		}
	}


}