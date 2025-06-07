#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include "Mesh.h"
#include "Texture.h"
#include "AABB.h"

class Model {
public:
    std::vector<Mesh> meshes;
    std::string directory;
    std::unordered_map<std::string, Texture> loadedTextures; // Cache for loaded textures

    // Optionally, store wall AABBs for easy collision
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
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
            return;
        }

        // Extract directory path from the model file path
        std::filesystem::path modelPath(path);
        directory = modelPath.parent_path().string();
        if (!directory.empty() && directory.back() != '/' && directory.back() != '\\') {
            directory += "/";
        }

        std::cout << "Model directory: " << directory << std::endl;
        std::cout << "Model has " << scene->mNumMeshes << " meshes" << std::endl;

        meshes.reserve(scene->mNumMeshes);

        // Recursively process all nodes
        processNode(scene->mRootNode, scene);
        std::cout << "Successfully loaded " << meshes.size() << " meshes" << std::endl;
        std::cout << "Loaded " << loadedTextures.size() << " unique textures" << std::endl;
    }

    void processNode(aiNode* node, const aiScene* scene) {
        // Process all meshes in this node
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* ai_mesh = scene->mMeshes[node->mMeshes[i]];
            loadMesh(ai_mesh, scene, node->mName.C_Str());
        }
        // Recursively process children
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    void loadMesh(const aiMesh* aiMesh, const aiScene* scene, const std::string& meshName) {
        if (!aiMesh || aiMesh->mNumVertices == 0) {
            std::cout << "  Warning: Invalid or empty mesh!" << std::endl;
            return;
        }

        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<Texture> textures;

        vertices.reserve(aiMesh->mNumVertices);

        // Track bounding box for this mesh
        float minX = FLT_MAX, maxX = -FLT_MAX;
        float minY = FLT_MAX, maxY = -FLT_MAX;
        float minZ = FLT_MAX, maxZ = -FLT_MAX;

        // Load vertices
        for (unsigned int i = 0; i < aiMesh->mNumVertices; ++i) {
            Vertex vertex;
            vertex.position.x = aiMesh->mVertices[i].x;
            vertex.position.y = aiMesh->mVertices[i].y;
            vertex.position.z = aiMesh->mVertices[i].z;

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

        // Load indices
        for (unsigned int i = 0; i < aiMesh->mNumFaces; ++i) {
            const aiFace& face = aiMesh->mFaces[i];
            if (face.mNumIndices == 3) {
                for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                    indices.push_back(face.mIndices[j]);
                }
            }
        }

        if (indices.empty()) {
            std::cout << "  Warning: Mesh has no valid triangular faces!" << std::endl;
            return;
        }

        // Load material textures
        if (aiMesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
            auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
            auto normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "normal");
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
            auto normalMaps2 = loadMaterialTextures(material, aiTextureType_NORMALS, "normal");
            textures.insert(textures.end(), normalMaps2.begin(), normalMaps2.end());
        }

        // Create the mesh using your existing Mesh class
        Mesh mesh(vertices, indices, textures);
        mesh.boundingBox.min = glm::vec3(minX, minY, minZ);
        mesh.boundingBox.max = glm::vec3(maxX, maxY, maxZ);
        mesh.position = 0.5f * (mesh.boundingBox.min + mesh.boundingBox.max);
        meshes.emplace_back(std::move(mesh));
    }

    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName) {
        std::vector<Texture> textures;
        unsigned int textureCount = mat->GetTextureCount(type);

        for (unsigned int i = 0; i < textureCount; i++) {
            aiString str;
            mat->GetTexture(type, i, &str);

            std::string texturePath = str.C_Str();
            std::string filename = std::filesystem::path(texturePath).filename().string();

            std::vector<std::string> possiblePaths = {
                "textures/MapSchool/" + filename,
            };

            std::string fullPath;
            bool foundTexture = false;
            for (const auto& path : possiblePaths) {
                if (std::filesystem::exists(path)) {
                    fullPath = path;
                    foundTexture = true;
                    break;
                }
            }
            if (!foundTexture) continue;

            if (loadedTextures.find(fullPath) != loadedTextures.end()) {
                textures.push_back(loadedTextures[fullPath]);
                continue;
            }

            GLenum format = GL_RGBA;
            GLenum pixelType = GL_UNSIGNED_BYTE;
            std::string extension = std::filesystem::path(fullPath).extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            if (extension == ".jpg" || extension == ".jpeg") format = GL_RGB;

            Texture texture(fullPath.c_str(), typeName.c_str(), i, format, pixelType);
            textures.push_back(texture);
            loadedTextures[fullPath] = texture;
        }
        return textures;
    }
};

#endif