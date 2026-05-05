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

// --- GLOBAL ENGINE STATE ---
struct EngineState {
    GLFWwindow* window = nullptr;
    std::unique_ptr<Shader> shader;
    std::unique_ptr<Scene> scene;

    // Camera State
    Vector3 camera_pos = Vector3(0.0f, 0.0f, 5.0f);
    Vector3 camera_front = Vector3(0.0f, 0.0f, -1.0f);
    Vector3 world_up = Vector3(0.0f, 1.0f, 0.0f);
    
    float yaw = -90.0f;
    float pitch = 0.0f;
    float last_x = 400.0f, last_y = 300.0f;
    bool first_mouse = true;
    bool fps_camera_enabled = true;

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
    ent->albedoMap = texture_cache[filepath];
}

void set_metallic_texture(std::shared_ptr<Entity> ent, const std::string& filepath) {
    if (!ent) return;
    if (texture_cache.find(filepath) == texture_cache.end()) {
        texture_cache[filepath] = std::make_shared<Texture>(filepath);
    }
    ent->metallicMap = texture_cache[filepath];
}

void set_roughness_texture(std::shared_ptr<Entity> ent, const std::string& filepath) {
    if (!ent) return;
    if (texture_cache.find(filepath) == texture_cache.end()) {
        texture_cache[filepath] = std::make_shared<Texture>(filepath);
    }
    ent->roughnessMap = texture_cache[filepath];
}

void set_normal_texture(std::shared_ptr<Entity> ent, const std::string& filepath) {
    if (!ent) return;
    if (texture_cache.find(filepath) == texture_cache.end()) {
        texture_cache[filepath] = std::make_shared<Texture>(filepath);
    }
    ent->normalMap = texture_cache[filepath];
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

    // 4. Rendering
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    state.shader->use();

    float aspect = state.height > 0 ? (float)state.width / (float)state.height : 1.0f;
    Matrix4x4 projection = Matrix4x4::perspective(45.0f * (3.14159265f / 180.0f), aspect, 0.1f, 100.0f);
    Matrix4x4 view = Matrix4x4::lookAt(state.camera_pos, state.camera_pos + state.camera_front, state.world_up);

    state.shader->setMat4("view", view);
    state.shader->setMat4("projection", projection);
    state.shader->setVec3("lightColor", 1.0f, 1.0f, 0.9f);
    state.shader->setVec3("lightPos", 5.0f, 10.0f, 5.0f);
    state.shader->setVec3("viewPos", state.camera_pos.x, state.camera_pos.y, state.camera_pos.z);

    state.scene->draw(*state.shader);

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
}