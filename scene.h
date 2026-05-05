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
#include "obj_loader.h"
#include "gltf_loader.h" // Add this at the top


class Entity {
public:
    std::shared_ptr<Mesh> mesh;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale_vec;
    Vector3 color; // This will now act as our "Albedo"
    
    // --- NEW PBR PROPERTIES ---
    float metallic;
    float roughness;
    float ao; // Ambient Occlusion (Pre-baked shadows, default to 1.0)
    std::shared_ptr<Texture> albedoMap;
    std::shared_ptr<Texture> metallicMap;  // NEW
    std::shared_ptr<Texture> roughnessMap; // NEW
    std::shared_ptr<Texture> normalMap;
    
    Entity(std::shared_ptr<Mesh> m) 
        : mesh(m), position(0,0,0), rotation(0,0,0), scale_vec(1,1,1), 
          color(0.8f, 0.8f, 0.8f), metallic(0.0f), roughness(0.5f), ao(1.0f) {}

    Matrix4x4 get_model_matrix() const {
        Matrix4x4 t = Matrix4x4::translation(position);
        Matrix4x4 rx = Matrix4x4::rotateX(rotation.x);
        Matrix4x4 ry = Matrix4x4::rotateY(rotation.y);
        Matrix4x4 rz = Matrix4x4::rotateZ(rotation.z);
        Matrix4x4 s = Matrix4x4::scale(scale_vec);
        return t * rz * ry * rx * s;
    }
};


class Scene {
private:
    // The C++ memory vault: Maps a filepath (string) to a GPU Mesh (shared_ptr)
    std::unordered_map<std::string, std::shared_ptr<Mesh>> mesh_cache;

    // Internal helper to handle the loading logic
    std::shared_ptr<Mesh> get_or_load_mesh(const std::string& filepath) {
        if (mesh_cache.find(filepath) != mesh_cache.end()) {
            return mesh_cache[filepath];
        }

        std::cout << "[Radiance3D] Caching new geometry to GPU: " << filepath << "\n";
        
        std::vector<float> vertices;
        
        // Check the file extension to determine the loader!
        if (filepath.find(".gltf") != std::string::npos || filepath.find(".glb") != std::string::npos) {
            vertices = GltfLoader::load(filepath);
        } else {
            vertices = ObjLoader::load(filepath);
        }
        
        if (vertices.empty()) {
            std::cerr << "[Radiance3D] ERROR: Missing or corrupt file -> " << filepath << "\n";
            return nullptr;
        }

        auto mesh = std::make_shared<Mesh>();
        mesh->load_vertices(vertices);
        
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
            if (ent->albedoMap) {
                ent->albedoMap->bind(0); 
                shader.setInt("albedoMap", 0);
                shader.setInt("useTexture", 1); 
            } else {
                shader.setInt("useTexture", 0); 
            }

            // Metallic is on slot 1
            if (ent->metallicMap) {
                ent->metallicMap->bind(1);
                shader.setInt("metallicMap", 1);
                shader.setInt("useMetallicMap", 1);
            } else {
                shader.setInt("useMetallicMap", 0);
            }

            // Roughness is on slot 2
            if (ent->roughnessMap) {
                ent->roughnessMap->bind(2);
                shader.setInt("roughnessMap", 2);
                shader.setInt("useRoughnessMap", 1);
            } else {
                shader.setInt("useRoughnessMap", 0);
            }
            if (ent->normalMap) {
                ent->normalMap->bind(3);
                shader.setInt("normalMap", 3);
                shader.setInt("useNormalMap", 1);
            } else {
                shader.setInt("useNormalMap", 0);
            }
            shader.setMat4("model", ent->get_model_matrix());
            shader.setVec3("albedo", ent->color.x, ent->color.y, ent->color.z);
            
            // --- PASS PBR UNIFORMS ---
            shader.setFloat("metallic", ent->metallic);
            shader.setFloat("roughness", ent->roughness);
            shader.setFloat("ao", ent->ao);

            

            ent->mesh->draw();
        }
    }
};