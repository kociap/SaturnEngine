#include "Subsystems/Renderer/VertexArray.hpp"

#include "Subsystems/Logging/LogSystem.hpp"
#include "Subsystems/Renderer/OpenGL.hpp"
#include "Utility/bind_guard.hpp"

#include <cassert>
#include <numeric>
#include <vector>

namespace Saturn {

static std::size_t
get_vertex_size_in_bytes(std::vector<VertexAttribute> const& attributes) {
    std::size_t result = 0;
    for (auto& attr : attributes) {
        result += attr.num_components * sizeof(float);
    }
    return result;
}

VertexArray::VertexArray(CreateInfo const& create_info) { assign(create_info); }

VertexArray::~VertexArray() {}

void VertexArray::assign(CreateInfo const& create_info) {
    if (vao.id != 0) glDeleteBuffers(1, &vao.id);
    if (ebo.id != 0) glDeleteBuffers(1, &ebo.id);
    for (auto& vbo : buffers) { glDeleteBuffers(1, &vbo->id); }

    glGenVertexArrays(1, &vao.id);
    // Create buffers
    buffers.resize(create_info.buffers.size());
    for (std::size_t i = 0; i < create_info.buffers.size(); ++i) {
        buffers[i] = std::make_unique<Vbo<BufferTarget::ArrayBuffer>>();
        glGenBuffers(1, &buffers[i]->id);
    }
    glGenBuffers(1, &ebo.id);

    do_create(create_info);
}

std::size_t VertexArray::size() const { return vertex_count; }
std::size_t VertexArray::index_size() const { return indices_size; }

void VertexArray::bind(VertexArray& buf) {
    glBindVertexArray(buf.vao.id);
    Ebo::bind(buf.ebo);
}

void VertexArray::unbind() {
    glBindVertexArray(0);
    Ebo::unbind();
}

void VertexArray::do_create(CreateInfo const& create_info) {

    if (vao.id == 0) {
        LogSystem::write(LogSystem::Severity::Error, "Failed to create VAO");
    }
    if (buffers.empty()) {
        LogSystem::write(LogSystem::Severity::Error,
                         "Failed to create buffers!");
    }

    //#MaybeOptimize
    indices = create_info.indices;

    // If the EBO is empty, generate a default one (indices from 0 ... to
    // vertex_count)

    // The amount of vertices is equal to the total size in bytes of the
    // array divided by the vertex size in bytes
    std::size_t vertex_size_in_bytes =
        get_vertex_size_in_bytes(create_info.buffers[0].attributes);
    vertex_count = create_info.buffers[0].vertices.size() * sizeof(float) /
                   vertex_size_in_bytes;

    if (indices.empty()) {
        // Make space for N elements (N is vertex count), to fill buffer in
        // next call
        indices.resize(vertex_count);
        // Fill with values ranging 0 ... N where N is the vertex count
        std::iota(indices.begin(), indices.end(), 0);
    }

    indices_size = indices.size();
    bind_guard ebo_guard(ebo);
    // Fill EBO
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
                 indices.data(),
                 GL_STATIC_DRAW); // Indices will not change often

    for (std::size_t i = 0; i < create_info.buffers.size(); ++i) {
        auto const& buffer_info = create_info.buffers[i];
        auto& buffer = *buffers[i];
        bind_guard vao_guard(vao);
        bind_guard<Vbo<BufferTarget::ArrayBuffer>> vbo_guard(buffer);

        GLint mode;
        switch (buffer_info.mode) {
            case BufferMode::Static: mode = GL_STATIC_DRAW; break;
            case BufferMode::Dynamic: mode = GL_DYNAMIC_DRAW; break;
            case BufferMode::DataStream: mode = GL_STREAM_DRAW; break;
        }

        // Fill VBO
        glBufferData(GL_ARRAY_BUFFER,
                     buffer_info.vertices.size() * sizeof(float),
                     buffer_info.vertices.data(), mode);

        std::size_t offset = 0;
        for (auto& attr : buffer_info.attributes) {
            assert(attr.num_components >= 1 && attr.num_components <= 4);

            glVertexAttribPointer(attr.location_in_shader, attr.num_components,
                                  GL_FLOAT, GL_FALSE, vertex_size_in_bytes,
                                  (void*)(offset * sizeof(float)));
            if (attr.divisor != 0) {
                glVertexAttribDivisor(attr.location_in_shader, attr.divisor);
            }

            glEnableVertexAttribArray(attr.location_in_shader);
            offset += attr.num_components; // move the offset pointer for
                                           // the next iteration
        }
    }
}

void VertexArray::update_buffer(std::size_t buffer_index,
                                std::size_t start_index,
                                float const* data,
                                std::size_t count) {
    auto& buf = *buffers[buffer_index];
    bind_guard guard(buf);
    glBufferSubData(static_cast<GLenum>(buf.target),
                    start_index * sizeof(float), count * sizeof(float), data);
}

} // namespace Saturn
