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

class Model {
public:
    std::vector<Mesh> meshes;
    std::string directory;
    std::unordered_map<std::string, Texture> loadedTextures; // Cache for loaded textures

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

        if (!scene->HasMeshes()) {
            std::cerr << "Model has no meshes!" << std::endl;
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

        // Reserve space to avoid reallocations
        meshes.reserve(scene->mNumMeshes);

        // Load ALL meshes
        for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
            const aiMesh* aiMesh = scene->mMeshes[meshIndex];
            std::cout << "Loading mesh " << meshIndex << ": " << aiMesh->mNumVertices
                << " vertices, " << aiMesh->mNumFaces << " faces" << std::endl;

            loadMesh(aiMesh, scene);
        }

        std::cout << "Successfully loaded " << meshes.size() << " meshes" << std::endl;
        std::cout << "Loaded " << loadedTextures.size() << " unique textures" << std::endl;
    }

    void loadMesh(const aiMesh* aiMesh, const aiScene* scene) {
        if (!aiMesh || aiMesh->mNumVertices == 0) {
            std::cout << "  Warning: Invalid or empty mesh!" << std::endl;
            return;
        }

        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<Texture> textures;

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

        // Load material textures
        if (aiMesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];

            // Load diffuse textures
            std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            // Load specular textures
            std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

            // Load normal maps
            std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "normal");
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

            // Also try aiTextureType_NORMALS for normal maps
            std::vector<Texture> normalMaps2 = loadMaterialTextures(material, aiTextureType_NORMALS, "normal");
            textures.insert(textures.end(), normalMaps2.begin(), normalMaps2.end());

            std::cout << "  Loaded " << textures.size() << " textures for this mesh" << std::endl;
        }

        // Create the mesh using your existing Mesh class
        meshes.emplace_back(vertices, indices, textures);

        std::cout << "  Mesh loaded successfully: " << vertices.size() << " vertices, "
            << indices.size() << " indices, " << textures.size() << " textures" << std::endl;
    }

    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName) {
        std::vector<Texture> textures;

        unsigned int textureCount = mat->GetTextureCount(type);
        std::cout << "    Found " << textureCount << " " << typeName << " textures" << std::endl;

        for (unsigned int i = 0; i < textureCount; i++) {
            aiString str;
            mat->GetTexture(type, i, &str);

            std::string texturePath = str.C_Str();
            std::cout << "      Original texture path: " << texturePath << std::endl;

            // Extract just the filename from the path (handles both absolute and relative paths)
            std::string filename = std::filesystem::path(texturePath).filename().string();
            std::cout << "      Extracted filename: " << filename << std::endl;

            // Try multiple possible locations for the texture
            std::vector<std::string> possiblePaths = {
                "textures/MapSchool/" + filename,     // models/MapSchool/textures/texture.jpg  
            };

            std::string fullPath;
            bool foundTexture = false;

            // Check each possible path
            for (const auto& path : possiblePaths) {
                if (std::filesystem::exists(path)) {
                    fullPath = path;
                    foundTexture = true;
                    std::cout << "      Found texture at: " << fullPath << std::endl;
                    break;
                }
            }

            if (!foundTexture) {
                std::cerr << "      Warning: Could not find texture file: " << filename << std::endl;
                std::cerr << "      Searched in:" << std::endl;
                for (const auto& path : possiblePaths) {
                    std::cerr << "        " << path << std::endl;
                }
                continue;
            }

            // Check if texture was already loaded
            if (loadedTextures.find(fullPath) != loadedTextures.end()) {
                std::cout << "      Using cached texture: " << fullPath << std::endl;
                textures.push_back(loadedTextures[fullPath]);
                continue;
            }

            // Try to load the texture
            try {
                std::cout << "      Loading new texture: " << fullPath << std::endl;

                // Create texture object with appropriate format
                GLenum format = GL_RGBA;
                GLenum pixelType = GL_UNSIGNED_BYTE;

                // Try to determine format from file extension
                std::string extension = std::filesystem::path(fullPath).extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                if (extension == ".jpg" || extension == ".jpeg") {
                    format = GL_RGB;
                }

                Texture texture(fullPath.c_str(), typeName.c_str(), i, format, pixelType);
                textures.push_back(texture);

                // Cache the texture
                loadedTextures[fullPath] = texture;

                std::cout << "      Successfully loaded texture: " << fullPath << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "      Failed to load texture " << fullPath << ": " << e.what() << std::endl;
            }
        }

        return textures;
    }
};
#endif