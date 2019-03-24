#ifndef MVG_RENDERER_HPP_
#define MVG_RENDERER_HPP_

#include "glad/glad.h"
#include "Utility.hpp"

#include <functional>

namespace Saturn {

class Application;

class Renderer {
public:
    struct CreateInfo {
        WindowDim screen_size;
        Application& app;
    };

    Renderer(CreateInfo create_info);

    Renderer(Renderer const&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer const&) = default;
    Renderer& operator=(Renderer&&) = default;

    void clear(Color clear_color,
               GLenum flags = GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

private:
    std::reference_wrapper<Application> app;
    WindowDim screen_size;
};

} // namespace Saturn

#endif