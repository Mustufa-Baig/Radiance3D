#pragma once
// This macro tells the compiler to compile the C implementation inside the header
#define CGLTF_IMPLEMENTATION 
#include "cgltf.h"
#include <vector>
#include <string>
#include <iostream>

struct GltfData {
    std::vector<float> vertices;
    std::shared_ptr<Texture> albedoMap;
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> metallicRoughnessMap;
};

class GltfLoader {
public:
    static std::shared_ptr<Texture> extract_texture(cgltf_texture_view& view) {
        if (view.texture && view.texture->image) {
            cgltf_image* img = view.texture->image;
            if (img->buffer_view) {
                const unsigned char* buffer = (const unsigned char*)img->buffer_view->buffer->data + img->buffer_view->offset;
                return std::make_shared<Texture>(buffer, (int)img->buffer_view->size);
            }
        }
        return nullptr;
    }

    static std::vector<GltfData> load(const std::string& filepath) {
        std::vector<GltfData> all_primitives;
        cgltf_options options = {}; cgltf_data* data = NULL;
        if (cgltf_parse_file(&options, filepath.c_str(), &data) != cgltf_result_success) return all_primitives;
        cgltf_load_buffers(&options, data, filepath.c_str());

        for (cgltf_size m = 0; m < data->meshes_count; ++m) {
            for (cgltf_size p = 0; p < data->meshes[m].primitives_count; ++p) {
                cgltf_primitive* primitive = &data->meshes[m].primitives[p];
                GltfData result; std::vector<float> final_vertices;
                
                
                cgltf_accessor* pos_accessor = NULL;
                cgltf_accessor* norm_accessor = NULL;
                cgltf_accessor* uv_accessor = NULL; 
                cgltf_accessor* tangent_accessor = NULL; // NEW

                for (cgltf_size i = 0; i < primitive->attributes_count; ++i) {
                    if (primitive->attributes[i].type == cgltf_attribute_type_position) {
                        pos_accessor = primitive->attributes[i].data;
                    } else if (primitive->attributes[i].type == cgltf_attribute_type_normal) {
                        norm_accessor = primitive->attributes[i].data;
                    } else if (primitive->attributes[i].type == cgltf_attribute_type_texcoord) {
                        uv_accessor = primitive->attributes[i].data;
                    } else if (primitive->attributes[i].type == cgltf_attribute_type_tangent) {
                        tangent_accessor = primitive->attributes[i].data; // NEW
                    }
                }

                // --- UPGRADED GEOMETRY EXTRACTION ---
                cgltf_accessor* index_accessor = primitive->indices;
                // If indices exist, loop through them. Otherwise, loop through raw vertices.
                cgltf_size total_vertices_to_draw = index_accessor ? index_accessor->count : pos_accessor->count;

                for (cgltf_size i = 0; i < total_vertices_to_draw; ++i) {
                    
                    // 1. Find the true index of the vertex we need to read
                    cgltf_size v_idx = i; 
                    if (index_accessor) {
                        v_idx = cgltf_accessor_read_index(index_accessor, i);
                    }

                    // 2. Read all attributes using 'v_idx' instead of 'i'
                    float pos[3] = {0.0f, 0.0f, 0.0f};
                    if (pos_accessor) cgltf_accessor_read_float(pos_accessor, v_idx, pos, 3);
                    final_vertices.push_back(pos[0]);
                    final_vertices.push_back(pos[1]);
                    final_vertices.push_back(pos[2]);

                    float norm[3] = {0.0f, 1.0f, 0.0f}; 
                    if (norm_accessor) cgltf_accessor_read_float(norm_accessor, v_idx, norm, 3);
                    final_vertices.push_back(norm[0]);
                    final_vertices.push_back(norm[1]);
                    final_vertices.push_back(norm[2]);

                    float uv[2] = {0.0f, 0.0f};
                    if (uv_accessor) cgltf_accessor_read_float(uv_accessor, v_idx, uv, 2);
                    final_vertices.push_back(uv[0]);
                    final_vertices.push_back(uv[1]);

                    float tangent[4] = {0.0f, 0.0f, 0.0f, 1.0f}; 
                    if (tangent_accessor) cgltf_accessor_read_float(tangent_accessor, v_idx, tangent, 4);
                    final_vertices.push_back(tangent[0]);
                    final_vertices.push_back(tangent[1]);
                    final_vertices.push_back(tangent[2]);
                    final_vertices.push_back(tangent[3]);
                }

                result.vertices = final_vertices;
                if (primitive->material) {
                    if (primitive->material->has_pbr_metallic_roughness) {
                        result.albedoMap = extract_texture(primitive->material->pbr_metallic_roughness.base_color_texture);
                        result.metallicRoughnessMap = extract_texture(primitive->material->pbr_metallic_roughness.metallic_roughness_texture);
                    }
                    result.normalMap = extract_texture(primitive->material->normal_texture);
                }
                all_primitives.push_back(result);
            }
        }
        cgltf_free(data); return all_primitives;
    }
};