#pragma once

#include "stb_image.h"
#include <memory>

namespace HelloVulkan {

	class ImageData {

	public:

		ImageData(
			const char* filename
		);

	private:
		int texWidth;
		int texHeight;
		int texChannels;
		
		struct StbiDeleter {
			void operator()(stbi_uc* ptr) const noexcept {
				stbi_image_free(ptr);
			}
		};
		std::unique_ptr<stbi_uc, StbiDeleter> pixels = nullptr;
		std::size_t imageSize;
	};

}