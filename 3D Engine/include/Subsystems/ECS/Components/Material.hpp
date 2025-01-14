#ifndef MVG_MATERIAL_HPP_
#define MVG_MATERIAL_HPP_

#include "ComponentBase.hpp"
#include "Subsystems/AssetManager/Resource.hpp"
#include "Subsystems/Renderer/Shader.hpp"
#include "Subsystems/Renderer/Texture.hpp"

namespace Saturn::Components {

struct COMPONENT Material : public ComponentBase {
    Resource<Shader> shader;
    // Optional
    Resource<Texture> texture;
    bool lit = true;

    // Lighting data
    Resource<Texture> diffuse_map;
    Resource<Texture> specular_map;
	float shininess;
};

} // namespace Saturn::Components

#endif
