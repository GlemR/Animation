#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H
#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>
#include <iostream>
#include "Mesh.h"
#include "Texture.h"

class Model {
public:
    std::vector<Mesh> meshes;

    Model(const std::string& path) {
        std::cout << "Loading model: " << path << std::endl;
        loadModel(path);
    }

    // Disable copy constructor and assignment
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    // Enable move constructor and assignment
    Model(Model&&) = default;
    Model& operator=(Model&&) = default;

    void Draw(Shader& shader, Camera& camera) {
        for (auto& mesh : meshes) {
            mesh.Draw(shader, camera);
        }
    }

private:
    void loadModel(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_FlipUVs |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
            return;
        }

        if (!scene->HasMeshes()) {
            std::cerr << "Model has no meshes!" << std::endl;
            return;
        }

        std::cout << "Model has " << scene->mNumMeshes << " meshes" << std::endl;

        // Reserve space to avoid reallocations
        meshes.reserve(scene->mNumMeshes);

        // Load ALL meshes
        for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
            const aiMesh* aiMesh = scene->mMeshes[meshIndex];
            std::cout << "Loading mesh " << meshIndex << ": " << aiMesh->mNumVertices
                << " vertices, " << aiMesh->mNumFaces << " faces" << std::endl;

            loadMesh(aiMesh);
        }

        std::cout << "Successfully loaded " << meshes.size() << " meshes" << std::endl;
    }

    void loadMesh(const aiMesh* aiMesh) {
        if (!aiMesh || aiMesh->mNumVertices == 0) {
            std::cout << "  Warning: Invalid or empty mesh!" << std::endl;
            return;
        }

        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<Texture> textures; // Empty for now, you can add texture loading later

        // Reserve space for better performance
        vertices.reserve(aiMesh->mNumVertices);

        // Track bounding box for this mesh
        float minX = FLT_MAX, maxX = -FLT_MAX;
        float minY = FLT_MAX, maxY = -FLT_MAX;
        float minZ = FLT_MAX, maxZ = -FLT_MAX;

        // Load vertices
        for (unsigned int i = 0; i < aiMesh->mNumVertices; ++i) {
            Vertex vertex;

            // Position
            vertex.position.x = aiMesh->mVertices[i].x;
            vertex.position.y = aiMesh->mVertices[i].y;
            vertex.position.z = aiMesh->mVertices[i].z;

            // Update bounding box
            minX = std::min(minX, vertex.position.x); maxX = std::max(maxX, vertex.position.x);
            minY = std::min(minY, vertex.position.y); maxY = std::max(maxY, vertex.position.y);
            minZ = std::min(minZ, vertex.position.z); maxZ = std::max(maxZ, vertex.position.z);

            // Normal
            if (aiMesh->HasNormals()) {
                vertex.normal.x = aiMesh->mNormals[i].x;
                vertex.normal.y = aiMesh->mNormals[i].y;
                vertex.normal.z = aiMesh->mNormals[i].z;
            }
            else {
                vertex.normal = glm::vec3(0.0f, 0.0f, 1.0f);
            }

            // Color (set to white by default)
            vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);

            // Texture coordinates
            if (aiMesh->HasTextureCoords(0)) {
                vertex.texUV.x = aiMesh->mTextureCoords[0][i].x;
                vertex.texUV.y = aiMesh->mTextureCoords[0][i].y;
            }
            else {
                vertex.texUV = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }

        // Print bounding box for this mesh
        std::cout << "  Mesh bounding box: X(" << minX << " to " << maxX << "), "
            << "Y(" << minY << " to " << maxY << "), "
            << "Z(" << minZ << " to " << maxZ << ")" << std::endl;

        // Load indices
        for (unsigned int i = 0; i < aiMesh->mNumFaces; ++i) {
            const aiFace& face = aiMesh->mFaces[i];
            if (face.mNumIndices == 3) { // Only triangles
                for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                    indices.push_back(face.mIndices[j]);
                }
            }
        }

        if (indices.empty()) {
            std::cout << "  Warning: Mesh has no valid triangular faces!" << std::endl;
            return;
        }

        // Create the mesh using your existing Mesh class
        meshes.emplace_back(vertices, indices, textures);

        std::cout << "  Mesh loaded successfully: " << vertices.size() << " vertices, "
            << indices.size() << " indices" << std::endl;
    }
};
#endif