#pragma once
#include <glad/glad.h>
#include <string>
#include <iostream>
#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class HDRTexture {
public:
    unsigned int ID;

    HDRTexture(const std::string& filepath) {
        stbi_set_flip_vertically_on_load(true);
        int width, height, nrComponents;
        
        // stbi_loadf loads floating-point pixel data!
        float *data = stbi_loadf(filepath.c_str(), &width, &height, &nrComponents, 0);
        
        if (data) {
            glGenTextures(1, &ID);
            glBindTexture(GL_TEXTURE_2D, ID);
            
            // Note the GL_RGB16F and GL_FLOAT here!
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            stbi_image_free(data);
        } else {
            std::cerr << "[Radiance3D] ERROR: Failed to load HDR -> " << filepath << "\n";
        }
    }

    void bind(unsigned int slot = 0) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, ID);
    }
};
class Texture {
public:
    unsigned int ID;

    // --- CONSTRUCTOR 1: The Original Filepath Loader ---
    // (Keeps your C++ bindings happy and allows manual loading if you ever need it)
    Texture(const std::string& filepath) {
        stbi_set_flip_vertically_on_load(true); 

        int width, height, nrChannels;
        unsigned char *data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 0);
        
        if (data) {
            GLenum format = GL_RGB;
            if (nrChannels == 1) format = GL_RED;
            else if (nrChannels == 4) format = GL_RGBA;

            glGenTextures(1, &ID);
            glBindTexture(GL_TEXTURE_2D, ID);
            
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            stbi_image_free(data);
        } else {
            std::cerr << "[Radiance3D] ERROR: Failed to load texture -> " << filepath << "\n";
        }
    }

    // --- CONSTRUCTOR 2: The New glTF Memory Loader ---
    // (Used by GltfLoader to extract embedded binary images)
    Texture(const unsigned char* buffer, int len) {
        stbi_set_flip_vertically_on_load(false); // glTF is usually pre-flipped
        
        int width, height, nrChannels;
        unsigned char *data = stbi_load_from_memory(buffer, len, &width, &height, &nrChannels, 0);
        
        if (data) {
            GLenum format = GL_RGB;
            if (nrChannels == 1) format = GL_RED;
            else if (nrChannels == 4) format = GL_RGBA;

            glGenTextures(1, &ID);
            glBindTexture(GL_TEXTURE_2D, ID);
            
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            stbi_image_free(data);
        } else {
            std::cerr << "[Radiance3D] ERROR: Failed to decode embedded glTF texture!\n";
        }
    }

    void bind(unsigned int slot = 0) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, ID);
    }
};