#pragma once
#include <vector>
#include <memory>
#include <unordered_map> // For our asset cache
#include <string>
#include <iostream>
#include "math_core.h"
#include "mesh.h"
#include "shader.h"
#include "obj_loader.h"  // Needed to load files directly from the Scene

class Entity {
public:
    std::shared_ptr<Mesh> mesh;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale_vec;
    Vector3 color;

    // Initialize with default values
    Entity(std::shared_ptr<Mesh> m) 
        : mesh(m), position(0,0,0), rotation(0,0,0), scale_vec(1,1,1), color(0.8f, 0.8f, 0.8f) {}

    Matrix4x4 get_model_matrix() const {
        Matrix4x4 t = Matrix4x4::translation(position);
        Matrix4x4 rx = Matrix4x4::rotateX(rotation.x);
        Matrix4x4 ry = Matrix4x4::rotateY(rotation.y);
        Matrix4x4 rz = Matrix4x4::rotateZ(rotation.z);
        Matrix4x4 s = Matrix4x4::scale(scale_vec);
        
        // Use the native C++ overloaded '*' operator
        return t * rz * ry * rx * s;
    }
};

class Scene {
private:
    // The C++ memory vault: Maps a filepath (string) to a GPU Mesh (shared_ptr)
    std::unordered_map<std::string, std::shared_ptr<Mesh>> mesh_cache;

    // Internal helper to handle the loading logic
    std::shared_ptr<Mesh> get_or_load_mesh(const std::string& filepath) {
        // 1. If it exists in the vault, return the cached memory
        if (mesh_cache.find(filepath) != mesh_cache.end()) {
            return mesh_cache[filepath];
        }

        // 2. If it's new, load it from the hard drive
        std::cout << "[Radiance3D] Caching new geometry to GPU: " << filepath << "\n";
        std::vector<float> vertices = ObjLoader::load(filepath);
        
        if (vertices.empty()) {
            std::cerr << "[Radiance3D] ERROR: Missing file -> " << filepath << "\n";
            return nullptr;
        }

        auto mesh = std::make_shared<Mesh>();
        mesh->load_vertices(vertices);
        
        // 3. Save it to the vault and return it
        mesh_cache[filepath] = mesh;
        return mesh;
    }

public:
    std::vector<std::shared_ptr<Entity>> entities;

    // NEW API: Python just passes a string, C++ handles all the heavy lifting!
    std::shared_ptr<Entity> add_entity(const std::string& filepath) {
        auto mesh = get_or_load_mesh(filepath);
        if (!mesh) return nullptr; // Failsafe if file doesn't exist

        auto ent = std::make_shared<Entity>(mesh);
        entities.push_back(ent);
        return ent;
    }

    void draw(Shader& shader) {
        for (auto& ent : entities) {
            shader.setMat4("model", ent->get_model_matrix());
            shader.setVec3("objectColor", ent->color.x, ent->color.y, ent->color.z);
            ent->mesh->draw();
        }
    }
};