#include "AABB.h"
#include <cfloat>
#include <glm/gtc/matrix_transform.hpp>

AABB transformAABB(const AABB& box, const glm::mat4& modelMatrix) {
    glm::vec3 corners[8] = {
        {box.min.x, box.min.y, box.min.z},
        {box.max.x, box.min.y, box.min.z},
        {box.min.x, box.max.y, box.min.z},
        {box.max.x, box.max.y, box.min.z},
        {box.min.x, box.min.y, box.max.z},
        {box.max.x, box.min.y, box.max.z},
        {box.min.x, box.max.y, box.max.z},
        {box.max.x, box.max.y, box.max.z}
    };
    glm::vec3 newMin(FLT_MAX), newMax(-FLT_MAX);
    for (int i = 0; i < 8; ++i) {
        glm::vec3 transformed = glm::vec3(modelMatrix * glm::vec4(corners[i], 1.0f));
        newMin = glm::min(newMin, transformed);
        newMax = glm::max(newMax, transformed);
    }
    return {newMin, newMax};
}