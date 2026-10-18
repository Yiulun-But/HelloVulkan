#define STB_IMAGE_IMPLEMENTATION
#include "ImageData.h"

#include <stdexcept>
#include <iostream>

namespace HelloVulkan {


	ImageData::ImageData(
		const char* filename
	) {
		pixels.reset(stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha));

		imageSize =
			static_cast<std::size_t>(texWidth) *
			static_cast<std::size_t>(texHeight) *
			4;

		mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

		if (!pixels)
		{
			throw std::runtime_error("Failed to load image: " + std::string(filename));
		}
	}


}