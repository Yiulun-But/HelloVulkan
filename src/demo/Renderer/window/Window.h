#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>
#include <utility>

namespace HelloVulkan {

    struct WindowProps
    {
        std::string Title = "Hello Vulkan";
        uint32_t Width = 1280;
        uint32_t Height = 720;
    };

    class Window
    {
    public:
        explicit Window(const WindowProps& props = {});
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        SDL_Window* GetNativeWindow() const {
            return m_Window;
        }

        std::pair<int, int> GetPixelSize() const;

        std::vector<const char*>
            GetRequiredInstanceExtensions() const;

        VkSurfaceKHR CreateSurface(VkInstance instance) const;

        void PollEvents();
        bool WaitEvent();

        bool ShouldClose() const {
            return m_ShouldClose;
        }

        bool ConsumeFramebufferResize() {
            return std::exchange(m_FramebufferResized, false);
        }

    private:
        void HandleEvent(SDL_Event& event);

        SDL_Window* m_Window = nullptr;

        bool m_ShouldClose = false;
        bool m_FramebufferResized = false;
    };

}