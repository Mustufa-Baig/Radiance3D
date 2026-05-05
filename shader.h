#pragma once
#include <glad/glad.h>
#include <iostream>
#include <string>
#include "math_core.h"

// --- The GLSL Source Code ---
const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    
    out vec3 FragPos;
    out vec3 Normal;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    void main() {
        // Calculate the actual position of the vertex in the 3D world
        FragPos = vec3(model * vec4(aPos, 1.0));
        
        // Pass the normal to the fragment shader (adjusting for object rotation)
        Normal = mat3(model) * aNormal;  
        
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)glsl";

const char* fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoords;
    in vec3 FragPos;
    in vec3 Normal;

    // --- PBR Parameters ---
    uniform vec3 albedo;
    uniform float metallic;
    uniform float roughness;
    uniform float ao;

    // --- Lighting ---
    uniform vec3 lightPos;
    uniform vec3 lightColor;
    uniform vec3 viewPos;

    const float PI = 3.14159265359;

    // Trowbridge-Reitz GGX (Calculates microfacet alignment)
    float DistributionGGX(vec3 N, vec3 H, float roughness) {
        float a = roughness*roughness;
        float a2 = a*a;
        float NdotH = max(dot(N, H), 0.0);
        float NdotH2 = NdotH*NdotH;
        float nom   = a2;
        float denom = (NdotH2 * (a2 - 1.0) + 1.0);
        denom = PI * denom * denom;
        return nom / max(denom, 0.0000001); 
    }

    // Schlick-GGX (Calculates self-shadowing of microfacets)
    float GeometrySchlickGGX(float NdotV, float roughness) {
        float r = (roughness + 1.0);
        float k = (r*r) / 8.0;
        float nom   = NdotV;
        float denom = NdotV * (1.0 - k) + k;
        return nom / denom;
    }

    float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
        float NdotV = max(dot(N, V), 0.0);
        float NdotL = max(dot(N, L), 0.0);
        float ggx2 = GeometrySchlickGGX(NdotV, roughness);
        float ggx1 = GeometrySchlickGGX(NdotL, roughness);
        return ggx1 * ggx2;
    }

    // Fresnel-Schlick (Calculates base reflectivity at grazing angles)
    vec3 FresnelSchlick(float cosTheta, vec3 F0) {
        return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    }

    void main() {
        vec3 N = normalize(Normal);
        vec3 V = normalize(viewPos - FragPos);

        // Calculate base reflectivity. (Metals tint reflections with their albedo, non-metals use a flat 0.04)
        vec3 F0 = vec3(0.04); 
        F0 = mix(F0, albedo, metallic);

        // Reflectance equation
        vec3 Lo = vec3(0.0);
        
        // Single Light Calculation (Can be looped for multiple lights later)
        vec3 L = normalize(lightPos - FragPos);
        vec3 H = normalize(V + L);
        
        // Attenuation (Light falls off over distance)
        float distance = length(lightPos - FragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColor * attenuation * 100.0; // Multiplied by 100 for dramatic brightness

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        // Energy conservation: diffuse and specular cannot exceed 1.0
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic; // Metals absorb all refracted light (no diffuse)

        // Scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;

        // Ambient lighting (very simple for now)
        vec3 ambient = vec3(0.03) * albedo * ao;
        vec3 color = ambient + Lo;

        // HDR Tonemapping and Gamma Correction
        color = color / (color + vec3(1.0));
        color = pow(color, vec3(1.0/2.2)); 

        FragColor = vec4(color, 1.0);
    }
)glsl";
// --- The Shader Compiler Class ---
class Shader {
public:
    unsigned int ID;

    Shader() {
        unsigned int vertex, fragment;
        int success;
        char infoLog[512];

        // 1. Compile Vertex Shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertexShaderSource, NULL);
        glCompileShader(vertex);
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        // 2. Compile Fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragment);
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        // 3. Link Shaders into a Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }

        // 4. Clean up individual shaders (they are compiled into the program now)
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() { glUseProgram(ID); }

    // Utility to pass our custom Math matrices to the GPU
    void setMat4(const std::string &name, const Matrix4x4 &mat) const {
        // GL_TRUE tells OpenGL to transpose our row-major C++ matrix into column-major GLSL format
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_TRUE, &mat.m[0][0]);
    }

    void setFloat(const std::string &name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    
    // Utility to pass custom colors
    void setVec3(const std::string &name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }
};