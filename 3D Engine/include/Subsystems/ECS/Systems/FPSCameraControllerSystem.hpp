#ifndef MVG_FPS_CAMERA_CONTROLLER_SYSTEM_HPP_
#define MVG_FPS_CAMERA_CONTROLLER_SYSTEM_HPP_

#include "SystemBase.hpp"

namespace Saturn {

namespace Systems {

class FPSCameraControllerSystem : public SystemBase {
public:
    void on_start(Scene&) override;
    void on_update(Scene& scene) override;

};

} // namespace Systems

} // namespace Saturn

#endif
