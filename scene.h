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
    struct SubMesh {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Texture> albedoMap, normalMap, metallicMap, roughnessMap;
        bool isPackedGLTF = false;
    };
    std::vector<SubMesh> submeshes;
    std::shared_ptr<class PhysicsCollider> physics_body; // Forward declaration
    
    Vector3 position, rotation, scale_vec, color;
    float metallic, roughness, ao;
    
    bool use_physics_transform = false;
    float physics_rotation[9]; // Bullet's 3x3 rotation matrix, row-major
    
    Entity() : position(0,0,0), rotation(0,0,0), scale_vec(1,1,1), color(0.8f, 0.8f, 0.8f), metallic(0.0f), roughness(0.5f), ao(1.0f) {}

    Matrix4x4 get_model_matrix() const {
        Matrix4x4 t = Matrix4x4::translation(position);
        Matrix4x4 s = Matrix4x4::scale(scale_vec);
        Matrix4x4 r;
        if (use_physics_transform) {
            // Directly transplant Bullet's 3x3 into your 4x4 (row-major)
            r.m[0][0]=physics_rotation[0]; r.m[0][1]=physics_rotation[1]; r.m[0][2]=physics_rotation[2];
            r.m[1][0]=physics_rotation[3]; r.m[1][1]=physics_rotation[4]; r.m[1][2]=physics_rotation[5];
            r.m[2][0]=physics_rotation[6]; r.m[2][1]=physics_rotation[7]; r.m[2][2]=physics_rotation[8];
        } else {
            r = Matrix4x4::rotateZ(rotation.z) * Matrix4x4::rotateY(rotation.y) * Matrix4x4::rotateX(rotation.x);
        }
        return t * r * s;
    }
};

class Scene {
private:
    std::unordered_map<std::string, std::vector<Entity::SubMesh>> asset_cache;
    
    std::vector<Entity::SubMesh> get_or_load_asset(const std::string& filepath) {
        if (asset_cache.find(filepath) != asset_cache.end()) return asset_cache[filepath];
        std::vector<Entity::SubMesh> submeshes;
        
        if (filepath.find(".gltf") != std::string::npos || filepath.find(".glb") != std::string::npos) {
            for (auto& data : GltfLoader::load(filepath)) {
                Entity::SubMesh sub; sub.mesh = std::make_shared<Mesh>(); sub.mesh->load_vertices(data.vertices);
                sub.albedoMap = data.albedoMap; sub.normalMap = data.normalMap;
                if (data.metallicRoughnessMap) { sub.metallicMap = sub.roughnessMap = data.metallicRoughnessMap; sub.isPackedGLTF = true; }
                submeshes.push_back(sub);
            }
        } // (Keep your OBJ fallback here if needed)
        return asset_cache[filepath] = submeshes;
    }
public:
    std::vector<std::shared_ptr<Entity>> entities;
    
    std::shared_ptr<Entity> add_entity(const std::string& filepath) {
        auto ent = std::make_shared<Entity>(); ent->submeshes = get_or_load_asset(filepath);
        entities.push_back(ent); return ent;
    }
    
    void draw(Shader& shader) {
        for (auto& ent : entities) {
            shader.setMat4("model", ent->get_model_matrix());
            shader.setVec3("albedo", ent->color.x, ent->color.y, ent->color.z);
            shader.setFloat("metallic", ent->metallic); shader.setFloat("roughness", ent->roughness); shader.setFloat("ao", ent->ao);
            
            for (auto& sub : ent->submeshes) {
                if (sub.albedoMap) { sub.albedoMap->bind(0); shader.setInt("albedoMap", 0); shader.setInt("useTexture", 1); } else shader.setInt("useTexture", 0);
                if (sub.metallicMap) { sub.metallicMap->bind(1); shader.setInt("metallicMap", 1); shader.setInt("useMetallicMap", 1); sub.roughnessMap->bind(2); shader.setInt("roughnessMap", 2); shader.setInt("useRoughnessMap", 1); shader.setInt("isPackedGLTF", sub.isPackedGLTF ? 1 : 0); } else { shader.setInt("useMetallicMap", 0); shader.setInt("useRoughnessMap", 0); shader.setInt("isPackedGLTF", 0); }
                if (sub.normalMap) { sub.normalMap->bind(3); shader.setInt("normalMap", 3); shader.setInt("useNormalMap", 1); } else shader.setInt("useNormalMap", 0);
                sub.mesh->draw();
            }
        }
    }
};