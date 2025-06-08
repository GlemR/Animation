#include "Mesh.h"
#include "Camera.h"
#include <vector>
#include <glm/gtc/type_ptr.hpp>

Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, std::vector<Texture>& textures)
    : vertices(vertices), indices(indices), textures(textures)
{
    // Compute local AABB
    glm::vec3 min(FLT_MAX), max(-FLT_MAX);
    for (const auto& v : vertices) {
        min = glm::min(min, v.position);
        max = glm::max(max, v.position);
    }
    localAABB = { min, max };

    VAO.Bind();
    VBO VBO(vertices);
    EBO EBO(indices);
    VAO.LinkAttrib(VBO, 0, 3, GL_FLOAT, sizeof(Vertex), (void*)0);
    VAO.LinkAttrib(VBO, 1, 3, GL_FLOAT, sizeof(Vertex), (void*)(3 * sizeof(float)));
    VAO.LinkAttrib(VBO, 2, 3, GL_FLOAT, sizeof(Vertex), (void*)(6 * sizeof(float)));
    VAO.LinkAttrib(VBO, 3, 2, GL_FLOAT, sizeof(Vertex), (void*)(9 * sizeof(float)));
    VAO.Unbind();
    VBO.Unbind();
    EBO.Unbind();
}

void Mesh::ComputeWorldTriangles(const glm::mat4& modelMatrix) {
    worldTriangles.clear();
    std::vector<glm::vec3> worldVerts;
    worldVerts.reserve(vertices.size());
    for (const auto& v : vertices) {
        worldVerts.push_back(glm::vec3(modelMatrix * glm::vec4(v.position, 1.0f)));
    }
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        worldTriangles.push_back({ worldVerts[indices[i]], worldVerts[indices[i + 1]], worldVerts[indices[i + 2]] });
    }
}

void Mesh::Draw(Shader& shader, Camera& camera) {
    shader.Activate();
    VAO.Bind();

    // Keep track of how many of each type of textures we have
    unsigned int numDiffuse = 0;
    unsigned int numSpecular = 0;

    for (unsigned int i = 0; i < textures.size(); i++)
    {
        std::string num;
        std::string type = textures[i].type;
        if (type == "diffuse")
        {
            num = std::to_string(numDiffuse++);
        }
        else if (type == "specular")
        {
            num = std::to_string(numSpecular++);
        }
        textures[i].texUnit(shader, (type + num).c_str(), i);
        textures[i].Bind();
    }
    // Take care of the camera Matrix
    glUniform3f(glGetUniformLocation(shader.ID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
    camera.Matrix(shader, "camMatrix");

    // Draw the actual mesh
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

// Draws the AABB as lines (wireframe box)
void Mesh::DrawAABB(const glm::mat4& modelMatrix, Shader& aabbShader, Camera& camera) {
    glm::vec3 min = localAABB.min;
    glm::vec3 max = localAABB.max;
    glm::vec3 corners[8] = {
        {min.x, min.y, min.z}, {max.x, min.y, min.z},
        {max.x, max.y, min.z}, {min.x, max.y, min.z},
        {min.x, min.y, max.z}, {max.x, min.y, max.z},
        {max.x, max.y, max.z}, {min.x, max.y, max.z}
    };
    GLuint indices[24] = {
        0,1, 1,2, 2,3, 3,0, // bottom
        4,5, 5,6, 6,7, 7,4, // top
        0,4, 1,5, 2,6, 3,7  // sides
    };
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    aabbShader.Activate();
    glUniformMatrix4fv(glGetUniformLocation(aabbShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
    camera.Matrix(aabbShader, "camMatrix");
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);

    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
}