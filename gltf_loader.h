#pragma once
// This macro tells the compiler to compile the C implementation inside the header
#define CGLTF_IMPLEMENTATION 
#include "cgltf.h"
#include <vector>
#include <string>
#include <iostream>

class GltfLoader {
public:
    static std::vector<float> load(const std::string& filepath) {
        std::vector<float> final_vertices;
        
        cgltf_options options = {};
        cgltf_data* data = NULL;
        
        // Parse the JSON structure and load the binary buffers
        cgltf_result result = cgltf_parse_file(&options, filepath.c_str(), &data);
        if (result != cgltf_result_success) {
            std::cerr << "[Radiance3D] ERROR: Could not parse glTF -> " << filepath << "\n";
            return final_vertices;
        }
        cgltf_load_buffers(&options, data, filepath.c_str());

        // For now, we just grab the very first mesh in the file
        if (data->meshes_count == 0) {
            cgltf_free(data);
            return final_vertices;
        }
        
        cgltf_primitive* primitive = &data->meshes[0].primitives[0];
        
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

        // Extract the data into our flat std::vector format
        if (pos_accessor) {
            // CHECK: Does this file use an Index Buffer?
            if (primitive->indices) {
                // UNROLL the vertices using the indices
                for (cgltf_size i = 0; i < primitive->indices->count; ++i) {
                    cgltf_size vertex_index = cgltf_accessor_read_index(primitive->indices, i);

                    // Read Position at this exact index
                    float pos[3] = {0};
                    cgltf_accessor_read_float(pos_accessor, vertex_index, pos, 3);
                    final_vertices.push_back(pos[0]);
                    final_vertices.push_back(pos[1]);
                    final_vertices.push_back(pos[2]);

                    // Read Normal at this exact index
                    float norm[3] = {0.0f, 1.0f, 0.0f}; 
                    if (norm_accessor) {
                        cgltf_accessor_read_float(norm_accessor, vertex_index, norm, 3);
                    }
                    final_vertices.push_back(norm[0]);
                    final_vertices.push_back(norm[1]);
                    final_vertices.push_back(norm[2]);

                    float uv[2] = {0.0f, 0.0f};
                    if (uv_accessor) {
                        cgltf_accessor_read_float(uv_accessor, vertex_index /* or 'i' for unindexed */, uv, 2);
                    }
                    final_vertices.push_back(uv[0]);
                    final_vertices.push_back(uv[1]);

                    // ... after pushing the 2 UV floats:
                    float tangent[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // Default flat tangent
                    if (tangent_accessor) {
                        cgltf_accessor_read_float(tangent_accessor, vertex_index /* or 'i' */, tangent, 4);
                    }
                    final_vertices.push_back(tangent[0]);
                    final_vertices.push_back(tangent[1]);
                    final_vertices.push_back(tangent[2]);
                    final_vertices.push_back(tangent[3]);
                }
            } else {
                // FALLBACK: The file is already unrolled (rare, but happens)
                for (cgltf_size i = 0; i < pos_accessor->count; ++i) {
                    float pos[3] = {0};
                    cgltf_accessor_read_float(pos_accessor, i, pos, 3);
                    final_vertices.push_back(pos[0]);
                    final_vertices.push_back(pos[1]);
                    final_vertices.push_back(pos[2]);

                    float norm[3] = {0.0f, 1.0f, 0.0f}; 
                    if (norm_accessor) {
                        cgltf_accessor_read_float(norm_accessor, i, norm, 3);
                    }
                    final_vertices.push_back(norm[0]);
                    final_vertices.push_back(norm[1]);
                    final_vertices.push_back(norm[2]);


                    float uv[2] = {0.0f, 0.0f};
                    if (uv_accessor) {
                        cgltf_accessor_read_float(uv_accessor, i, uv, 2);
                    }
                    final_vertices.push_back(uv[0]);
                    final_vertices.push_back(uv[1]);

                    float tangent[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // Default flat tangent
                    if (tangent_accessor) {
                        cgltf_accessor_read_float(tangent_accessor, i, tangent, 4);
                    }
                    final_vertices.push_back(tangent[0]);
                    final_vertices.push_back(tangent[1]);
                    final_vertices.push_back(tangent[2]);
                    final_vertices.push_back(tangent[3]);
                }
            }
        }
        // Clean up memory allocated by the library
        cgltf_free(data);
        return final_vertices;
    }
};