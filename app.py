import radiance3d as r3d
import time, math

r3d.init_window(800, 600, "Radiance3D - Physically Based Rendering")

# 1. Gold Monkey
gold = r3d.load_model("monkey.glb")
r3d.set_position(gold, -4, 0, 0)
# RGB (Gold), Metallic (1.0 = Pure Metal), Roughness (0.2 = Slightly blurry mirror)
r3d.set_material(gold, 1.0, 0.86, 0.57, 1.0, 0.3) 

# 2. Blue Plastic Monkey
plastic = r3d.load_model("monkey.glb")
r3d.set_position(plastic, 0, 0, 0)
# RGB (Blue), Metallic (0.0 = Dielectric), Roughness (0.8 = Very rough, matte)
r3d.set_material(plastic, 0.1, 0.3, 1.0, 0.0, 0.8)

# 3. Shiny Red Car Paint Monkey
paint = r3d.load_model("monkey.glb")
r3d.set_position(paint, 4, 0, 0)
# RGB (Red), Metallic (0.5), Roughness (0.1 = Very smooth, sharp reflections)
r3d.set_material(paint, 1.0, 0.1, 0.1, 0.5, 0.1)

r3d.set_rotation(gold,math.radians(90),0,0)
r3d.set_rotation(plastic,math.radians(90),0,0)
r3d.set_rotation(paint,math.radians(90),0,0)


r3d.set_camera_position(0, 1, 5)
r3d.camera_look_at(0, 0, 0)

while r3d.is_running():
    r3d.render_frame()