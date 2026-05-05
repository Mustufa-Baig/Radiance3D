import radiance3d as r3d
import time

# init engine
r3d.init_window(800, 600, "Radiance3D - Game Logic")

# Disable the built-in flying camera so we can control the monkey instead
r3d.set_fps_camera(False)

# scene
scene = r3d.load_model("scene.obj")
monkey = r3d.load_model("model.obj")

r3d.set_position(monkey, 2, 5, 0)

# camera
r3d.set_camera_position(20, 5, 10)
r3d.camera_look_at(2, 5, 0)

# Keycodes
W, A, S, D = 87, 65, 83, 68
speed = 10

last_time = time.time()

# main loop
while r3d.is_running():
    # 1. Delta Time (For smooth movement regardless of framerate)
    current_time = time.time()
    dt = current_time - last_time
    last_time = current_time

    # 2. Get current position
    x, y, z = r3d.get_position(monkey)

    # 3. Game Logic (WASD Movement)
    if r3d.get_key(W): z += speed * dt
    if r3d.get_key(S): z -= speed * dt
    if r3d.get_key(A): x -= speed * dt
    if r3d.get_key(D): x += speed * dt

    # 4. Apply new position
    r3d.set_position(monkey, x, y, z)

    # 5. Render
    r3d.render_frame()