#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "math_core.h" // To use Vector3

class ObjLoader {
public:
    static std::vector<float> load(const std::string& filepath) {
        std::vector<float> final_vertices;
        std::vector<Vector3> temp_vertices;
        std::vector<Vector3> temp_normals;

        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Radiance3D ERROR: Could not open OBJ file -> " << filepath << "\n";
            return final_vertices;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v") {
                float x, y, z;
                iss >> x >> y >> z;
                temp_vertices.push_back(Vector3(x, y, z));
            } 
            else if (prefix == "vn") {
                float x, y, z;
                iss >> x >> y >> z;
                temp_normals.push_back(Vector3(x, y, z));
            } 
            else if (prefix == "f") {
                // Assuming triangulated faces (3 corners per face)
                std::string vertexData[3];
                iss >> vertexData[0] >> vertexData[1] >> vertexData[2];

                for (int i = 0; i < 3; ++i) {
                    int vIdx = 0, vtIdx = 0, vnIdx = 0;
                    
                    // Parse "v//vn" (no textures) or "v/vt/vn" (with textures)
                    if (vertexData[i].find("//") != std::string::npos) {
                        sscanf(vertexData[i].c_str(), "%d//%d", &vIdx, &vnIdx);
                    } else {
                        sscanf(vertexData[i].c_str(), "%d/%d/%d", &vIdx, &vtIdx, &vnIdx);
                    }

                    // OBJ indices start at 1, so we subtract 1 for C++ arrays
                    Vector3 vertex = temp_vertices[vIdx - 1];
                    Vector3 normal = temp_normals[vnIdx - 1];

                    // Push Position
                    final_vertices.push_back(vertex.x);
                    final_vertices.push_back(vertex.y);
                    final_vertices.push_back(vertex.z);
                    // Push Normal
                    final_vertices.push_back(normal.x);
                    final_vertices.push_back(normal.y);
                    final_vertices.push_back(normal.z);
                }
            }
        }
        return final_vertices;
    }
};