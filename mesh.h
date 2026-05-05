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
        
        // Position (Location 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Normal (Location 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // UV / TexCoords (Location 2)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        // Tangent (Location 3) - NEW!
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);
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