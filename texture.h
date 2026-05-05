#pragma once
#include <glad/glad.h>
#include <string>
#include <iostream>
#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class Texture {
public:
    unsigned int ID;

    Texture(const std::string& filepath) {
        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);

        // Wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Filtering parameters (Linear interpolation for smooth textures)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // OpenGL expects the 0.0 coordinate on the Y-axis to be at the bottom, images usually have it at the top.
        stbi_set_flip_vertically_on_load(true); 

        int width, height, nrChannels;
        unsigned char *data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 0);
        
        if (data) {
            GLenum format;
            if (nrChannels == 1) format = GL_RED;
            else if (nrChannels == 3) format = GL_RGB;
            else if (nrChannels == 4) format = GL_RGBA;

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            std::cerr << "[Radiance3D] ERROR: Failed to load texture -> " << filepath << "\n";
        }
        stbi_image_free(data);
    }

    void bind(unsigned int slot = 0) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, ID);
    }
};