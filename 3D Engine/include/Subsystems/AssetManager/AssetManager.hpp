#ifndef MVG_ASSET_MANAGER_HPP_
#define MVG_ASSET_MANAGER_HPP_

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

#include "Resource.hpp"
#include "ResourceLoaders.hpp"
#include "Utility/IDGenerator.hpp"

namespace Saturn {

template<typename R>
class AssetManager {
public:
    // If the resource is not loaded yet, this function will load it. Otherwise
    // it just returns the resource
    static Resource<R> get_resource(std::string const& path) {
        // Check if the resource has been loaded already
        if (id_map.find(path) != id_map.end()) {
            auto id = id_map[path];
            R* raw = resources[id].get();
            return Resource<R>(raw, id, true, path);
        }

        auto id = IDGenerator<R>::next();
        std::unique_ptr<R> res = ResourceLoader<R>::load(path);
        if (res == nullptr) {
            return Resource<R>(nullptr, -1, false, path);
        } else {
            id_map[path] = id;
            R* raw = res.get();
            resources[id] = std::move(res);
            return Resource<R>(raw, id, true, path);
        }
    }

    static Resource<R> get_resource(typename R::CreateInfo const& info,
                                    std::string const& name) {
        auto id = IDGenerator<R>::next();
        std::unique_ptr<R> res = std::make_unique<R>(info);
        if (res == nullptr) {
            return Resource<R>(nullptr, -1, false, "");
        } else {
            id_map[name] = id;
            R* raw = res.get();
            resources[id] = std::move(res);
            return Resource<R>(raw, id, true, name);
        }
    }

    // Resource must be loaded already.
    static Resource<R> get_loaded_resource(std::size_t id) {
        return resources.at(id);
    }

    //#TODO: Smart resource unloading (refcount in Resource<R>)

    static void unload(std::size_t id) { resources.erase(resources.find(id)); }

private:
    static inline std::unordered_map<std::size_t, std::unique_ptr<R>> resources;
    static inline std::unordered_map<std::string, std::size_t> id_map;
};

template<typename R>
void from_json(nlohmann::json const& j, Resource<R>& res) {
    auto path = j["Resource"];
    res = AssetManager<R>::get_resource(path);
}

template<typename R>
void to_json(nlohmann::json& j, Resource<R> const& res) {
    j["Resource"] = res.get_path();
}

} // namespace Saturn

#endif
