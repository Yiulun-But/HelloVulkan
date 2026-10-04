#include "Window.h"

#include <SDL3/SDL_vulkan.h>
#include <stdexcept>

namespace HelloVulkan {

    Window::Window(const WindowProps& props)
    {
        m_Window = SDL_CreateWindow(
            props.Title.c_str(),
            static_cast<int>(props.Width),
            static_cast<int>(props.Height),
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
        );

        if (!m_Window)
        {
            throw std::runtime_error(SDL_GetError());
        }
    }

    Window::~Window()
    {
        if (m_Window)
        {
            SDL_DestroyWindow(m_Window);
        }
    }

    std::pair<int, int> Window::GetPixelSize() const
    {
        int width = 0;
        int height = 0;

        if (!SDL_GetWindowSizeInPixels(
            m_Window, &width, &height))
        {
            throw std::runtime_error(SDL_GetError());
        }

        return { width, height };
    }

    std::vector<const char*> Window::GetRequiredInstanceExtensions() const
    {
        uint32_t count_instance_extensions;
        auto extensions =  SDL_Vulkan_GetInstanceExtensions(&count_instance_extensions);
        return std::vector<const char*>(extensions, extensions + count_instance_extensions);
    }

    void Window::PollEvents()
    {
        SDL_Event event{};

        while (SDL_PollEvent(&event))
            HandleEvent(event);
    }

    bool Window::WaitEvent()
    {
        SDL_Event event{};

        if (!SDL_WaitEvent(&event))
        {
            throw std::runtime_error(SDL_GetError());
        }

        HandleEvent(event);

        return !m_ShouldClose;
    }

    void Window::HandleEvent(SDL_Event& event)
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            m_ShouldClose = true;
            break;

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            if (event.window.windowID == SDL_GetWindowID(m_Window))
                m_ShouldClose = true;
            break;

        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            if (event.window.windowID == SDL_GetWindowID(m_Window))
                m_FramebufferResized = true;
            break;
        }

    }



}