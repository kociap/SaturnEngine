#include "Subsystems/Renderer/Renderer.hpp"

#include "Core/Application.hpp"
#include "Subsystems/Logging/LogSystem.hpp"
#include "Subsystems/Math/Math.hpp"
#include "Subsystems/Renderer/PostProcessing.hpp"
#include "Subsystems/Scene/Scene.hpp"
#include "Utility/Exceptions.hpp"
#include "Utility/Utility.hpp"
#include "Utility/bind_guard.hpp"

namespace Saturn {

static std::vector<float> screen_vertices = {
    // Vertices	        Texture coords
    -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // TL
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // BL
    1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // BR
    1.0f,  1.0f,  0.0f, 1.0f, 1.0f, // TR
};

Renderer::Renderer(CreateInfo create_info) :
    app(create_info.app), screen_size(create_info.screen_size) {

    // Setup the framebuffer
    Framebuffer::CreateInfo framebuffer_create_info;
    framebuffer_create_info.size = screen_size;
    framebuf.assign(framebuffer_create_info);

    // Create default viewport
    add_viewport(Viewport(0, 0, screen_size.x, screen_size.y));
    // Set it as the active viewport
    Viewport::set_active(get_viewport(0));

    std::vector<VertexAttribute> screen_attributes;
    screen_attributes.push_back({0, 3}); // Position is a vec3
    screen_attributes.push_back({1, 2}); // TexCoords is a vec2
    std::vector<VertexArray::CreateInfo::Buffer> buffers(1);
    buffers[0].attributes = screen_attributes;
    buffers[0].vertices = screen_vertices;
    buffers[0].mode = BufferMode::Static;
    screen.assign({buffers, {0, 1, 2, 0, 3, 2}});

    PostProcessing::get_instance().load_shaders(
        "resources/shaders/postprocessing/postprocessing_effects.ppe");
    PostProcessing::get_instance().set_active("none");

    UniformBuffer::CreateInfo matrix_info;
    matrix_info.binding_point = 0;
    matrix_info.dynamic = true; // View/Projection matrices can change
    matrix_info.size_in_bytes = 2 * sizeof(glm::mat4);
    matrix_buffer.assign(matrix_info);

    no_shader_error =
        AssetManager<Shader>::get_resource("resources/shaders/default.sh");
    particle_shader =
        AssetManager<Shader>::get_resource("resources/shaders/particle.sh");
}

Renderer::~Renderer() {}

void Renderer::clear(
    Color clear_color,
    GLenum flags /*= GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT*/) {

    bind_guard<Framebuffer> framebuf_guard(framebuf);

    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    glClear(flags);
}

void Renderer::render_scene(Scene& scene) {
    // Temporary

    bind_guard<Framebuffer> framebuf_guard(framebuf);
    // All temporary obviously

    // Render every viewport
    for (auto& vp : viewports) {
        if (!vp.has_camera()) continue;
        Viewport::set_active(vp);

        auto cam_id = vp.get_camera();
        auto& cam = scene.ecs.get_with_id<Components::Camera>(cam_id);
        auto& cam_trans = cam.entity->get_component<Components::Transform>();
        auto projection = glm::perspective(
            glm::radians(cam.fov),
            (float)vp.dimensions().x / (float)vp.dimensions().y, 0.1f, 100.0f);

        auto view = glm::lookAt(cam_trans.position,
                                cam_trans.position + cam.front, cam.up);

        bind_guard<UniformBuffer> uniform_guard(matrix_buffer);
        matrix_buffer.set_mat4(projection, 0);
        matrix_buffer.set_mat4(view, sizeof(glm::mat4));

        render_particles(scene); // #TODO: Check if it makes any difference if
                                 // we render particles before or after the
                                 // scene + figure out best option

        for (auto [relative_transform, mesh, material] :
             scene.ecs.select<Components::Transform, Components::StaticMesh,
                              Components::Material>()) {
            auto& shader = material.shader.is_loaded() ? material.shader.get()
                                                       : no_shader_error.get();
            auto& vtx_array = mesh.mesh->get_vertices();

            auto transform = make_absolute_transform(relative_transform);

            auto model = glm::mat4(1.0f);
            // Apply transformations

            model = glm::translate(model, transform.position);
            model = glm::rotate(model, {glm::radians(transform.rotation.x),
                                        glm::radians(transform.rotation.y),
                                        glm::radians(transform.rotation.z)});

            model = glm::scale(model, transform.scale);

            shader.set_mat4(Shader::Uniforms::Model, model);

            bind_guard<Shader> shader_guard(shader);
            bind_guard<VertexArray> vao_guard(vtx_array);

            if (material.texture.is_loaded() && material.shader.is_loaded()) {
                // If there is a texture
                Texture::bind(material.texture.get());
                shader.set_int("tex", material.texture->unit() - GL_TEXTURE0);
            }

            glDrawElements(GL_TRIANGLES, vtx_array.index_size(),
                           GL_UNSIGNED_INT, nullptr);

            if (material.texture.is_loaded() && material.shader.is_loaded()) {
                Texture::unbind(material.texture.get());
            }
        }
    }
}

//#MaybeTODO: Render particles with GL_POINTS if they're not textured?
void Renderer::render_particles(Scene& scene) {
    using namespace Components;
    bind_guard<Shader> shader_guard(particle_shader.get());

    Resource<Texture> default_texture =
        AssetManager<Texture>::get_resource("resources/textures/white.tex");

    glDisable(GL_CULL_FACE);
    for (auto [emitter] : scene.ecs.select<ParticleEmitter>()) {
        if (emitter.additive) { glBlendFunc(GL_SRC_ALPHA, GL_ONE); }
        // Bind VAO
        bind_guard<VertexArray> vao_guard(emitter.particle_vao.get());
        for (ParticleEmitter::Particle const& particle : emitter.particles) {

            particle_shader->set_vec3(Shader::Uniforms::Position,
                                      particle.position);
            particle_shader->set_vec3(
                Shader::Uniforms::Scale,
                glm::vec3(particle.size.x, particle.size.y, 0.0f));
            auto& texture = emitter.texture.is_loaded() ? emitter.texture.get()
                                                        : default_texture.get();
            particle_shader->set_vec4(Shader::Uniforms::Color, particle.color);
            Texture::bind(texture);
            particle_shader->set_int(Shader::Uniforms::Texture,
                                     texture.unit() - GL_TEXTURE0);

            glDrawElements(GL_TRIANGLES, emitter.particle_vao->index_size(),
                           GL_UNSIGNED_INT, nullptr);

            Texture::unbind(texture);
        }

        if (emitter.additive) {
            // reset blend function to old one
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
    }
    glEnable(GL_CULL_FACE);
}

void Renderer::update_screen() {
    bind_guard<Framebuffer> framebuf_guard(screen_framebuf);

    Viewport::set_active(get_viewport(0));

    // Bind VAO
    bind_guard<VertexArray> screen_guard(screen);
    bind_guard<Ebo> ebo_guard(screen.ebo);

    // Temporarily disable some GL functionality
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Set (postprocessing) shader
    auto& shader = PostProcessing::get_instance().get_active().get();
    bind_guard<Shader> shader_guard(shader);

    // Render framebuffer texture to the screen
    glBindTexture(GL_TEXTURE_2D, framebuf.texture);
    shader.set_int(Shader::Uniforms::Texture, 0);
    glDrawElements(GL_TRIANGLES, screen.index_size(), GL_UNSIGNED_INT, nullptr);

    // Re enable functionality
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

Viewport& Renderer::get_viewport(std::size_t index) {
    if (index >= viewports.size()) {
        throw InvalidViewportException("Invalid viewport with index " +
                                       std::to_string(index) + " requested.");
    }
    return viewports[index];
}

std::size_t Renderer::add_viewport(Viewport vp) {
    viewports.push_back(vp);
    return viewports.size() - 1;
}

} // namespace Saturn
