#include "Subsystems/Math/RandomEngine.hpp"

namespace Saturn::Math {

void RandomEngine::initialize() { engine.seed(rd()); }

} // namespace Saturn::Math
