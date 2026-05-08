#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <memory>
#include <chrono>

#include "math_core.h"
#include "shader.h"
#include "texture.h"
#include "scene.h"

namespace py = pybind11;


unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube() {
    if (cubeVAO == 0) {
        float vertices[] = {
            // Back, Front, Left, Right, Bottom, Top faces (36 vertices total)
            -1, -1, -1,   1,  1, -1,   1, -1, -1,   1,  1, -1,  -1, -1, -1,  -1,  1, -1,
            -1, -1,  1,   1, -1,  1,   1,  1,  1,   1,  1,  1,  -1,  1,  1,  -1, -1,  1,
            -1,  1,  1,  -1,  1, -1,  -1, -1, -1,  -1, -1, -1,  -1, -1,  1,  -1,  1,  1,
             1,  1,  1,   1, -1, -1,   1,  1, -1,   1, -1, -1,   1,  1,  1,   1, -1,  1,
            -1, -1, -1,   1, -1, -1,   1, -1,  1,   1, -1,  1,  -1, -1,  1,  -1, -1, -1,
            -1,  1, -1,   1,  1,  1,   1,  1, -1,   1,  1,  1,  -1,  1, -1,  -1,  1,  1
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    }
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int quadVAO = 0;
unsigned int quadVBO = 0;
void renderQuad() {
    if (quadVAO == 0) {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// --- GLOBAL ENGINE STATE ---
struct EngineState {
    GLFWwindow* window = nullptr;
    std::unique_ptr<Shader> shader;
    std::unique_ptr<Scene> scene;
    std::unique_ptr<Shader> shadow_shader;

    std::unique_ptr<Shader> skybox_shader;
    std::unique_ptr<Shader> irradiance_shader;
    
    std::shared_ptr<HDRTexture> skybox_hdr;

    unsigned int depthMapFBO;
    unsigned int depthMap;
    unsigned int irradianceMap;
    const unsigned int SHADOW_RES = 4096; // 4K shadows for Sponza!


    // Camera State
    Vector3 camera_pos = Vector3(0.0f, 0.0f, 5.0f);
    Vector3 camera_front = Vector3(0.0f, 0.0f, -1.0f);
    Vector3 world_up = Vector3(0.0f, 1.0f, 0.0f);
    
    float yaw = -90.0f;
    float pitch = 0.0f;
    float last_x = 400.0f, last_y = 300.0f;
    bool first_mouse = true;
    bool fps_camera_enabled = true;

    Vector3 sun_dir = Vector3(-0.2f, -1.0f, -0.3f); // Pointing slightly angled down
    Vector3 sun_color = Vector3(5.0f, 5.0f, 4.5f);  // Bright, slightly warm light

    // Timing
    std::chrono::high_resolution_clock::time_point last_time;

    int width = 800, height = 600;
} state;

std::unordered_map<std::string, std::shared_ptr<Texture>> texture_cache;

// --- CALLBACKS ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    state.width = width;
    state.height = height;
    glViewport(0, 0, width, height);
}

// --- GLOBAL API FUNCTIONS ---
bool init_window(int width, int height, const std::string& title) {
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    state.window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!state.window) { glfwTerminate(); return false; }

    glfwMakeContextCurrent(state.window);
    glfwSetFramebufferSizeCallback(state.window, framebuffer_size_callback);
    glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Trap mouse by default

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return false;

    glEnable(GL_DEPTH_TEST);

    state.width = width;
    state.height = height;
    state.scene = std::make_unique<Scene>();
    state.shader = std::make_unique<Shader>();
    state.last_time = std::chrono::high_resolution_clock::now();


    state.shadow_shader = std::make_unique<Shader>(shadowVertexShaderSource, shadowFragmentShaderSource);
    state.skybox_shader = std::make_unique<Shader>(skyboxVertexShader, skyboxFragmentShader);
    
    glGenFramebuffers(1, &state.depthMapFBO);
    glGenTextures(1, &state.depthMap);
    glBindTexture(GL_TEXTURE_2D, state.depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, state.SHADOW_RES, state.SHADOW_RES, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    glBindFramebuffer(GL_FRAMEBUFFER, state.depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, state.depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

std::shared_ptr<Entity> load_model(const std::string& filepath) {
    if (!state.scene) return nullptr;
    return state.scene->add_entity(filepath);
}

void set_albedo_texture(std::shared_ptr<Entity> ent, const std::string& filepath) {
    if (!ent) return;
    if (texture_cache.find(filepath) == texture_cache.end()) {
        texture_cache[filepath] = std::make_shared<Texture>(filepath);
    }
    for(auto& sub : ent->submeshes) { sub.albedoMap = texture_cache[filepath]; }
}


void load_skybox(const std::string& hdr_filepath) {
    state.skybox_hdr = std::make_shared<HDRTexture>(hdr_filepath);
    
    // 1. Generate the empty floating-point map
    glGenTextures(1, &state.irradianceMap);
    glBindTexture(GL_TEXTURE_2D, state.irradianceMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 512, 256, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 2. Setup the FBO
    unsigned int captureFBO;
    glGenFramebuffers(1, &captureFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state.irradianceMap, 0);

    // 3. Bake the Image!
    if (!state.irradiance_shader) {
        state.irradiance_shader = std::make_unique<Shader>(irradianceVertexShader, irradianceFragmentShader);
    }
    
    glViewport(0, 0, 512, 256); // Irradiance maps don't need high res, 512x256 is plenty
    state.irradiance_shader->use();
    state.skybox_hdr->bind(0);
    state.irradiance_shader->setInt("environmentMap", 0);
    
    glClear(GL_COLOR_BUFFER_BIT);
    renderQuad(); 
    
    // Cleanup
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &captureFBO);
    
    // Reset viewport back to normal screen size for the game loop
    glViewport(0, 0, state.width, state.height); 
}

void set_metallic_texture(std::shared_ptr<Entity> ent, const std::string& filepath) {
    if (!ent) return;
    if (texture_cache.find(filepath) == texture_cache.end()) {
        texture_cache[filepath] = std::make_shared<Texture>(filepath);
    }
    for(auto& sub : ent->submeshes) { sub.metallicMap = texture_cache[filepath]; }
}

void set_roughness_texture(std::shared_ptr<Entity> ent, const std::string& filepath) {
    if (!ent) return;
    if (texture_cache.find(filepath) == texture_cache.end()) {
        texture_cache[filepath] = std::make_shared<Texture>(filepath);
    }
    for(auto& sub : ent->submeshes) { sub.roughnessMap = texture_cache[filepath]; }
}

void set_normal_texture(std::shared_ptr<Entity> ent, const std::string& filepath) {
    if (!ent) return;
    if (texture_cache.find(filepath) == texture_cache.end()) {
        texture_cache[filepath] = std::make_shared<Texture>(filepath);
    }
    for(auto& sub : ent->submeshes) { sub.normalMap = texture_cache[filepath]; }
}

void set_position(std::shared_ptr<Entity> ent, float x, float y, float z) {
    if (ent) ent->position = Vector3(x, y, z);
}

void set_rotation(std::shared_ptr<Entity> ent, float x, float y, float z) {
    if (ent) ent->rotation = Vector3(x, y, z);
}

void set_scale(std::shared_ptr<Entity> ent, float x, float y, float z) {
    if (ent) ent->scale_vec = Vector3(x, y, z);
}

void set_camera_position(float x, float y, float z) {
    state.camera_pos = Vector3(x, y, z);
}

void set_material(std::shared_ptr<Entity> ent, float r, float g, float b, float metallic, float roughness) {
    if (ent) {
        ent->color = Vector3(r, g, b);
        ent->metallic = metallic;
        ent->roughness = roughness;
    }
}

void camera_look_at(float x, float y, float z) {
    Vector3 target(x, y, z);
    state.camera_front = (target - state.camera_pos).normalize();
    // Update yaw and pitch so mouse movement doesn't snap abruptly
    state.pitch = std::asin(state.camera_front.y) * (180.0f / 3.14159265f);
    state.yaw = std::atan2(state.camera_front.z, state.camera_front.x) * (180.0f / 3.14159265f);
}

// --- ADD THESE TWO FUNCTIONS ---
bool get_key(int key_code) {
    if (!state.window) return false;
    return glfwGetKey(state.window, key_code) == GLFW_PRESS;
}

py::tuple get_position(std::shared_ptr<Entity> ent) {
    if (!ent) return py::make_tuple(0.0f, 0.0f, 0.0f);
    return py::make_tuple(ent->position.x, ent->position.y, ent->position.z);
}

bool is_running() {
    return state.window && !glfwWindowShouldClose(state.window);
}

void set_fps_camera(bool enabled) {
    state.fps_camera_enabled = enabled;
    if (state.window) {
        if (enabled) {
            glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void set_sun_direction(float x, float y, float z) {
    state.sun_dir = Vector3(x, y, z);
}

void set_sun_color(float r, float g, float b) {
    state.sun_color = Vector3(r, g, b);
}


void render_frame() {
    if (!state.window) return;

    // 1. Calculate Delta Time
    auto current_time = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(current_time - state.last_time).count();
    state.last_time = current_time;

    // 2. Escape Hatch
    if (glfwGetKey(state.window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(state.window, true);

    // 3. Process FPS Camera Logic (If enabled)
    if (state.fps_camera_enabled) {
        double mx, my;
        glfwGetCursorPos(state.window, &mx, &my);
        float mouse_x = static_cast<float>(mx);
        float mouse_y = static_cast<float>(my);

        if (state.first_mouse) {
            state.last_x = mouse_x;
            state.last_y = mouse_y;
            state.first_mouse = false;
        }

        float x_offset = mouse_x - state.last_x;
        float y_offset = state.last_y - mouse_y;
        state.last_x = mouse_x;
        state.last_y = mouse_y;

        state.yaw += x_offset * 0.1f;
        state.pitch += y_offset * 0.1f;
        if (state.pitch > 89.0f) state.pitch = 89.0f;
        if (state.pitch < -89.0f) state.pitch = -89.0f;

        Vector3 front;
        front.x = std::cos(state.yaw * (3.14159265f / 180.0f)) * std::cos(state.pitch * (3.14159265f / 180.0f));
        front.y = std::sin(state.pitch * (3.14159265f / 180.0f));
        front.z = std::sin(state.yaw * (3.14159265f / 180.0f)) * std::cos(state.pitch * (3.14159265f / 180.0f));
        state.camera_front = front.normalize();

        float speed = 10.0f * dt;
        if (glfwGetKey(state.window, GLFW_KEY_W) == GLFW_PRESS) state.camera_pos = state.camera_pos + (state.camera_front * speed);
        if (glfwGetKey(state.window, GLFW_KEY_S) == GLFW_PRESS) state.camera_pos = state.camera_pos - (state.camera_front * speed);
        if (glfwGetKey(state.window, GLFW_KEY_A) == GLFW_PRESS) state.camera_pos = state.camera_pos - (state.camera_front.cross(state.world_up).normalize() * speed);
        if (glfwGetKey(state.window, GLFW_KEY_D) == GLFW_PRESS) state.camera_pos = state.camera_pos + (state.camera_front.cross(state.world_up).normalize() * speed);
        if (glfwGetKey(state.window, GLFW_KEY_E) == GLFW_PRESS) state.camera_pos = state.camera_pos + (state.world_up * speed);
        if (glfwGetKey(state.window, GLFW_KEY_Q) == GLFW_PRESS) state.camera_pos = state.camera_pos - (state.world_up * speed);
    }

    // --- PASS 1: RENDER SHADOW MAP ---
    // The second-to-last parameter is now -500.0f instead of 1.0f!
    Matrix4x4 lightProjection = Matrix4x4::ortho(-75.0f, 75.0f, -75.0f, 75.0f, -500.0f, 500.0f);
    Matrix4x4 lightView = Matrix4x4::lookAt(state.sun_dir * -50.0f, Vector3(0,0,0), Vector3(0,1,0));
    Matrix4x4 lightSpaceMatrix = lightProjection * lightView;

    state.shadow_shader->use();
    state.shadow_shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

    glViewport(0, 0, state.SHADOW_RES, state.SHADOW_RES);
    glBindFramebuffer(GL_FRAMEBUFFER, state.depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    state.scene->draw(*state.shadow_shader); 
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // --- PASS 2: RENDER MAIN SCENE ---
    glViewport(0, 0, state.width, state.height);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    state.shader->use();

    // Re-calculate the main camera matrices
    float aspect = state.height > 0 ? (float)state.width / (float)state.height : 1.0f;
    Matrix4x4 projection = Matrix4x4::perspective(45.0f * (3.14159265f / 180.0f), aspect, 0.1f, 100.0f);
    Matrix4x4 view = Matrix4x4::lookAt(state.camera_pos, state.camera_pos + state.camera_front, state.world_up);

    // Bind the generated shadow map to Slot 4
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, state.depthMap);
    state.shader->setInt("shadowMap", 4);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, state.irradianceMap);
    state.shader->setInt("irradianceMap", 5);
    
    // Pass the matrices and uniforms to the main shader
    state.shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    state.shader->setMat4("view", view);
    state.shader->setMat4("projection", projection);
    
    state.shader->setVec3("sunDir", state.sun_dir.x, state.sun_dir.y, state.sun_dir.z);
    state.shader->setVec3("sunColor", state.sun_color.x, state.sun_color.y, state.sun_color.z);
    state.shader->setVec3("viewPos", state.camera_pos.x, state.camera_pos.y, state.camera_pos.z);

    // Draw the scene with shadows applied
    state.scene->draw(*state.shader);


    // --- PASS 3: RENDER SKYBOX ---
    if (state.skybox_hdr) {
        // Change depth function so the skybox renders exactly ON the far clipping plane (1.0)
        glDepthFunc(GL_LEQUAL); 
        
        state.skybox_shader->use();
        state.skybox_shader->setMat4("view", view);
        state.skybox_shader->setMat4("projection", projection);
        
        state.skybox_hdr->bind(0);
        state.skybox_shader->setInt("equirectangularMap", 0);
        
        // Draw the cube geometry
        renderCube();
        
        glDepthFunc(GL_LESS); // Reset depth function to default
    }

    glfwSwapBuffers(state.window);
    glfwPollEvents();
}

// --- PYBIND11 MODULE ---
PYBIND11_MODULE(radiance3d, m) {
    m.doc() = "Radiance3D Core Engine";

    // Expose Entity so Python can hold a reference to it
    py::class_<Entity, std::shared_ptr<Entity>>(m, "Entity");

    // Map the global C++ functions directly to the Python module namespace
    m.def("init_window", &init_window);
    m.def("load_model", &load_model);
    m.def("set_position", &set_position);
    m.def("set_camera_position", &set_camera_position);
    m.def("camera_look_at", &camera_look_at);
    m.def("is_running", &is_running);
    m.def("render_frame", &render_frame);
    m.def("set_fps_camera", &set_fps_camera);
    m.def("get_key", &get_key);
    m.def("get_position", &get_position);
    m.def("set_rotation", &set_rotation);
    m.def("set_scale", &set_scale);
    m.def("set_material", &set_material);
    m.def("set_albedo_texture", &set_albedo_texture);
    m.def("set_metallic_texture", &set_metallic_texture);
    m.def("set_roughness_texture", &set_roughness_texture);
    m.def("set_normal_texture", &set_normal_texture);
    m.def("set_sun_direction", &set_sun_direction);
    m.def("set_sun_color", &set_sun_color);
    m.def("load_skybox", &load_skybox);
}