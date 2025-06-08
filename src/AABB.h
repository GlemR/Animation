#pragma once
#include <glm/glm.hpp>

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

// Transforms the AABB by a model matrix (handles scaling, rotation, translation)
AABB transformAABB(const AABB& box, const glm::mat4& modelMatrix);