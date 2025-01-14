#ifndef MVG_CURVE_HPP_
#define MVG_CURVE_HPP_

#include "NumericRange.hpp"
#include <nlohmann/json.hpp>

namespace Saturn::Math {

enum class CurveShape { Constant, LinearUp, LinearDown };

struct Curve {
    CurveShape shape = CurveShape::Constant;
    float min;
    float max;

    float get(float x) const;

    numeric_range<float> output_range() const;
};

void to_json(nlohmann::json& j, Curve const& curve);
void from_json(nlohmann::json const& j, Curve& curve);

} // namespace Saturn::Math



#endif
