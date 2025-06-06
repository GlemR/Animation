#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H
#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>
#include <iostream>

struct Mesh {
    GLuint VAO = 0, VBO = 0, EBO = 0;
    size_t indexCount = 0;
    bool initialized = false;

    // Disable copy constructor and assignment to prevent double-deletion
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Enable move constructor and assignment
    Mesh() = default;

    Mesh(Mesh&& other) noexcept {
        VAO = other.VAO;
        VBO = other.VBO;
        EBO = other.EBO;
        indexCount = other.indexCount;
        initialized = other.initialized;

        // Reset the moved-from object
        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
        other.indexCount = 0;
        other.initialized = false;
    }

    Mesh& operator=(Mesh&& other) noexcept {
        if (this != &other) {
            cleanup();

            VAO = other.VAO;
            VBO = other.VBO;
            EBO = other.EBO;
            indexCount = other.indexCount;
            initialized = other.initialized;

            other.VAO = 0;
            other.VBO = 0;
            other.EBO = 0;
            other.indexCount = 0;
            other.initialized = false;
        }
        return *this;
    }

    ~Mesh() {
        cleanup();
    }

private:
    void cleanup() {
        if (initialized) {
            if (VAO != 0) {
                glDeleteVertexArrays(1, &VAO);
                VAO = 0;
            }
            if (VBO != 0) {
                glDeleteBuffers(1, &VBO);
                VBO = 0;
            }
            if (EBO != 0) {
                glDeleteBuffers(1, &EBO);
                EBO = 0;
            }
            initialized = false;
        }
    }
};

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

    void Draw() {
        for (auto& mesh : meshes) {
            if (mesh.indexCount > 0 && mesh.initialized) {
                glBindVertexArray(mesh.VAO);
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indexCount), GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
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

        // Reserve space to avoid reallocations that could cause issues
        meshes.reserve(scene->mNumMeshes);

        // Load ALL meshes
        for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
            const aiMesh* aiMesh = scene->mMeshes[meshIndex];
            std::cout << "Loading mesh " << meshIndex << ": " << aiMesh->mNumVertices
                << " vertices, " << aiMesh->mNumFaces << " faces" << std::endl;

            Mesh mesh;
            if (loadMesh(aiMesh, mesh)) {
                meshes.emplace_back(std::move(mesh));
            }
        }

        std::cout << "Successfully loaded " << meshes.size() << " meshes" << std::endl;
    }

    bool loadMesh(const aiMesh* aiMesh, Mesh& mesh) {
        if (!aiMesh || aiMesh->mNumVertices == 0) {
            std::cout << "  Warning: Invalid or empty mesh!" << std::endl;
            return false;
        }

        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        // Reserve space for better performance
        vertices.reserve(aiMesh->mNumVertices * 8); // 8 floats per vertex

        // Track bounding box for this mesh
        float minX = FLT_MAX, maxX = -FLT_MAX;
        float minY = FLT_MAX, maxY = -FLT_MAX;
        float minZ = FLT_MAX, maxZ = -FLT_MAX;

        for (unsigned int i = 0; i < aiMesh->mNumVertices; ++i) {
            // Position
            float x = aiMesh->mVertices[i].x;
            float y = aiMesh->mVertices[i].y;
            float z = aiMesh->mVertices[i].z;

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // Update bounding box
            minX = std::min(minX, x); maxX = std::max(maxX, x);
            minY = std::min(minY, y); maxY = std::max(maxY, y);
            minZ = std::min(minZ, z); maxZ = std::max(maxZ, z);

            // Normal
            if (aiMesh->HasNormals()) {
                vertices.push_back(aiMesh->mNormals[i].x);
                vertices.push_back(aiMesh->mNormals[i].y);
                vertices.push_back(aiMesh->mNormals[i].z);
            }
            else {
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
                vertices.push_back(1.0f);
            }

            // TexCoords
            if (aiMesh->HasTextureCoords(0)) {
                vertices.push_back(aiMesh->mTextureCoords[0][i].x);
                vertices.push_back(aiMesh->mTextureCoords[0][i].y);
            }
            else {
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
            }
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

        mesh.indexCount = indices.size();

        if (mesh.indexCount == 0) {
            std::cout << "  Warning: Mesh has no valid triangular faces!" << std::endl;
            return false;
        }

        // Generate OpenGL objects
        glGenVertexArrays(1, &mesh.VAO);
        glGenBuffers(1, &mesh.VBO);
        glGenBuffers(1, &mesh.EBO);

        if (mesh.VAO == 0 || mesh.VBO == 0 || mesh.EBO == 0) {
            std::cerr << "  Error: Failed to generate OpenGL objects!" << std::endl;
            return false;
        }

        glBindVertexArray(mesh.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // layout: pos (3), normal (3), tex (2) = 8 floats per vertex
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "  OpenGL Error during mesh setup: " << error << std::endl;
            return false;
        }

        mesh.initialized = true;

        std::cout << "  Mesh loaded successfully: " << vertices.size() / 8 << " vertices, "
            << mesh.indexCount << " indices" << std::endl;

        return true;
    }
};
#endif