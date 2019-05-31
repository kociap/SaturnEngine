#ifndef MVG_VERTEX_ARRAY_HPP_
#define MVG_VERTEX_ARRAY_HPP_

#include <cstddef>
#include <vector>
#include <memory>

#include "OpenGL.hpp"

namespace Saturn {

struct VertexAttribute {
    std::size_t location_in_shader;
    // The amount of components one element has. Must be 1, 2, 3 or 4
    std::size_t num_components;
    // How often the attribute should be updated in the shader. 0 means every
    // shader run, 1 means every new instance, 2 every 2 instances, etc
    std::size_t divisor = 0;
};

enum class BufferMode { Static, Dynamic, DataStream };

class VertexArray {
public:
    struct CreateInfo {
        struct Buffer {
            std::vector<VertexAttribute> attributes;
            std::vector<float> vertices;
            BufferMode mode = BufferMode::Static; ///< Indicates whether the
                                                  ///< buffer data will change
        };
        std::vector<Buffer> buffers;
        std::vector<GLuint>
            indices; ///< Index buffer. Leave empty to auto-generate
    };

    VertexArray() = default;
    VertexArray(CreateInfo const& create_info);

    VertexArray(VertexArray const&) = delete;
    VertexArray(VertexArray&& rhs) = delete;

    VertexArray& operator=(VertexArray const&) = delete;
    VertexArray& operator=(VertexArray&& rhs) = delete;

    void assign(CreateInfo const& create_info);

    ~VertexArray();

    static void bind(VertexArray& buf);
    static void unbind();

    // Returns the amount of vertices
    std::size_t size() const;

    // Returns the amount of indices in the index buffer
    std::size_t index_size() const;

	void update_buffer(std::size_t buffer_index,
                       std::size_t start_index,
                       float const* data,
                       std::size_t count);

private:
    friend class Renderer;

    void do_create(CreateInfo const& create_info);

    Vao vao;
    std::vector<std::unique_ptr<Vbo<BufferTarget::ArrayBuffer>>> buffers;
    Vbo<BufferTarget::ElementArrayBuffer> ebo;

    std::size_t vertex_count;
    std::size_t indices_size;

    std::vector<GLuint> indices;
};

} // namespace Saturn

#endif
