#ifndef MESH_CLASS_H
#define MESH_CLASS_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "VAO.h"
#include "EBO.h"
#include "Texture.h"
#include "AABB.h"

class Camera;
class Shader;

class Mesh {
public:
    struct Triangle {
        glm::vec3 a, b, c;
    };
    std::vector<Triangle> worldTriangles;
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;
    VAO VAO;
    AABB localAABB; // Always in model (local) space

    Mesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, std::vector<Texture>& textures);

    void Draw(Shader& shader, Camera& camera);
    void DrawAABB(const glm::mat4& modelMatrix, Shader& aabbShader, Camera& camera);
    void ComputeWorldTriangles(const glm::mat4& modelMatrix);
};

#endif