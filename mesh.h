#pragma once
#include <glad/glad.h>
#include <vector>

class Mesh {
private:
    unsigned int VAO, VBO;
    int vertexCount;

public:
    Mesh() : vertexCount(0) {
        // Generate the GPU buffers
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
    }

    // Takes a raw list of floats from Python (x,y,z, x,y,z...)
    void load_vertices(const std::vector<float>& vertices) {
        vertexCount = vertices.size() / 6; // Now 6 floats per vertex!

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Attribute 0: Position (X, Y, Z) - Read 3 floats, skip the next 3
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Attribute 1: Normal (Nx, Ny, Nz) - Start reading after the first 3 floats
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void draw() {
        if (vertexCount == 0) return;
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);
    }
    
    ~Mesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
};