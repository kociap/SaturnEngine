#ifndef MVG_SHADER_HPP_
#define MVG_SHADER_HPP_

#include <string_view>
#include <unordered_map>

#include "Matrix.hpp"
#include "Vector.hpp"

namespace Saturn {

class Shader {
public:
    struct CreateInfo {
        std::string_view vtx_path;
        std::string_view frag_path;
    };

    static void bind(Shader& shader);
    static void unbind();

    Shader() = default;
    Shader(CreateInfo create_info);
    Shader(Shader&& rhs) = delete;
    Shader& operator=(Shader&& rhs) = delete;

    void assign(CreateInfo create_info);

    unsigned int handle();

    void set_int(std::string_view name, int value);
    void set_float(std::string_view name, float value);

    void set_vec3(std::string_view name, Math::Vec3<float> const& value);
    void set_vec4(std::string_view name, Math::Vec4<float> const& value);
    void set_mat4(std::string_view name, Math::Matrix4x4<float> const& value);

    // #TODO: Uniform location caching
    int location(std::string_view name);

private:
    unsigned int program = 0;

    std::unordered_map<std::string_view, int> uniform_cache;
};

} // namespace Saturn

#endif
