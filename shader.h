#pragma once
#include <glad/glad.h>
#include <iostream>
#include <string>
#include "math_core.h"


const char* irradianceVertexShader = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoords;
    out vec2 TexCoords;
    void main() {
        TexCoords = aTexCoords;
        gl_Position = vec4(aPos, 1.0);
    }
)glsl";

const char* irradianceFragmentShader = R"glsl(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoords;
    uniform sampler2D environmentMap;

    const float PI = 3.14159265359;

    vec3 SphericalToCartesian(vec2 uv) {
        float phi = uv.x * 2.0 * PI - PI;
        float theta = uv.y * PI - (PI / 2.0);
        return vec3(cos(theta)*cos(phi), sin(theta), cos(theta)*sin(phi));
    }
    vec2 CartesianToSpherical(vec3 v) {
        vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
        uv *= vec2(0.1591, 0.3183);
        return uv + 0.5;
    }

    void main() {
        vec3 N = normalize(SphericalToCartesian(TexCoords));
        vec3 irradiance = vec3(0.0);

        vec3 up    = vec3(0.0, 1.0, 0.0);
        vec3 right = normalize(cross(up, N));
        up         = normalize(cross(N, right));

        float sampleDelta = 0.1; // Smaller = higher quality bake, but slower
        float nrSamples = 0.0; 
        for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
            for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
                vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
                vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

                vec2 sampleUV = CartesianToSpherical(normalize(sampleVec));
                irradiance += texture(environmentMap, sampleUV).rgb * cos(theta) * sin(theta);
                nrSamples++;
            }
        }
        FragColor = vec4(PI * irradiance * (1.0 / float(nrSamples)), 1.0);
    }
)glsl";

const char* skyboxVertexShader = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;

    out vec3 WorldPos;

    uniform mat4 projection;
    uniform mat4 view;

    void main() {
        WorldPos = aPos;
        // Strip the translation from the view matrix so the skybox never moves when the player walks
        mat4 rotView = mat4(mat3(view)); 
        vec4 clipPos = projection * rotView * vec4(WorldPos, 1.0);
        
        // Force the Z depth to 1.0 (the maximum possible distance) so it always renders behind Sponza
        gl_Position = clipPos.xyww; 
    }
)glsl";

const char* skyboxFragmentShader = R"glsl(
    #version 330 core
    out vec4 FragColor;
    in vec3 WorldPos;

    uniform sampler2D equirectangularMap;

    // Inverse trigonometry to wrap a flat 2D image around a 3D sphere
    vec2 SampleSphericalMap(vec3 v) {
        vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
        uv *= vec2(0.1591, 0.3183); 
        uv += 0.5;
        return uv;
    }

    void main() {       
        vec2 uv = SampleSphericalMap(normalize(WorldPos));
        vec3 color = texture(equirectangularMap, uv).rgb;
        
        // Tone mapping (squash HDR values back into 0.0 - 1.0 for the monitor)
        color = color / (color + vec3(1.0));
        color = pow(color, vec3(1.0/2.2)); 
        
        FragColor = vec4(color, 1.0);
    }
)glsl";

const char* shadowVertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 lightSpaceMatrix;
    uniform mat4 model;
    void main() {
        gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
    }
)glsl";

const char* shadowFragmentShaderSource = R"glsl(
    #version 330 core
    void main() {
        // We output nothing! OpenGL automatically records the depth.
    }
)glsl";

// --- The GLSL Source Code ---
const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoords;
    layout (location = 3) in vec4 aTangent; // NEW

    out vec3 FragPos;
    out vec2 TexCoords;
    out vec3 Normal;
    out mat3 TBN; // Pass the matrix to the fragment shader!

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    // Add these two lines:
    uniform mat4 lightSpaceMatrix;
    out vec4 FragPosLightSpace;


    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
        TexCoords = aTexCoords;
        
        mat3 normalMatrix = mat3(transpose(inverse(model)));
        Normal = normalMatrix * aNormal;  

        // Calculate Tangent Space (TBN)
        vec3 T = normalize(normalMatrix * aTangent.xyz);
        vec3 N = normalize(Normal);
        // Gram-Schmidt orthogonalization (re-orthogonalize T with respect to N)
        T = normalize(T - dot(T, N) * N);
        // Calculate Bitangent
        vec3 B = cross(N, T) * aTangent.w;
        
        TBN = mat3(T, B, N);

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
    uniform vec3 sunDir;
    uniform vec3 sunColor;
    uniform vec3 viewPos;

    // Add these near your other uniforms:
    uniform sampler2D albedoMap;
    uniform int useTexture;

    uniform sampler2D metallicMap;
    uniform int useMetallicMap;
    
    uniform sampler2D roughnessMap;
    uniform int useRoughnessMap;

    uniform sampler2D normalMap;
    uniform int useNormalMap;
    
    uniform sampler2D shadowMap;
    in vec4 FragPosLightSpace;

    uniform sampler2D irradianceMap;

    in mat3 TBN; // Receive from vertex shader


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

    float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5; // Transform from [-1, 1] to [0, 1]
        if(projCoords.z > 1.0) return 0.0;
        
        float currentDepth = projCoords.z;
        // Bias prevents "shadow acne" (geometric self-shadowing artifacts)
        float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
        
        // Upgraded PCF (Soft Shadows)
        float shadow = 0.0;
        vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
        
        // Increase the sample radius to blur the blockiness
        int sampleRadius = 2; 
        float samples = 0.0;
        
        for(int x = -sampleRadius; x <= sampleRadius; ++x) {
            for(int y = -sampleRadius; y <= sampleRadius; ++y) {
                float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
                samples += 1.0;
            }    
        }
        return shadow / samples;
    }

    void main() {
        // Fallback to geometric normal
        vec3 N = normalize(Normal); 
        
        // If we have a normal map, override the Normal!
        if (useNormalMap == 1) {
            // Read the image (from 0 to 1)
            N = texture(normalMap, TexCoords).rgb;
            // Unpack it to standard vector space (-1 to 1)
            N = N * 2.0 - 1.0;
            // Bend it using the TBN matrix
            N = normalize(TBN * N); 
        }

        vec3 V = normalize(viewPos - FragPos);

        // 1. Get Base Color
        vec3 surfaceColor = albedo;
        if (useTexture == 1) {
            surfaceColor = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
        }

        // 2. Get Metallic Value
        float currentMetallic = metallic;
        if (useMetallicMap == 1) {
            currentMetallic = texture(metallicMap, TexCoords).r; // Just grab the red channel
        }

        // 3. Get Roughness Value
        float currentRoughness = roughness;
        if (useRoughnessMap == 1) {
            currentRoughness = texture(roughnessMap, TexCoords).r;
        }

        // Calculate base reflectivity
        vec3 F0 = vec3(0.04); 
        F0 = mix(F0, surfaceColor, currentMetallic);

        // Reflectance equation
        vec3 Lo = vec3(0.0);
        
        // Directional Light Calculation (Parallel rays, so we negate the sun's direction)
        vec3 L = normalize(-sunDir);
        vec3 H = normalize(V + L);
        
        // Sunlight has no attenuation! The radiance is constant everywhere.
        vec3 radiance = sunColor; 

        // Cook-Torrance BRDF (Remains exactly the same)
        float NDF = DistributionGGX(N, H, currentRoughness);   
        float G   = GeometrySmith(N, V, L, currentRoughness);      
        vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; 
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - currentMetallic; 

        float NdotL = max(dot(N, L), 0.0);        
        float shadow = ShadowCalculation(FragPosLightSpace, N, L);
        Lo += (1.0 - shadow) * (kD * surfaceColor / PI + specular) * radiance * NdotL;

        
        // Extract the ambient energy based on the metalness/fresnel
        vec3 kS_ambient = FresnelSchlick(max(dot(N, V), 0.0), F0);
        vec3 kD_ambient = 1.0 - kS_ambient;
        kD_ambient *= 1.0 - currentMetallic;

        // Inverse trig to map our 3D Normal Vector to the 2D Irradiance Map!
        vec2 irradianceUV = vec2(atan(N.z, N.x), asin(N.y));
        irradianceUV *= vec2(0.1591, 0.3183);
        irradianceUV += 0.5;
        
        vec3 irradiance = texture(irradianceMap, irradianceUV).rgb;
        vec3 diffuse = irradiance * surfaceColor;
        
        vec3 ambient = (kD_ambient * diffuse) * ao;
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

    Shader(const char* vCode = vertexShaderSource, const char* fCode = fragmentShaderSource) {
        unsigned int vertex, fragment;
        int success;
        char infoLog[512];

        // 1. Compile Vertex Shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vCode, NULL);
        glCompileShader(vertex);
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        // 2. Compile Fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fCode, NULL);
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
    
    void setInt(const std::string &name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }


    // Utility to pass custom colors
    void setVec3(const std::string &name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }
};