#pragma once

#include "stb_image.h"
#include <memory>

namespace HelloVulkan {

	class ImageData {

	public:

		ImageData(
			const char* filename
		);

		inline std::size_t size() const { return imageSize; }
		inline stbi_uc* data() const { return pixels.get(); }

		inline const int getWidth() const { return texWidth; }
		inline const int getHeight() const { return texHeight; }
		inline const int getChannels() const { return texChannels; }

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
		std::size_t imageSize = 0;
	};

}